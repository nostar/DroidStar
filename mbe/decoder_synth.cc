#include "decoder_synth.h"

#include <cmath>

#include "fastmath.h"
#include "fft.h"
#include "numeric.h"
#include "tables.h"

namespace halfrate::dec {

namespace {

using core::fast_exp2;
using core::fast_log2;
using core::fast_sqrt;
using core::to_int32_sat;
using core::to_int64_sat;

constexpr float kOneOver160 = 0x1.99999ap-8f;
constexpr float kOneOverTwoPi = 0x1.45f306p-3f;
constexpr float kLogFloor = 0x1.0c6f7ap-20f;  // 1e-6
constexpr float kLn2 = 0x1.62e43p-1f;
constexpr float kLog2E = 0x1.715476p+0f;
constexpr float kWhiteNoiseScale = 0x1.186f18p-2f;
constexpr float kSqrt3 = 0x1.bb67aep+0f;  // uniform noise of amplitude 1 has rms 1/sqrt(3)
constexpr float kFlushBelow = 1.0000000031710769e-30f;
constexpr uint32_t kSeedOffset = 0x51c0de01u;

const float* const kCos = tables::kCosine;

inline uint32_t next_random(uint32_t x) { return x * 1664525u + 1013904223u; }

// x * 2^bits to int64, truncating and saturating
inline int64_t to_fixed64(float x, int bits) { return to_int64_sat(static_cast<double>(x) * static_cast<double>(1ull << bits)); }

// a fundamental (cycles per sample) as a 32.32 fixed-point phase increment, in the top half of 64 bits
inline uint64_t phase_increment(float fundamental) { return static_cast<uint64_t>(to_fixed64(fundamental, 32)) << 32; }

// high 64 bits of the signed 128-bit product
inline int64_t multiply_high(int64_t a, int64_t b) { return static_cast<int64_t>((static_cast<__int128>(a) * b) >> 64); }

// NaN (a zero-length crossfade at its start point) gives 0
inline float clamp01(float v) {
	v = !(v >= 0.0f) ? 0.0f : v;
	return v > 1.0f ? 1.0f : v;
}

// The noise spectrum of a frame at the 65 bins of a 128-point FFT: amplitude[k] and its unvoiced weight band[k],
// taken from the harmonic nearest bin k.
void noise_bins(const Options& options, const SynthFrame& frame, float* amplitude, float* band) {
	if (frame.active == 0) {
		for (int k = 0; k < 65; k++) { amplitude[k] = 0.0f; band[k] = 0.0f; }
		return;
	}
	float w0 = frame.fundamental;
	float period = 1.0f / w0;
	float gain = options.unvoiced_gain;
	gain = gain * fast_sqrt(32.0f / w0);
	for (uint32_t k = 0; k < 65; k++) {
		float x = period * (static_cast<float>(k) * (1.0f / 128.0f));
		x = x + 0.5f;
		int32_t l = to_int32_sat(x);
		amplitude[k] = 0.0f;
		band[k] = 0.0f;
		if (l <= 0) {
			if (options.unvoiced_low_cut != 0) continue;
			l = 1;
		}
		if (l > frame.harmonic_count) continue;
		float bin_gain;
		if (k == 1) bin_gain = options.noise_gain_bin1;
		else if (k <= 3) bin_gain = options.noise_gain_bins23;
		else bin_gain = 1.0f;
		float a = gain * frame.amplitude[l];
		amplitude[k] = a * bin_gain;
		band[k] = frame.unvoiced[l];
	}
}

// In-place 128-point forward FFT (kernel e^-j), unscaled.
void fft128(float* re, float* im) {
	for (int i = 0; i < 128; i++) {
		int j = 0;
		for (int b = 0; b < 7; b++) j |= ((i >> b) & 1) << (6 - b);
		if (i < j) {
			float a = re[i], b = im[i];
			re[i] = re[j]; im[i] = im[j];
			re[j] = a; im[j] = b;
		}
	}
	for (uint32_t len = 2; len <= 128; len *= 2) {
		uint32_t half = len >> 1;
		uint32_t step = 8192u / len;
		for (uint32_t base = 0; base < 128; base += len) {
			for (uint32_t k = 0; k < half; k++) {
				float c = kCos[k * step];
				float s = kCos[(0x1800 + k * step) & 0x1fff];
				uint32_t p = base + half + k, q = base + k;
				float xr = re[p], xi = im[p];
				float tr = c * xr + s * xi;
				float ti = c * xi - s * xr;
				re[p] = re[q] - tr;
				im[p] = im[q] - ti;
				re[q] = tr + re[q];
				im[q] = ti + im[q];
			}
		}
	}
}

// The spectrum at the 257 bins of a 512-point FFT: amplitude times weight of the harmonic nearest each bin.
// Bins above harmonic L get harmonic L's value up to L + 0.5 + top * L when top > 0.
void spectrum_bins(const SynthFrame& frame, const float* weight, int low_cut, float top, float* out) {
	float period = 1.0f / frame.fundamental;
	for (uint32_t k = 0; k < 257; k++) {
		float h = period * (static_cast<float>(k) * (1.0f / 512.0f));
		int32_t l = to_int32_sat(h + 0.5f);
		float v = 0.0f;
		if (!(l <= 0 && low_cut != 0)) {
			int32_t m = l > 0 ? l : 1;
			int32_t count = frame.harmonic_count;
			bool inside = true;
			if (m > count) {
				inside = false;
				if (top > 0.0f) {
					float fl = static_cast<float>(count);
					float limit = (fl + 0.5f) + top * fl;
					if (h < limit) { m = count; inside = true; }
				}
			}
			if (inside) v = frame.amplitude[m] * weight[m];
		}
		out[k] = v;
	}
}

// In-place inverse 512-point FFT, scaled by 1/512.
void ifft512(float* re, float* im) {
	for (int i = 0; i < 512; i++) im[i] = -im[i];
	core::fft512(re, im);
	for (int i = 0; i < 512; i++) {
		re[i] = re[i] * (1.0f / 512.0f);
		im[i] = im[i] * (-1.0f / 512.0f);
	}
}

// The ramp start and end of a glide over [start, end) samples, clamped to the frame; the whole frame when empty.
void glide_span(int32_t start, int32_t end, int32_t* from, int32_t* to) {
	if (start < 0) start = 0;
	if (end >= kFrameSamples) end = kFrameSamples;
	if (end > start) { *to = end; *from = start; }
	else { *to = kFrameSamples; *from = 0; }
}

// Harmonics with phase_tracking 1: one fundamental phase track, harmonic l at l times it plus its own phase offset.
void harmonics_tracked(SynthState& state, const Options& options, const SynthFrame& frame, float* out,
                       int count, uint64_t previous_increment) {
	const SynthFrame& prev = state.previous;
	uint64_t current_increment = previous_increment;
	if (frame.active) current_increment = phase_increment(frame.fundamental);

	// the fundamental's phase per sample; its frequency glides between the two frames' over the pitch glide span
	uint64_t phase[kFrameSamples];
	int32_t start, end;
	glide_span(options.pitch_glide_start, options.pitch_glide_end, &start, &end);
	int64_t slope = static_cast<int64_t>(current_increment - previous_increment) / static_cast<int64_t>(end - start);
	int64_t half_slope = static_cast<int64_t>(static_cast<uint64_t>(slope) + (static_cast<uint64_t>(slope) >> 63)) >> 1;
	uint64_t acc = state.fundamental_phase;
	uint64_t freq = previous_increment;
	for (int64_t n = 0; n < kFrameSamples; n++) {
		phase[n] = acc;
		uint64_t step = freq + half_slope, next = freq + slope;
		if (n >= end) { next = freq; step = current_increment; }
		if (static_cast<uint64_t>(n) < static_cast<uint64_t>(static_cast<uint32_t>(start))) step = previous_increment;
		else freq = next;
		acc = step + acc;
	}
	state.fundamental_phase = acc;

	for (int64_t l = 1; l <= count; l++) {
		bool in_prev = prev.active ? l <= prev.harmonic_count : false;
		bool in_cur = frame.active ? l <= frame.harmonic_count : false;
		float amp_prev = in_prev ? prev.amplitude[l] : 0.0f;
		float amp_cur = in_cur ? frame.amplitude[l] : 0.0f;
		float voi_prev = in_prev ? prev.voiced[l] : 0.0f;
		float voi_cur = in_cur ? frame.voiced[l] : 0.0f;
		if (voi_prev == 0.0f && voi_cur == 0.0f) continue;
		bool both = in_prev && in_cur;
		if (both && options.amplitude_frequency_map && voi_prev > 0.0f && voi_cur > 0.0f) {
			// the previous frame's amplitude at the frequency of this frame's harmonic l
			float x = static_cast<float>(static_cast<uint32_t>(l));
			x = frame.fundamental * x;
			x = x / prev.fundamental;
			int32_t i = to_int32_sat(x);
			float fraction = x - static_cast<float>(to_int32_sat(x));
			if (i < 1) fraction = 0.0f;
			i = i > 1 ? i : 1;
			int32_t prev_count = prev.harmonic_count;
			float frac = i < prev_count ? fraction : 0.0f;
			i = i < prev_count ? i : prev_count;
			amp_prev = prev.amplitude[i];
			if (frac > 0.0f && amp_prev > 0.0f && prev.amplitude[i + 1] > 0.0f) {
				float l0 = fast_log2(amp_prev);
				float l1 = fast_log2(prev.amplitude[i + 1]);
				float l0b = fast_log2(prev.amplitude[i]);
				float y = l1 - l0b;
				y = frac * y;
				y = l0 + y;
				amp_prev = fast_exp2(y);
			}
		}
		if (!in_prev || (voi_prev == 0.0f && options.amplitude_hold)) amp_prev = amp_cur;
		if (!in_cur || (options.amplitude_hold && voi_cur == 0.0f)) amp_cur = amp_prev;
		float lo, hi;
		if (!both) { lo = options.harmonic_fade_start; hi = options.harmonic_fade_end; }
		else if (voi_cur < voi_prev) { lo = options.voicing_off_start; hi = options.voicing_off_end; }
		else { lo = options.voicing_on_start; hi = options.voicing_on_end; }
		float phase_cur = in_cur ? frame.phase[l] : prev.phase[l];
		float phase_prev = phase_cur;
		if (in_prev) phase_prev = prev.phase[l];
		float dphase = phase_cur - phase_prev;
		float round = dphase >= 0.0f ? 0.5f : -0.5f;
		bool use_log = (amp_cur > 0.0f && amp_prev > 0.0f) && options.amplitude_log_interp != 0;
		int32_t amp_start, amp_end;
		glide_span(options.amplitude_glide_start, options.amplitude_glide_end, &amp_start, &amp_end);
		float ratio, amp_slope;
		if (use_log) {
			float lc = fast_log2(amp_cur);
			float lp = fast_log2(amp_prev);
			float y = lc - lp;
			float r = 1.0f / static_cast<float>(amp_end - amp_start);
			ratio = fast_exp2(r * y);
			amp_slope = 0.0f;
		} else {
			float d = amp_cur - amp_prev;
			float r = 1.0f / static_cast<float>(amp_end - amp_start);
			amp_slope = d * r;
			ratio = 1.0f;
		}
		float phase_wrap = dphase - static_cast<float>(to_int32_sat(dphase + round));
		float voicing_span = hi - lo;
		float dvoicing = voi_cur - voi_prev;
		float amp = amp_prev;
		for (uint64_t n = 0; n < kFrameSamples; n++) {
			if (n < static_cast<uint64_t>(static_cast<uint32_t>(amp_start)) || static_cast<int64_t>(n) >= amp_end) {
				amp = static_cast<int64_t>(n) < amp_end ? amp : amp_cur;
			} else if (use_log) {
				amp = ratio * amp;
			} else {
				amp = amp_prev + amp_slope * static_cast<float>(static_cast<int32_t>(n) - amp_start + 1);
			}
			float t = static_cast<float>(static_cast<uint32_t>(n + 1));
			float phi = t * kOneOver160;
			phi = phase_wrap * phi;
			phi = phase_prev + phi;
			float v = t - lo;
			v = v / voicing_span;
			v = clamp01(v);
			v = dvoicing * v;
			v = voi_prev + v;
			uint64_t x = phase[n] * static_cast<uint64_t>(l);
			int64_t offset = to_fixed64(phi, 32);
			v = v * amp;
			x = x + (static_cast<uint64_t>(offset) << 32);
			x = x + (1ull << 50);
			x >>= 51;
			v = v * kCos[x];
			out[n] = out[n] + v;
		}
	}
}

// Harmonics with any other phase_tracking: one phase accumulator per harmonic, linear frequency glide.
void harmonics_accumulated(SynthState& state, const Options& options, const SynthFrame& frame, float* out,
                           int count, uint64_t previous_increment) {
	const SynthFrame& prev = state.previous;
	int cur_active = frame.active;
	uint64_t current_increment = previous_increment;
	if (cur_active) current_increment = phase_increment(frame.fundamental);
	int64_t cur_count = frame.harmonic_count, prev_count = prev.harmonic_count;
	int prev_active = prev.active;
	for (int64_t l = 1; l <= count; l++) {
		bool absent_prev = !(prev_active != 0 && l <= prev_count);
		bool absent_cur = !(cur_active != 0 && l <= cur_count);
		float amp_prev = 0.0f, amp_cur = 0.0f;
		if (!absent_prev) amp_prev = prev.amplitude[l] * prev.voiced[l];
		if (!absent_cur) amp_cur = frame.amplitude[l] * frame.voiced[l];
		uint64_t freq_cur = current_increment * static_cast<uint64_t>(l);
		uint64_t acc = state.harmonic_phase[l];
		uint64_t freq_prev = previous_increment * static_cast<uint64_t>(l);
		if (absent_prev) freq_prev = freq_cur;
		int64_t delta = static_cast<int64_t>(freq_cur - freq_prev);
		if (absent_cur) delta = 0;
		// delta / 160 and delta / 320, truncating
		int64_t h = multiply_high(delta, 0x6666666666666667LL);
		int64_t step = (h >> 6) + static_cast<int64_t>(static_cast<uint64_t>(h) >> 63);
		if (amp_prev == 0.0f && amp_cur == 0.0f) {
			acc = static_cast<uint64_t>(step) * 12800u + freq_prev * 160u + acc;
			state.harmonic_phase[l] = acc;
			continue;
		}
		bool fade = false;
		if (amp_prev != 0.0f && amp_cur == 0.0f) fade = l > cur_count ? false : cur_active != 0;
		uint64_t freq = static_cast<uint64_t>((h >> 7) + static_cast<int64_t>(static_cast<uint64_t>(h) >> 63)) + freq_prev;
		float damp = amp_cur - amp_prev;
		damp = damp * kOneOver160;
		for (uint64_t n = 0; n < kFrameSamples; n++) {
			float a;
			float fade_end = options.voiced_fade_end, fade_start = options.voiced_fade_start;
			if (fade && fade_end > fade_start) {
				float x = static_cast<float>(static_cast<uint32_t>(n)) - fade_start;
				float y = fade_end - fade_start;
				x = x / y;
				x = clamp01(x);
				x = x * 0.5f;
				x = cosine_interpolated(x);
				x = x * 0.5f;
				x = x + 0.5f;
				a = amp_prev * x;
			} else {
				a = damp * static_cast<float>(static_cast<uint32_t>(n + 1));
				a = amp_prev + a;
			}
			a = a * kCos[(acc + (1ull << 50)) >> 51];
			out[n] = out[n] + a;
			acc = freq + acc;
			freq = freq + static_cast<uint64_t>(step);
		}
		state.harmonic_phase[l] = acc;
	}
}

// noise_method 1: noise in four 40-sample blocks, 128-point FFTs, 80-sample sine windows, overlap-add.
void noise_blocks(SynthState& state, const Options& options, const SynthFrame& frame, float* out) {
	float amp0[65], band0[65], amp1[65], band1[65], log0[65], log1[65];
	noise_bins(options, state.previous, amp0, band0);
	noise_bins(options, frame, amp1, band1);
	bool none = true;
	for (int k = 0; k < 65; k++) {
		if (band0[k] == 0.0f && band1[k] == 0.0f) continue;
		float x = amp0[k], y = amp1[k];
		if (x == 0.0f) { amp0[k] = y; x = y; }
		if (y == 0.0f) { amp1[k] = x; y = x; }
		log0[k] = x > 0.0f ? fast_log2(x) : 0.0f;
		log1[k] = y > 0.0f ? fast_log2(y) : 0.0f;
		none = false;
	}
	if (none) return;
	float white_re[128], white_im[128], re[128], im[128];
	const float* window = state.window80.data();
	for (int block = 0; block < 4; block++) {
		float pos = static_cast<float>(static_cast<uint32_t>(40 * block + 40));
		float x = (pos - options.voicing_off_start) / (options.voicing_off_end - options.voicing_off_start);
		float y = (pos - options.voicing_on_start) / (options.voicing_on_end - options.voicing_on_start);
		float fall = clamp01(x);
		float rise = clamp01(y);
		if (options.noise_white) {
			for (int n = 0; n < 128; n++) white_im[n] = 0.0f;
			for (int n = 0; n < 128; n++) {
				float v = 0.0f;
				if (n < 80) {
					state.random = next_random(state.random);
					v = static_cast<float>(state.random >> 8) * (1.0f / 8388608.0f);
					v = v + -1.0f;
					v = window[n] * v;
				}
				white_re[n] = v;
			}
			fft128(white_re, white_im);
		}
		float frame_pos = pos * kOneOver160;
		for (int n = 0; n < 128; n++) { re[n] = 0.0f; im[n] = 0.0f; }
		for (int k = 0; k < 63; k++) {
			state.random = next_random(state.random);
			uint32_t ph = state.random >> 19;
			if (static_cast<int64_t>(k + 1) < static_cast<int64_t>(options.noise_coherent_bins)) {
				float jitter = static_cast<float>(static_cast<int32_t>(ph - 4096));
				jitter = options.noise_phase_jitter * jitter;
				int32_t ji = to_int32_sat(jitter);
				uint32_t v = static_cast<uint32_t>(0xa00 * (k + 1)) + state.noise_phase[k];
				v = v + static_cast<uint32_t>(ji);
				ph = v & 0x1fff;
				state.noise_phase[k] = ph;
			}
			float bp = band0[k + 1];
			if (bp == 0.0f && band1[k + 1] == 0.0f) continue;
			float x0 = amp0[k + 1], x1 = amp1[k + 1], m;
			if (x0 > 0.0f && x1 > 0.0f && options.amplitude_log_interp) {
				float d = log1[k + 1] - log0[k + 1];
				d = frame_pos * d;
				m = fast_exp2(log0[k + 1] + d);
			} else {
				float d = x1 - x0;
				d = frame_pos * d;
				m = x0 + d;
			}
			float bc = band1[k + 1];
			float d = bc - bp;
			float g = bc > bp ? fall : rise;
			g = d * g;
			g = bp + g;
			g = m * g;
			if (g == 0.0f) continue;
			float vr, vi;
			if (options.noise_white) {
				vr = g * white_re[k + 1];
				vr = vr * kWhiteNoiseScale;
				vi = g * white_im[k + 1];
				vi = vi * kWhiteNoiseScale;
			} else {
				vr = g * kCos[ph];
				vi = g * kCos[(ph + 0x1800) & 0x1fff];
			}
			re[k + 1] = vr;
			im[k + 1] = vi;
			re[127 - k] = vr;
			im[127 - k] = -vi;
		}
		for (int n = 0; n < 128; n++) im[n] = -im[n];
		fft128(re, im);
		for (int n = 0; n < 80; n++) {
			float w = 1.0f;
			if (!options.noise_white) w = window[n];
			float v = re[n] * (1.0f / 128.0f);
			v = v * w;
			int idx = 40 * block + n;
			if (idx <= 159) out[idx] = v + out[idx];
			else state.overlap[idx - 160] = v + state.overlap[idx - 160];
		}
	}
}

// noise_method 0: one 512-point FFT of random-phase noise, 320-sample window, added at noise_offset.
void noise_single(SynthState& state, const Options& options, const SynthFrame& frame, float* out) {
	float bins[257], noise[320], re[512], im[512];
	float gain = options.unvoiced_gain;
	gain = gain * fast_sqrt(128.0f / frame.fundamental);
	int64_t offset = options.noise_offset;  // 0..160 (set_option clamps)
	spectrum_bins(frame, frame.unvoiced.data(), options.unvoiced_low_cut, options.unvoiced_top_extend, bins);
	bool any = false;
	for (int k = 0; k < 257; k++) if (bins[k] != 0.0f) { any = true; break; }
	if (!any) {
		for (int n = 0; n < 320; n++) noise[n] = 0.0f;
	} else {
		uint32_t r = state.random;
		for (int k = 0; k < 257; k++) {
			r = next_random(r);
			if ((k & 0xff) == 0) { re[k] = 0.0f; im[k] = 0.0f; continue; }
			float a = gain * bins[k];
			float c = a * kCos[r >> 19];
			re[k] = c;
			float s = kCos[((r >> 19) + 0x1800) & 0x1fff];
			im[k] = a * s;
			re[512 - k] = c;
			im[512 - k] = -(a * s);
		}
		state.random = r;
		ifft512(re, im);
		for (int n = 0; n < 320; n++) noise[n] = re[n] * state.window320[n];
	}
	for (int64_t j = 0; j < 320; j++) {
		int64_t idx = offset + j;
		if (idx <= 159) out[idx] = out[idx] + noise[j];
		else state.overlap[idx - 160] = state.overlap[idx - 160] + noise[j];
	}
}

// Pulses: a 512-sample pulse shape from the pulse weights (zero or minimum phase), windowed, added at the frame's
// pulse positions.
void add_pulses(SynthState& state, const Options& options, const SynthFrame& frame, float* out) {
	float weights[257], re[512], im[512], pulse[512];
	float gain = options.pulse_gain;
	gain = gain * fast_sqrt(40.0f / frame.fundamental);
	spectrum_bins(frame, frame.pulse.data(), 0, options.unvoiced_top_extend, weights);
	for (uint32_t k = 0; k < 257; k++) {
		float f = static_cast<float>(k) * 15.625f;
		if (!(f < options.pulse_highpass_hz)) break;
		weights[k] = 0.0f;
	}
	if (options.pulse_minimum_phase) {
		// cepstrum of the log magnitude, folded to make it causal, back to a spectrum, exponentiated
		for (int k = 0; k < 257; k++) {
			float x = core::max_num(gain * weights[k], kLogFloor);
			x = fast_log2(x) * kLn2;
			re[k] = x; im[k] = 0.0f;
			if (k & 0xff) { re[512 - k] = x; im[512 - k] = 0.0f; }
		}
		ifft512(re, im);
		for (int k = 1; k < 256; k++) {
			re[k] = re[k] + re[k];
			re[512 - k] = 0.0f;
		}
		for (int k = 0; k < 512; k++) im[k] = 0.0f;
		core::fft512(re, im);
		for (int k = 0; k < 512; k++) {
			float m = fast_exp2(re[k] * kLog2E);
			float p = im[k] * kOneOverTwoPi;
			re[k] = m * cosine_interpolated(p);
			p = p + -0.25f;
			im[k] = m * cosine_interpolated(p);
		}
		ifft512(re, im);
		for (int n = 0; n < 256; n++) pulse[n] = 0.0f;
		for (int n = 0; n < 256; n++) pulse[256 + n] = re[n];
	} else {
		for (int k = 0; k < 257; k++) {
			float x = gain * weights[k];
			if (k == 256) x = 0.0f;
			re[k] = x; im[k] = 0.0f;
			if (k & 0xff) { re[512 - k] = x; im[512 - k] = 0.0f; }
		}
		ifft512(re, im);
		for (int n = 0; n < 512; n++) pulse[n] = re[n ^ 0x100];  // centred on sample 256
	}
	float length = options.pulse_length;
	for (int n = 0; n < 512; n++) {
		float d = static_cast<float>(n - 256);
		d = n < 256 ? -d : d;
		float v = 0.0f;
		if (!(d > length)) {
			v = pulse[n];
			float x = d * 0.5f;
			if (d != 0.0f) x = x / length;  // the centre is 0 for any length, including pulse_length 0
			x = cosine_interpolated(x);
			x = x * 0.5f;
			x = x + 0.5f;
			v = v * x;
		}
		pulse[n] = v;
	}
	// sample j of the pulse lands at position + shift + j - 256: in this frame below 160, then the overlap buffer
	for (int i = 0; i < frame.pulse_count; i++) {
		int64_t pos = static_cast<int32_t>(static_cast<uint32_t>(options.pulse_shift) +
		                                   static_cast<uint32_t>(frame.pulse_position[i]));
		float pulse_gain = frame.pulse_gain[i];
		for (int j = 0; j < 512; j++, pos++) {
			float v = pulse[j];
			if (v == 0.0f || pos < 256) continue;
			if (pos < 256 + kFrameSamples) out[pos - 256] = pulse_gain * v + out[pos - 256];
			else if (pos < 256 + 3 * kFrameSamples)
				state.overlap[pos - 416] = pulse_gain * v + state.overlap[pos - 416];
		}
	}
}

}  // namespace

float cosine_interpolated(float x) {
	float s = x * 8192.0f;
	int64_t i = to_fixed64(x, 13) - (s < 0.0f);
	float fraction = s - static_cast<float>(i);
	float t0 = kCos[static_cast<uint64_t>(i) & 0x1fff];
	float t1 = kCos[(static_cast<uint32_t>(i) + 1u) & 0x1fff];
	return t0 + fraction * (t1 - t0);
}

void synth_reset(SynthState& state, const Options& options) {
	state = SynthState{};
	uint32_t r = static_cast<uint32_t>(options.noise_seed) + kSeedOffset;
	state.random = r;
	for (uint32_t i = 0; i < 320; i++) {
		float x = static_cast<float>(i) + 0.5f;
		x = x / 640.0f;
		x = x + -0.25f;
		state.window320[i] = cosine_interpolated(x);
	}
	for (uint32_t i = 0; i < 80; i++) {
		float x = static_cast<float>(i) + 0.5f;
		x = x / 160.0f;
		x = x + -0.25f;
		state.window80[i] = cosine_interpolated(x);
	}
	uint32_t q = 0;
	for (uint64_t j = 0; j < kHarmonicSlots; j++) {
		uint64_t v;
		if (options.initial_phase == 2) {
			v = static_cast<uint64_t>(q * static_cast<uint32_t>(j)) << 32;
		} else if (options.initial_phase == 1) {
			r = next_random(r);
			state.random = r;
			v = static_cast<uint64_t>(r) << 32;
		} else {
			v = 0;
		}
		state.harmonic_phase[j] = v;
		q += 0x9000000u;
	}
}

void synth_comfort_noise(SynthState& state, float level, float* out) {
	uint32_t r = state.random;
	for (int i = 0; i < kFrameSamples; i++) {
		r = next_random(r);
		float u = static_cast<float>(r >> 8) * (1.0f / 8388608.0f);
		u = u + -1.0f;
		out[i] = level * u * kSqrt3;
	}
	state.random = r;
}

void synth_frame(SynthState& state, const Options& options, const SynthFrame& frame, float* out) {
	for (int n = 0; n < kFrameSamples; n++) out[n] = state.overlap[n];
	for (int n = 0; n < kFrameSamples; n++) {
		state.overlap[n] = state.overlap[kFrameSamples + n];
		state.overlap[kFrameSamples + n] = 0.0f;
	}
	if (frame.active == 0 && state.previous.active == 0) {
		state.previous = frame;
		return;
	}
	int32_t lc = frame.harmonic_count, lp = state.previous.harmonic_count;
	int count = lc > lp ? lc : lp;
	const SynthFrame& pitch_from = state.previous.active == 0 ? frame : state.previous;
	uint64_t previous_increment = phase_increment(pitch_from.fundamental);
	if (options.phase_tracking == 1) harmonics_tracked(state, options, frame, out, count, previous_increment);
	else if (count >= 1) harmonics_accumulated(state, options, frame, out, count, previous_increment);
	if (options.noise_method == 1) noise_blocks(state, options, frame, out);
	if (frame.active != 0) {
		if (options.noise_method == 0) noise_single(state, options, frame, out);
		if (frame.pulse_count >= 1) add_pulses(state, options, frame, out);
	}
	for (int n = 0; n < kFrameSamples; n++) {
		if (kFlushBelow > std::fabs(out[n])) out[n] = 0.0f;
	}
	state.previous = frame;
}

}  // namespace halfrate::dec
