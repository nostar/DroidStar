#include "decoder.h"

#include <cmath>

#include "enhance.h"
#include "fastmath.h"
#include "frame.h"
#include "numeric.h"
#include "decoder_tables.h"
#include "tables.h"

namespace halfrate::dec {

namespace {

using core::fast_exp2;
using core::fast_log2;
using core::fast_sqrt;

constexpr int32_t kFirstNonVoicePitch = 120;  // pitch index 120 and up: silence, tone or erasure
constexpr int32_t kSilencePitch = 124;        // silence frames are dequantised with this pitch index
constexpr int32_t kSilenceVoicing = 16;       // voicing indices 16 and 17 mark silence frames
constexpr int32_t kFirstPulseVoicing = 18;    // voicing 18 and up are pulse codes
constexpr int32_t kPulsePairVoicing = 24;     // 24 and 25: two pulses

constexpr float kOneOverTwoPi = 0x1.45f306p-3f;
constexpr float kOneOver160 = 0x1.99999ap-8f;
constexpr float kSqrtHalf = 0x1.6a09e6p-1f;
constexpr float kSqrt2 = 0x1.6a09e6p+0f;
constexpr float kDbPerLog2 = 0x1.815182p+1f;   // 10 log10(2): power log2 -> dB
constexpr float kLog2PerDb = 0x1.542a5ap-3f;   // log2(10) / 20: amplitude dB -> log2
constexpr float kLimiterCeiling = 32766.0f;
constexpr float kFlushBelow = 0x1.4484cp-100f;  // ~1e-30: the high-pass state is flushed to 0 below this
constexpr float kToneLevelLog2 = 0x1.e353f8p-4f;  // tone level step, log2 per amplitude code
constexpr float kToneScale = 2737.0f;
constexpr float kSampleRate = 8000.0f;
constexpr float kDialTone425 = 425.0f;

// Tone indices
constexpr uint32_t kSingleToneFirst = 5, kSingleToneLast = 122;  // index * 31.25 Hz
constexpr uint32_t kKeypadTones = 0x80, kToneSet = 0x90;        // 16 dual tones each
constexpr uint32_t kProgress350_440 = 0xa0, kTone425 = 0xa1, kProgress480_620 = 0xa2, kProgress350_490 = 0xa3;

core::EnhanceOptions enhance_options(const Options& o) {
	core::EnhanceOptions e;
	e.mode = o.enhance;
	e.strength = o.enhance_strength;
	e.min_weight = o.enhance_min_weight;
	e.max_weight = o.enhance_max_weight;
	e.bypass_divisor = o.enhance_bypass_divisor;
	e.power_high = o.enhance_power_high;
	e.power_low = o.enhance_power_low;
	e.split_frequency = o.enhance_split_frequency;
	return e;
}

}  // namespace

Decoder::Decoder() { reset(); }

void Decoder::reset() {
	core::dequant_reset(dequant);
	synth_reset(synth, options);
	last_good = SynthFrame{};
	had_good = false;
	bad_count = 0;
	muted = false;
	tone_phase = {};
	limiter_gain = 1.0f;
	highpass.sections = {};
	highpass.cutoff = -1.0f;
}

bool Decoder::set(const char* name, double value) {
	SetEffect effect;
	if (!set_option(options, name, value, &effect)) return false;
	if (effect == SetEffect::reset_synth) synth_reset(synth, options);
	return true;
}

// An erased frame: repeat the last good parameters (amplitudes times repeat_gain each time, with a high-band lift
// above repeat_high_start) for repeat_max frames, then one silent frame, then comfort noise.
// Before any good frame: comfort noise at once.
void Decoder::conceal(float* buf) {
	int32_t n = bad_count;
	bad_count = n + 1;
	if (had_good) {
		if (n >= options.repeat_max) {
			if (!muted) {
				SynthFrame silent{};
				synth_frame(synth, options, silent, buf);
				muted = true;
				return;
			}
		} else if (!muted) {
			SynthFrame p = last_good;
			int32_t count = p.harmonic_count;
			float g = options.repeat_gain;
			// the high-band lift rises in dB from 1 at repeat_high_start to repeat_high_gain at half the sample rate
			float start = options.repeat_high_start;
			float span = 0.5f - start > 0.01f ? 0.5f - start : 0.01f;
			float lift = std::log(options.repeat_high_gain > 0.0f ? options.repeat_high_gain : 1.0f);
			for (int32_t k = 0; k < count; k++) {
				float f = (k + 1) * p.fundamental;
				float s = f > start && lift != 0.0f ? g * std::exp(lift * (f - start) / span) : g;
				p.amplitude[1 + k] = p.amplitude[1 + k] * s;
			}
			last_good = p;
			synth_frame(synth, options, p, buf);
			return;
		}
	}
	synth_comfort_noise(synth, options.comfort_noise_level, buf);
	synth.previous = SynthFrame{};
	muted = true;
}

// A frame with pitch index 120 and up that is not silence: a tone frame (bits 0..7 all set and the tone index in
// bits 12..43 four times), otherwise an erasure.
DecodedKind Decoder::tone_or_erasure(const uint8_t* in, float* buf) {
	for (int k = 0; k < 8; k++) {
		if (in[k] == 0) { conceal(buf); return DecodedKind::erasure; }
	}
	int32_t copies[4];
	for (int j = 0; j < 4; j++) {
		uint32_t v = 0;
		for (int k = 0; k < 8; k++) v = in[12 + 8 * j + k] | (v << 1);
		copies[j] = static_cast<int32_t>(v);
	}
	uint32_t index = static_cast<uint32_t>(copies[0]);
	if (!(copies[0] == copies[1] && copies[0] == copies[2] && copies[0] == copies[3])) {
		conceal(buf);
		return DecodedKind::erasure;
	}
	uint32_t level = static_cast<uint32_t>(in[9]) << 3 | static_cast<uint32_t>(in[8]) << 4;
	level |= static_cast<uint32_t>(in[10]) << 2;
	level |= static_cast<uint32_t>(in[11]) << 1;
	level |= in[44];
	bad_count = 0;
	muted = false;

	bool silent, dual;
	float freq_a, freq_b;  // Hz
	if (index - kSingleToneFirst <= kSingleToneLast - kSingleToneFirst) {
		silent = false; dual = false;
		freq_a = static_cast<float>(index) * 31.25f;
		freq_b = 0.0f;
	} else if ((index & ~0xfu) == kToneSet) {
		silent = false;
		const int8_t* pick = tables::kToneSetIndex[index - kToneSet];
		freq_a = tables::kToneSetLow[pick[0]];
		freq_b = tables::kToneSetHigh[pick[1]];
		dual = true;
	} else if ((index & ~0xfu) == kKeypadTones) {
		silent = false;
		uint32_t i = index - kKeypadTones;
		freq_a = tables::kKeypadLow[tables::kKeypadLowIndex[i]];
		freq_b = tables::kKeypadHigh[tables::kKeypadHighIndex[i]];
		dual = true;
	} else {
		dual = false; silent = true; freq_a = 0.0f; freq_b = 0.0f;
		if (index == kProgress480_620) { silent = false; dual = true; freq_a = 480.0f; freq_b = 620.0f; }
		else if (index == kProgress350_490) { silent = false; dual = true; freq_a = 350.0f; freq_b = 490.0f; }
		else if (index == kProgress350_440) { silent = false; dual = true; freq_a = 350.0f; freq_b = 440.0f; }
		else if (index == kTone425) { silent = false; freq_a = kDialTone425; }
	}
	float scale = options.tone_gain;
	float a = fast_exp2(static_cast<float>(level) * kToneLevelLog2);
	if (silent) {
		for (int i = 0; i < kFrameSamples; i++) buf[i] = 0.0f;
	} else {
		float s = scale * kToneScale;
		a = s * a;
		float a_single = a * kSqrt2;  // the 425 Hz tone is a single tone at the level of two
		int64_t inc1 = core::to_int64_sat(static_cast<double>(freq_a / kSampleRate) * 4294967296.0);
		int64_t inc2 = core::to_int64_sat(static_cast<double>(freq_b / kSampleRate) * 4294967296.0);
		if (index == kTone425) a = a_single;
		uint32_t ph1 = tone_phase[0], ph2 = tone_phase[1];
		const float* cosine = halfrate::tables::kCosine;
		for (int i = 0; i < kFrameSamples; i++) {
			float v = cosine[(ph1 + 0x40000u) >> 19];
			if (dual) v = v + cosine[(ph2 + 0x40000u) >> 19];
			buf[i] = a * v;
			ph1 += static_cast<uint32_t>(inc1);
			ph2 += static_cast<uint32_t>(inc2);
		}
		tone_phase[1] = ph2;
		tone_phase[0] = ph1;
	}
	core::dequant_reset(dequant);
	synth_reset(synth, options);
	had_good = false;
	return DecodedKind::tone;
}

// A voice or silence frame from its dequantised parameters: harmonic amplitudes, phase term, enhancement, high-band
// tilt, per-harmonic voiced / noise / pulse weights, pulse placement; then synthesis.
void Decoder::voice(const core::FrameParams& params, bool silence, float* buf) {
	const Options& o = options;
	int32_t voicing = params.fields[core::kFieldVoicing];
	int32_t pitch = params.fields[core::kFieldPitch];
	float voiced_share = fast_sqrt(o.mixed_voiced_share);
	float noise_share = fast_sqrt(1.0f - o.mixed_voiced_share);
	SynthFrame p{};
	float phase[kHarmonicSlots] = {};
	float amp[kFrameSamples];  // [1..L]
	p.active = 1;
	p.fundamental = params.fundamental;
	int32_t count = params.harmonic_count;
	p.harmonic_count = count;
	const int32_t decoded_count = count;
	const float* log2_amp = params.log2_amplitude.data();
	if (decoded_count >= 1) {
		for (int64_t k = 0; k < params.harmonic_count; k++) amp[1 + k] = fast_exp2(log2_amp[k]);
		count = params.harmonic_count;
		// phase term per harmonic: a weighted sum of log-amplitude differences around it (odd taps only if set)
		int64_t taps = o.phase_taps;
		float dispersion = o.phase_dispersion;
		for (int64_t l = 1; l <= count; l++) {
			int64_t l1 = l - 1;
			float s = 0.0f;
			if (taps >= 1) {
				int64_t m = 0, nm = 0;
				int64_t step = o.phase_odd_taps != 0 ? 2 : 1;
				do {
					float up = 0.0f, lo;
					if (count >= static_cast<int32_t>(static_cast<int32_t>(l1) + static_cast<int32_t>(m) + 2)) {
						up = log2_amp[l1 + m + 1];
						up = up + o.phase_level_offset;
					}
					if (l1 + nm >= 1) {
						lo = log2_amp[l1 + nm - 1];
						lo = lo + o.phase_level_offset;
					} else {
						lo = 0.0f;
					}
					up = up - lo;
					up = up * tables::kPhaseTapWeight[m + 1];
					s = s + up;
					m += step;
					nm -= step;
				} while (m + 1 <= taps);
			}
			s = dispersion * s;
			s = s * kOneOverTwoPi;
			phase[l] = s;
		}
	}
	core::enhance_formants(enhance_options(o), params.fundamental, params.omega, params.harmonic_count, amp);
	count = params.harmonic_count;
	bool weights_done = false;
	if (o.high_band_tilt != 0.0f) {
		if (count < 1) {
			weights_done = true;
		} else {
			// a gain rising from 1 at high_band_start * L to 2^high_band_tilt at L
			for (int64_t l = 1;; l++) {
				float t = static_cast<float>(static_cast<uint32_t>(l)) / static_cast<float>(count);
				float start = o.high_band_start;
				t = t - start;
				float width = 1.0f - start;
				t = t / width;
				if (t > 0.0f) {
					float a = amp[l];
					t = t > 1.0f ? 1.0f : t;
					t = t * o.high_band_tilt;
					amp[l] = a * fast_exp2(t);
				}
				if (!(l < count)) break;
			}
		}
	}
	if (!weights_done && count >= 1) {
		const char* classes = tables::kHarmonicClass[silence ? tables::kSilenceClassRow : (voicing & 0x1f)];
		float w0 = params.fundamental;
		float gain = o.output_gain;
		for (int64_t k = 0; k < count; k++) {
			float x = w0 * static_cast<float>(static_cast<uint32_t>(k + 1));
			int32_t slot = core::to_int32_sat(static_cast<double>(x) * 32.0);
			slot = slot < 15 ? slot : 15;
			char c = classes[slot];
			p.amplitude[1 + k] = amp[1 + k] * gain;
			float weight = 1.0f;
			std::array<float, kHarmonicSlots>* target = &p.unvoiced;
			switch (c) {
			case tables::kPulseNoise:
				if (o.pulse_enable != 0) {
					p.pulse[1 + k] = o.pulse_noise_pulse_weight;
					weight = o.pulse_noise_noise_weight;
				}
				break;
			case tables::kVoiced:
				weight = o.voiced_weight;
				target = &p.voiced;
				break;
			case tables::kMixed:
				p.voiced[1 + k] = voiced_share * o.voiced_weight;
				weight = noise_share;
				break;
			case tables::kPulse:
				target = o.pulse_enable == 0 ? &p.unvoiced : &p.pulse;
				break;
			default:
				break;
			}
			(*target)[1 + k] = weight;
		}
		for (int32_t l = 1; l <= count; l++) p.phase[l] = phase[l];
	}

	p.pulse_count = 0;
	if (!silence && voicing >= kFirstPulseVoicing) {
		if (o.pulse_enable != 0) {
			float window = o.pulse_window;
			float distance;
			if ((voicing & 0x7ffffffe) == kPulsePairVoicing) {
				// two pulses: positions from pitch / 11 and pitch % 11
				int32_t q = pitch / 11, r = pitch - q * 11;
				float s = o.pulse_pair_first_step * static_cast<float>(q);
				s = o.pulse_pair_first_start + s;
				s = s + 0.5f;
				int32_t pos = core::to_int32_sat(s);
				float s2 = o.pulse_pair_second_step * static_cast<float>(r);
				s2 = o.pulse_pair_second_start + s2;
				s2 = s2 + 0.5f;
				int32_t pos2 = core::to_int32_sat(s2);
				float d1 = static_cast<float>(static_cast<int32_t>(static_cast<uint32_t>(pos) - 160u));
				d1 = pos < 160 ? -d1 : d1;
				float d2 = static_cast<float>(static_cast<int32_t>(static_cast<uint32_t>(pos2) - 80u));
				d1 = d1 / window;
				d1 = 1.0f - d1;
				d1 = core::max_num(d1, 0.0f);
				p.pulse_gain[0] = d1;
				p.pulse_position[1] = pos2;
				distance = pos2 < 80 ? -d2 : d2;
				p.pulse_count = 2;
				p.pulse_position[0] = pos;
			} else {
				int32_t base = static_cast<uint32_t>(voicing) < static_cast<uint32_t>(kPulsePairVoicing)
				                   ? o.pulse_position_low : o.pulse_position_high;
				p.pulse_position[0] = static_cast<int32_t>(static_cast<uint32_t>(base) + static_cast<uint32_t>(pitch));
				float d = static_cast<float>(static_cast<int32_t>(static_cast<uint32_t>(pitch) -
				                                                   static_cast<uint32_t>(o.pulse_window_center)));
				bool below = static_cast<int64_t>(pitch) - static_cast<int64_t>(o.pulse_window_center) < 0;
				distance = below ? -d : d;
				p.pulse_count = 1;
			}
			float g = distance / window;
			g = 1.0f - g;
			g = core::max_num(g, 0.0f);
			p.pulse_gain[p.pulse_count - 1] = g;
		}
	} else if (silence && decoded_count >= 1) {
		float g = o.silence_gain;
		for (int32_t k = 0; k < decoded_count; k++) p.amplitude[1 + k] = p.amplitude[1 + k] * g;
	}
	bad_count = 0;
	muted = false;
	synth_frame(synth, options, p, buf);
	last_good = p;
	had_good = true;
}

// Output stage: the high-pass (when highpass_hz > 0), the frame limiter, the soft limiter, rounding to 16 bits.
void Decoder::output(float* buf, int16_t* pcm) {
	const Options& o = options;
	float fc = o.highpass_hz;
	HighPass& hp = highpass;
	if (fc > 0.0f) {
		if (!(hp.cutoff == fc)) {
			// RBJ high-pass biquad, Q = 1/sqrt(2)
			float t = fc * 8192.0f;
			t = t / kSampleRate;
			t = t + 0.5f;
			int32_t i = core::to_int32_sat(static_cast<double>(t));
			const float* cosine = halfrate::tables::kCosine;
			float cos_w = cosine[static_cast<uint32_t>(i) & 0x1fff];
			float alpha = cosine[(static_cast<uint32_t>(i) + 0x1800) & 0x1fff];
			alpha = alpha * kSqrtHalf;
			float a0 = alpha + 1.0f;
			float one_plus_cos = cos_w + 1.0f;
			hp.b1 = -one_plus_cos / a0;
			float b0 = one_plus_cos * 0.5f, a1 = cos_w * -2.0f;
			b0 = b0 / a0;
			a1 = a1 / a0;
			hp.b0 = b0;
			hp.b2 = b0;
			hp.a1 = a1;
			hp.a2 = (1.0f - alpha) / a0;
			hp.cutoff = fc;
		}
		for (HighPass::Section& z : hp.sections) {
			for (int i = 0; i < kFrameSamples; i++) {
				float x = buf[i];
				float y = x * hp.b0;
				float p0 = hp.b1 * z.x1, p1 = hp.b2 * z.x2;
				y = y + p0;
				y = y + p1;
				float q0 = hp.a1 * z.y1, q1 = hp.a2 * z.y2;
				y = y - q0;
				y = y - q1;
				float old_x1 = z.x1, old_y1 = z.y1;
				z.x1 = x;
				z.x2 = old_x1;
				// written as !(>=) so a NaN is flushed too rather than kept in the filter state
				z.y1 = !(std::fabs(y) >= kFlushBelow) ? 0.0f : y;
				z.y2 = !(std::fabs(old_y1) >= kFlushBelow) ? 0.0f : old_y1;
				buf[i] = y;
			}
		}
	}
	if (o.frame_limiter != 0) {
		// frame rms in dB; above the knee the excess is scaled by the ratio; the gain ramps across the frame
		float e = 0.0f;
		for (int i = 0; i < kFrameSamples; i++) { float q = buf[i] * buf[i]; e = e + q; }
		e = e * kOneOver160;
		float db = 0.0f, g = 1.0f;
		if (e > 1.0f) db = fast_log2(e) * kDbPerLog2;
		float knee = o.frame_limiter_knee_db;
		if (db > knee) {
			float t = db - knee;
			t = t * o.frame_limiter_ratio;
			t = knee + t;
			t = t - db;
			t = t * kLog2PerDb;
			g = fast_exp2(t);
		}
		for (uint32_t i = 0; i < kFrameSamples; i++) {
			float r = static_cast<float>(i + 1) * kOneOver160;
			float g0 = limiter_gain;
			float dg = g - g0;
			r = r * dg;
			r = g0 + r;
			buf[i] = buf[i] * r;
		}
		limiter_gain = g;
	}
	// soft limiter: above the threshold x - t is mapped to (x - t) / (1 + (x - t) / (ceiling - t))
	float t = o.limiter_threshold;
	bool on = !(t <= 0.0f);
	float headroom = kLimiterCeiling - t;
	for (int i = 0; i < kFrameSamples; i++) {
		float x = buf[i];
		bool negative = x < 0.0f;
		float ax = negative ? -x : x;
		if (on && !(t >= ax)) {
			float v = ax - t;
			v = v / headroom;
			float w = v * headroom;
			v = v + 1.0f;
			v = w / v;
			v = t + v;
			x = negative ? -v : v;
		}
		bool high = x >= 32767.0f, low = -32768.0f >= x;
		if (!high && !low) {
			float h = x >= 0.0f ? 0.5f : -0.5f;
			pcm[i] = static_cast<int16_t>(static_cast<uint16_t>(core::to_int32_sat(static_cast<double>(x + h))));
		} else {
			pcm[i] = low && !high ? -32768 : 32767;
		}
	}
}

DecodedKind Decoder::decode_bits(const uint8_t* in, int16_t* pcm) {
	uint8_t bits[core::kFrameBits];
	if (options.rotated_layout == 1) core::standard_from_rotated(in, bits);
	else for (int i = 0; i < core::kFrameBits; i++) bits[i] = in[i];
	const core::FrameFields received = core::fields_from_bits(bits);
	float buf[kFrameSamples];
	core::FrameParams params;
	DecodedKind kind;
	int32_t pitch = received[core::kFieldPitch];
	if (pitch < kFirstNonVoicePitch) {
		// voice. Pulse frames may be dequantised with a fixed pitch index; the received one places the pulses.
		core::FrameFields fields = received;
		if (received[core::kFieldVoicing] >= kFirstPulseVoicing && options.pulse_pitch_index >= 0) {
			fields[core::kFieldPitch] = options.pulse_pitch_index;
			core::dequantize(dequant, fields, &params);
			params.fields[core::kFieldPitch] = received[core::kFieldPitch];
		} else {
			core::dequantize(dequant, fields, &params);
		}
		voice(params, false, buf);
		kind = DecodedKind::voice;
	} else if ((received[core::kFieldVoicing] & ~1) == kSilenceVoicing) {
		// silence: dequantised at a fixed pitch; silence_predictor decides what that does to the predictor
		core::FrameFields fields = received;
		fields[core::kFieldPitch] = kSilencePitch;
		int32_t mode = options.silence_predictor;
		if (mode == 2 || mode == 3) {
			core::DequantState saved = dequant;
			if (mode == 2) core::dequant_reset(dequant);
			core::dequantize(dequant, fields, &params);
			dequant = saved;
		} else {
			if (mode == 1) core::dequant_reset(dequant);
			core::dequantize(dequant, fields, &params);
			if (mode == 4 || mode == 1) core::dequant_reset(dequant);
		}
		params.fields[core::kFieldVoicing] = kSilenceVoicing;
		voice(params, true, buf);
		kind = DecodedKind::silence;
	} else {
		// the tone fields are read from the frame as received (no layout rotation)
		kind = tone_or_erasure(in, buf);
	}
	output(buf, pcm);
	return kind;
}

void Decoder::decode_erased(int16_t* pcm) {
	float buf[kFrameSamples];
	conceal(buf);
	output(buf, pcm);
}

}  // namespace halfrate::dec
