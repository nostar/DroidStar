#include "encoder.h"

#include <cmath>
#include <cstring>

#include "enhance.h"
#include "fastmath.h"
#include "numeric.h"
#include "constants.h"
#include "enc_tables.h"
#include "tables.h"

namespace halfrate::enc {

namespace {

using core::fast_exp2;
using core::fast_log2;
using core::to_int32_sat;

constexpr int kAllUnvoiced = 16;    // voicing index with no band voiced (indices 0..15 voice at least one)
constexpr int kTonePitch = 124;     // pitch field of a silent frame: the fixed-pitch frame of 14 harmonics
constexpr int kToneHarmonics = 14;
constexpr int kAnalysisCenter = 944;  // window centre for an offset option of 0
constexpr int kNewSamples = kHistorySamples - kFrameSamples;  // first sample of the newest frame
constexpr int kYinMaxLag = 126;
constexpr int kRescoreBands = 18;

// Options that place the analysis windows are clamped so that every read stays inside the history block.
constexpr int kPitchOffsetLow = -781, kPitchOffsetHigh = 3372;
constexpr int kAmplitudeOffsetLow = -864, kAmplitudeOffsetHigh = 3455;

const core::Codebook& codebook() { return tables::kCodebook; }

inline bool is_voiced(int voicing) { return voicing < kAllUnvoiced; }

inline int clamp_int(int v, int low, int high) { return v < low ? low : (v > high ? high : v); }

// The voicing field of silent frames, clamped to the field's 5 bits.
int silent_voicing_index(const EncoderOptions& o) { return clamp_int(to_int32_sat(o.silent_voicing_index), 0, 31); }

float fundamental_of(int pitch) { return tables::kPitchFundamental[pitch]; }
int harmonics_of(int pitch) { return tables::kPitchHarmonicCount[pitch]; }

// The noise floor rises by floor_rise of the level's excess plus a fixed creep per frame and follows the level
// down. The gate is open while the level is above gate_level_db and gate_above_floor_db over the floor, and stays
// open gate_hang_frames frames after. Returns true when the gate is closed (the frame is silent).
bool update_gate(Encoder& e, float level) {
	const EncoderOptions& o = e.options;
	float floor = e.noise_floor_db;
	float up = level - floor;
	up = up < 0.0f ? 0.0f : up;
	floor = floor + o.floor_rise * up;
	floor = floor + kFloorCreep;
	floor = level < floor ? level : floor;
	e.noise_floor_db = floor;
	if (level > o.gate_level_db && level > floor + o.gate_above_floor_db) {
		e.gate_hang = to_int32_sat(o.gate_hang_frames);
		return false;
	}
	int h = e.gate_hang;
	if (h < 1) return true;
	e.gate_hang = h - 1;
	return false;
}

// Starts the noise floor at the lowest level of the first three frames.
void init_floor(Encoder& e) {
	float s0 = e.history.slots[0].level_db;
	e.noise_floor_db = s0;
	for (int j = 1; j < 3; j++) {
		float s1 = e.history.slots[j].level_db;
		if (s1 < s0) {
			e.noise_floor_db = s1;
			s0 = s1;
		}
	}
}

// log2 amplitudes of harmonics 1..n from a harmonic fit: voiced ones (flags[k] set) blend the fitted amplitude
// toward the spectrum's rms, unvoiced ones take the rms; each gets its offset and a per-band offset interpolated
// over 500 Hz steps, unvoiced ones a pitch slope, and harmonics above the high-pass corner region get the
// compensation that undoes the input high-pass.
void harmonic_amplitudes(const Encoder& e, const HarmonicFit& fit, const uint8_t* flags, int n, float* out,
                         float fundamental) {
	if (n < 1) return;
	const EncoderOptions& o = e.options;
	float f32 = fundamental * 32.0f;
	for (int k = 0; k < n; k++) {
		float hz = (fundamental * static_cast<float>(static_cast<unsigned>(k + 1))) * 8000.0f;
		float t = hz * kHzToVoicingBand;
		float v, s;
		if (flags[k]) {
			if (t <= 0.0f)
				v = o.voiced_band_offset[0];
			else if (t >= 8.0f)
				v = o.voiced_band_offset[8];
			else {
				int i = to_int32_sat(t);
				float fraction = t - static_cast<float>(to_int32_sat(t));
				float a0 = o.voiced_band_offset[i], a1 = o.voiced_band_offset[i + 1];
				v = a0 + fraction * (a1 - a0);
			}
			float q = fit.log2_amplitude[k];
			float r = q + o.voiced_amplitude_blend * (fit.log2_rms[k] - q);
			s = (o.voiced_offset + r) + v;
		} else {
			if (t <= 0.0f)
				v = o.unvoiced_band_offset[0];
			else if (t >= 8.0f)
				v = o.unvoiced_band_offset[8];
			else {
				int i = to_int32_sat(t);
				float fraction = t - static_cast<float>(to_int32_sat(t));
				float a0 = o.unvoiced_band_offset[i], a1 = o.unvoiced_band_offset[i + 1];
				v = a0 + fraction * (a1 - a0);
			}
			s = (fit.log2_rms[k] + o.unvoiced_offset) + v;
			s = s + o.unvoiced_pitch_slope * fast_log2(f32);
		}
		out[k] = s;
		float fc = o.highpass_hz, comp = o.highpass_compensation;
		if (fc > 0.0f && comp > 0.0f && hz >= o.highpass_compensation_from_hz) {
			float r = hz / fc;
			float r4 = r * (r * (r * r));
			float c = comp * -0.5f;
			float q = r4 / (r4 + 1.0f);
			q = q + kCompensationFloor;
			out[k] = s + c * fast_log2(q);
		}
	}
}

// The voicing index for a harmonic fit at grid pitch `pitch` (whose fit cost is fit_cost): per band, the cost of
// calling it voiced or unvoiced against a threshold, weighted by the band's energy share; the best of the first
// code_count voicing patterns, with an optional cost for each band that changes from the previous frame.
int choose_voicing(const Encoder& e, const HarmonicFit& fit, int code_count, int pitch, float fit_cost) {
	const EncoderOptions& o = e.options;
	const core::Codebook& cb = codebook();
	int previous = e.previous_voicing;
	int row = 0;
	while (row < 8) {
		if (pitch < tables::kVoicingRowStart[row + 1]) break;
		row++;
	}
	float sum = 0.0f;
	for (int i = 0; i < 8; i++) sum = sum + fit.band_energy[i];
	float pitch_index = static_cast<float>(pitch);
	float total = sum + kEnergyFloor;
	const float* base = tables::kVoicingThresholdBase + 8 * row;
	const float* slope = tables::kVoicingThresholdSlope + 8 * row;
	float voiced_cost[8], unvoiced_cost[8], weight[8];
	for (int k = 0; k < 8; k++) {
		float fk = static_cast<float>(static_cast<unsigned>(k));
		float t;
		if (o.voicing_table > 0.0f)
			t = o.voicing_table_offset + (base[k] + fit_cost * slope[k]);
		else
			t = o.voicing_threshold + ((fk + -3.5f) * o.voicing_threshold_slope) * kTwoSevenths;
		float strength = o.low_pitch_voicing_strength;
		if (strength > 0.0f && fit_cost < o.low_pitch_voicing_fit && o.low_pitch_voicing_index > pitch_index) {
			float d = strength * (o.low_pitch_voicing_fit - fit_cost);
			t = t + (fk * kOneSeventh) * d;
		}
		bool has_data = fit.band_has_data[k] != 0;
		float v = has_data ? fit.band_ratio[k] : t;
		voiced_cost[k] = v > t ? (v - t) + kVoicingCostFloor : kVoicingCostFloor;
		unvoiced_cost[k] = t > v ? t - v : 0.0f;
		float w = 0.0f;
		if (has_data) {
			if (k == 0 && e.low_band && o.low_band_ignore_first > 0.0f) {
				w = 0.0f;
			} else {
				float exponent = o.voicing_band_weight_exponent;
				w = 1.0f;
				if (!(exponent == 0.0f)) {
					float r = fit.band_energy[k] / total;
					w = 0.0f;
					if (r > 0.0f) w = fast_exp2(exponent * fast_log2(r));
				}
			}
		}
		weight[k] = w;
	}
	float weight_sum = 0.0f;
	for (int k = 0; k < 8; k++) weight_sum = weight_sum + weight[k];
	if (weight_sum <= 0.0f && code_count >= 17) return kAllUnvoiced;
	float best = kHuge;
	int result = 0;
	for (int i = 0; i < code_count; i++) {
		const int32_t* c = cb.voicing[i];
		float acc = 0.0f;
		for (int b = 0; b < 8; b++) acc = acc + (c[b] == 0 ? unvoiced_cost[b] : voiced_cost[b]) * weight[b];
		float penalty = o.voicing_hysteresis;
		if (previous >= 0 && penalty > 0.0f) {
			const int32_t* p = cb.voicing[previous];
			for (int b = 0; b < 8; b++) acc = c[b] == p[b] ? acc : penalty + acc;
		}
		if (acc < best) {
			best = acc;
			result = i;
		}
	}
	return result;
}

// Voicing of harmonics 1..n from the 500 Hz bands of voicing pattern `voicing`.
void voicing_flags(int voicing, int n, float fundamental, uint8_t* flags) {
	const int32_t* row = codebook().voicing[voicing];
	for (int i = 0; i < n; i++) {
		float v = static_cast<float>(static_cast<uint32_t>(i + 1)) * 16.0f;
		int b = to_int32_sat(fundamental * v);
		b = b < 7 ? b : 7;
		flags[i] = static_cast<uint8_t>(row[b]);
	}
}

// The rescoring input: each harmonic's band and weight (the window weight energy, scaled down by the amplitude
// offset the harmonic got), and the reference band energies from the amplitudes or from the amplitude spectrum.
void band_energies(const Encoder& e, RescoreInput& out, const float* amps, int n, const uint8_t* flags,
                   const float* weights, const AnalysisSlot& slot, float fundamental) {
	const EncoderOptions& o = e.options;
	out.band_energy.fill(0.0f);
	const float* edge = tables::kRescoreBandEdge;
	for (int k = 0; k < n; k++) {
		float offset = (flags && flags[k]) ? o.voiced_offset : o.unvoiced_offset;
		float hz = (fundamental * static_cast<float>(static_cast<unsigned>(k + 1))) * 8000.0f;
		int j = 0;
		while (j < 17) {
			if (!(hz >= edge[j + 1])) break;
			j++;
		}
		out.band[k] = j;
		out.weight[k] = weights[k] * fast_exp2(offset * -2.0f);
		if (o.rescore_from_spectrum == 0.0f) {
			float b = out.band_energy[j];
			float g = fast_exp2(amps[k] + amps[k]);
			out.band_energy[j] = b + g * out.weight[k];
		}
	}
	if (!(o.rescore_from_spectrum == 0.0f)) {
		const Spectrum& s = slot.amplitude_spectrum;
		for (int i = 1; i < kSpectrumBins; i++) {
			float hz = static_cast<float>(static_cast<unsigned>(i)) * 15.625f;
			int j = 0;
			while (j < 17) {
				if (!(hz >= edge[j + 1])) break;
				j++;
			}
			float b = out.band_energy[j];
			b = b + s.re[i] * s.re[i];
			b = b + s.im[i] * s.im[i];
			out.band_energy[j] = b;
		}
	}
}

// Chooses the gain and shape fields for the target log2 amplitudes amps[L], fills fields and runs the
// dequantiser on them. The gain is the nearest to the target mean, optionally looking two frames ahead; the shape
// is searched on the target minus the prediction, with optional rescoring of the best candidates (and the gains
// around the chosen one) by band energies or by the enhanced decoded amplitudes.
void quantize(Encoder& e, int pitch_field, int voicing_field, const float* amps, int L, const RescoreInput* rescore,
              core::FrameFields& fields) {
	const EncoderOptions& o = e.options;
	const core::Codebook& cb = codebook();
	core::DequantState& state = e.predictor;
	float prediction[kMaxFitHarmonics], target[kMaxFitHarmonics], weight[kMaxFitHarmonics];
	float recon[kMaxFitHarmonics], decoded[kMaxFitHarmonics], enhanced[kMaxFitHarmonics + 2], coef[kBasisRows];
	ShapeCandidate list[kMaxShapeResults];
	core::FrameParams frame{};
	core::dequant_predict(cb, state, L, prediction);

	float sum = 0.0f, sum_prediction = 0.0f, peak = kVeryNegative;
	for (int i = 0; i < L; i++) {
		sum = sum + amps[i];
		sum_prediction = sum_prediction + prediction[i];
		if (amps[i] > peak) peak = amps[i];
	}
	float reciprocal = tables::kReciprocalCount[L];
	float mean = sum * reciprocal;
	float bias = tables::kHalfLog2Count[L];
	float gain_target = mean + bias;
	fields = core::FrameFields{};
	fields[core::kFieldPitch] = pitch_field;
	fields[core::kFieldVoicing] = voicing_field;
	fields[core::kFieldGain] = 0;

	const float gain_bias = o.gain_bias;
	const int gain_count = cb.gain_count;
	const float* gain = cb.gain;
	int chosen_gain = 0;
	if (gain_count >= 1) {
		float w = cb.gain_weight, p = cb.previous_gain_weight * state.log2_gain;
		float best = kHuge;
		for (int k = 0; k < gain_count; k++) {
			float v = (((w * gain[k]) + p) - gain_target) - gain_bias;
			v = v < 0.0f ? -v : v;
			if (v < best) {
				fields[core::kFieldGain] = k;
				chosen_gain = k;
				best = v;
			}
		}
	}
	float lookahead_weight = o.gain_lookahead_weight;
	int lookahead = e.lookahead_count;
	if (lookahead_weight > 0.0f && lookahead >= 1) {
		float best = kHuge;
		for (int g = chosen_gain - 3, high = chosen_gain + 3;; g++) {
			if (g >= 0 && g < gain_count) {
				float w = cb.gain_weight, pw = cb.previous_gain_weight;
				float level = (w * gain[g]) + (pw * state.log2_gain);
				float c = (level - gain_target) - gain_bias;
				c = c * c;
				for (int m = 0; m < lookahead; m++) {
					float carried = pw * level, next_target = e.lookahead_gain[m], nearest = kHuge;
					for (int k = 0; k < gain_count; k++) {
						float candidate = carried + (w * gain[k]);
						float d = (candidate - next_target) - gain_bias;
						d = d < 0.0f ? -d : d;
						if (d < nearest) {
							nearest = d;
							level = candidate;
						}
					}
					c = c + (nearest * (lookahead_weight * nearest));
				}
				if (c < best) {
					best = c;
					chosen_gain = g;
				}
			}
			if (!(g < high)) break;
		}
		fields[core::kFieldGain] = chosen_gain;
	}

	float mean_prediction = sum_prediction * reciprocal;
	if (L >= 1) {
		float fL = static_cast<float>(static_cast<uint32_t>(L)), weight_sum = 0.0f;
		for (int i = 0; i < L; i++) {
			float a = amps[i];
			target[i] = (a - mean) - (prediction[i] - mean_prediction);
			float w = 1.0f;
			if (o.shape_weight_exponent > 0.0f) w = fast_exp2((a - peak) * o.shape_weight_exponent);
			weight[i] = w;
			float high = o.shape_high_weight;
			if (high != 0.0f) {
				float r = static_cast<float>(static_cast<uint32_t>(i + 1)) / fL;
				if (r > kHighShapeStart) {
					w = w * fast_exp2(((r + kMinusHighShapeStart) * high) * kHighShapeScale);
					weight[i] = w;
				}
			}
			weight_sum = weight_sum + w;
		}
		// every weight can underflow to 0 with a large shape_weight_exponent; they stay 0 then
		float scale = weight_sum != 0.0f ? fL / weight_sum : 1.0f;
		for (int i = 0; i < L; i++) weight[i] = scale * weight[i];
	}

	int candidates = 1;
	if (rescore) {
		int v = to_int32_sat(o.rescore_candidates);
		if (v >= 1 && rescore->enabled != 0) candidates = v;
	}
	if (static_cast<uint32_t>(candidates) >= 16) candidates = 16;
	int beam = to_int32_sat(o.shape_search_beam);
	int results = search_shape(e.shape_basis, cb, L, target, weight, beam, beam, list, candidates);
	fields[core::kFieldShapeLow] = results >= 1 ? list[0].low : 0;
	fields[core::kFieldShapeHigh] = results >= 1 ? list[0].high : 0;
	for (int b = 0; b < 4; b++) fields[core::kFieldDetail1 + b] = results >= 1 ? list[0].detail[b] : 0;

	if (rescore && rescore->enabled != 0 && results >= 1) {
		const float* rows = e.shape_basis.rows(L);
		int span = to_int32_sat(o.rescore_gain_span);
		span = span < gain_count ? span : gain_count;  // a wider span reaches no further gain index
		int center = fields[core::kFieldGain];
		float denominator = list[0].distortion + kRescoreDistanceFloor;
		float best = kHuge;
		int best_gain = center, best_candidate = 0;
		const float* reference = rescore->band_energy.data();
		for (int c = 0; c < results; c++) {
			const ShapeCandidate& candidate = list[c];
			const float* low = cb.shape_low[candidate.low];
			const float* high = cb.shape_high[candidate.high];
			for (int k = 0; k < 3; k++) coef[k] = low[k];
			for (int k = 0; k < 4; k++) coef[3 + k] = high[k];
			for (int b = 0; b < 4; b++) {
				const float* detail = cb.detail[b] + static_cast<long>(cb.detail_stride[b] * candidate.detail[b]) * 4;
				for (int k = 0; k < 4; k++) coef[7 + 4 * b + k] = detail[k];
			}
			for (int i = 0; i < L; i++) recon[i] = 0.0f;
			for (int r = 0; r < kBasisRows; r++)
				for (int i = 0; i < L; i++) recon[i] = recon[i] + coef[r] * rows[static_cast<long>(r) * L + i];
			float error = 0.0f;
			for (int i = 0; i < L; i++) {
				float d = target[i] - recon[i];
				error = error + d * (weight[i] * d);
			}
			if (span < 0) continue;
			float normalised_error = error / denominator;
			for (int g = center - span, high_g = center + span;; g++) {
				if (g >= 0 && g < gain_count) {
					float level = (cb.gain_weight * gain[g]) + (cb.previous_gain_weight * state.log2_gain);
					for (int i = 0; i < L; i++)
						decoded[i] = (level - bias) + (recon[i] + (prediction[i] - mean_prediction));
					float gain_error = (level - gain_target) - gain_bias;
					float score;
					if (o.rescore_enhanced > 0.0f) {
						frame.harmonic_count = L;
						float f = pitch_field <= 119 ? cb.fundamental[pitch_field] : kToneFundamental;
						frame.fundamental = f;
						frame.omega = f * kTwoPi;
						for (int i = 0; i < L; i++) frame.log2_amplitude[i] = decoded[i];
						core::shape_log2(frame, o.rescore_enhanced > 1.5f, enhanced);
						float s = 0.0f;
						if (L > 0) {
							float a6 = o.gain_bias, a7 = o.rescore_enhanced_offset;
							for (int i = 0; i < L; i++) {
								float d = enhanced[1 + i] - ((amps[i] + a6) + a7);
								s = s + d * (weight[i] * d);
							}
						}
						score = (reciprocal * s) + (gain_error * (gain_error * o.rescore_gain_weight));
					} else {
						float histogram[kRescoreBands] = {};
						for (int i = 0; i < L; i++) {
							int band = rescore->band[i];
							float h = histogram[band];
							float d = decoded[i] - o.gain_bias;
							float v = fast_exp2(d + d) * rescore->weight[i];
							histogram[band] = h + v;
						}
						float top = 0.0f;
						for (int b = 0; b < kRescoreBands; b++)
							if (reference[b] > top) top = reference[b];
						float floor = (top * fast_exp2(-o.rescore_floor)) + kRescoreBandFloor;
						float total = top + floor;
						float acc = 0.0f;
						for (int b = 0; b < kRescoreBands; b++) {
							if (reference[b] <= 0.0f && histogram[b] <= 0.0f) continue;  // NaN goes on
							float coded = fast_log2(floor + histogram[b]);
							float wanted = fast_log2(floor + reference[b]);
							float w = 1.0f;
							if (o.rescore_weight_exponent > 0.0f)
								w = fast_exp2(o.rescore_weight_exponent * fast_log2((floor + reference[b]) / total));
							float dl = coded - wanted, wf = 1.0f;
							if (dl > 0.0f) wf = o.rescore_asymmetry;
							acc = acc + ((dl * (dl * w)) * wf);
						}
						score = (acc + (normalised_error * o.rescore_shape_mix)) +
						        (gain_error * (gain_error * o.rescore_gain_weight));
					}
					if (score < best) {
						best_gain = g;
						best_candidate = c;
						best = score;
					}
				}
				if (!(g < high_g)) break;
			}
		}
		const ShapeCandidate& chosen = list[best_candidate];
		fields[core::kFieldShapeLow] = chosen.low;
		fields[core::kFieldShapeHigh] = chosen.high;
		for (int b = 0; b < 4; b++) fields[core::kFieldDetail1 + b] = chosen.detail[b];
		fields[core::kFieldGain] = best_gain;
	}
	core::dequantize(cb, state, fields, nullptr);
}

// The input frame: scaled to +-1, through the high-pass when its corner is above 0.
void filter_input(Encoder& e, const int16_t* pcm, float* x) {
	HighPass& hp = e.highpass;
	float corner = e.options.highpass_hz;
	if (corner > 0.0f) {
		if (!(hp.designed_hz == corner)) {
			int w8 = to_int32_sat((corner * 8192.0f) / 8000.0f + 0.5f);
			float c = tables::kCosine[static_cast<uint32_t>(w8) & 0x1fff];
			float alpha = tables::kCosine[(static_cast<uint32_t>(w8) + 0x1800u) & 0x1fff];
			alpha = alpha * kInvSqrt2;
			float denominator = alpha + 1.0f;
			float one_plus_cos = c + 1.0f;
			float half = one_plus_cos * 0.5f;
			float minus = -one_plus_cos;
			float a1 = c * -2.0f;
			hp.b0 = half / denominator;
			hp.b1 = minus / denominator;
			hp.b2 = half / denominator;
			hp.a1 = a1 / denominator;
			hp.a2 = (1.0f - alpha) / denominator;
			hp.designed_hz = corner;
		}
		for (int i = 0; i < kFrameSamples; i++) {
			float in = static_cast<float>(pcm[i]) * 0x1p-15f;
			float y = hp.b0 * in;
			y = y + hp.b1 * hp.x1;
			y = y + hp.b2 * hp.x2;
			y = y - hp.a1 * hp.y1;
			y = y - hp.a2 * hp.y2;
			float x1 = hp.x1, y1 = hp.y1;
			hp.x2 = x1;
			hp.x1 = in;
			hp.y1 = (kHighPassFlush > std::fabs(y)) ? 0.0f : y;
			hp.y2 = (kHighPassFlush > std::fabs(y1)) ? 0.0f : y1;
			x[i] = y;
		}
	} else {
		for (int i = 0; i < kFrameSamples; i++) x[i] = static_cast<float>(pcm[i]) * 0x1p-15f;
	}
}

void update_transitions(Encoder& e) {
	const EncoderOptions& o = e.options;
	const float* lf = e.log2_fundamental.data();
	for (int r = 0; r < kPitchCount; r++) {
		float* row = &e.transition[static_cast<size_t>(r) * kPitchCount];
		for (int c = 0; c < kPitchCount; c++) {
			float d = lf[r] - lf[c];
			float ad = d < 0.0f ? -d : d;
			float s1 = (ad + -1.0f) * kOctaveJumpScale;
			float octave = 0.0f, jump = 0.0f;
			if (std::fabs(s1) < 3.0f) octave = o.octave_jump_cost * fast_exp2(s1 * (s1 * kOctaveJumpExponent));
			float f = ad - o.pitch_jump_free;
			if (f > 0.0f) jump = f * o.pitch_jump_cost;
			row[c] = octave + jump;
		}
	}
}

// Analyses the newest frame (already in the history) into its slot and pushes its pitch costs to the tracker.
void analyse_frame(Encoder& e, int64_t count) {
	const EncoderOptions& o = e.options;
	SignalHistory& history = e.history;
	AnalysisSlot& slot = history.slots[static_cast<size_t>(count % 3)];
	int pitch_offset = clamp_int(to_int32_sat(o.pitch_window_offset), kPitchOffsetLow, kPitchOffsetHigh);
	int amplitude_offset =
	    clamp_int(to_int32_sat(o.amplitude_window_offset), kAmplitudeOffsetLow, kAmplitudeOffsetHigh);
	// The writes into the slot happen in this order: with positive offsets the later reads see the earlier writes.
	analysis_spectrum(e.long_window, history, pitch_offset + kAnalysisCenter, slot.pitch_spectrum);
	analysis_spectrum(e.short_window, history, amplitude_offset + kAnalysisCenter, slot.amplitude_spectrum);
	slot.peak = 0.0f;
	{
		int i0 = amplitude_offset + kNewSamples, i1 = amplitude_offset + kHistorySamples - 1;
		float top = 0.0f;
		for (int i = i0; i <= i1; i++) {
			float s = history.sample(i);
			if (s < 0.0f) s = -s;
			if (s > top) {
				slot.peak = s;
				top = s;
			}
		}
	}
	float energy;
	harmonic_fit_cost(e.long_window, slot.pitch_spectrum, e.fit_cost.data(), &energy);
	slot.fit_cost = e.fit_cost;
	slot.level_db = fast_log2(energy + kEnergyFloor) * kDbPerLog2;
	yin_difference(history, pitch_offset + kAnalysisCenter, e.inverse_fundamental.data(), kPitchCount, kYinMaxLag,
	               e.yin_cost.data());
	float low_pitch_penalty = o.low_pitch_penalty;
	if (o.low_band_threshold_db < 0.0f) {
		const Spectrum& s = slot.pitch_spectrum;
		float a = kTiny;
		for (int i = 2; i < 17; i++) {
			a = a + s.re[i] * s.re[i];
			a = a + s.im[i] * s.im[i];
		}
		float b = kTiny;
		for (int i = 22; i < 65; i++) {
			b = b + s.re[i] * s.re[i];
			b = b + s.im[i] * s.im[i];
		}
		float t = fast_log2(a / b) * kDbPerLog2;
		int low = t < o.low_band_threshold_db;
		slot.low_band = low;
		if (low) low_pitch_penalty = low_pitch_penalty * o.low_band_penalty_scale;
	}
	if (o.onset_db > 0.0f && count >= 2) {
		float d = slot.level_db - history.slots[static_cast<size_t>((count + 1) % 3)].level_db;
		if (d > o.onset_db) low_pitch_penalty = low_pitch_penalty * o.onset_low_pitch_scale;
	}
	for (int i = 0; i < kPitchCount; i++)
		e.pitch_cost[i] = (e.fit_cost[i] + e.yin_cost[i] * o.yin_weight) + e.log2_ratio_to_lowest[i] * low_pitch_penalty;

	// pitch network features: 120 fit costs, 120 YIN values, 64 band log energies (relative to the largest), level
	std::array<float, kNetFeatures>& features = e.features[static_cast<size_t>(count % 6)];
	std::memcpy(&features[0], e.fit_cost.data(), sizeof(float) * kPitchCount);
	std::memcpy(&features[kPitchCount], e.yin_cost.data(), sizeof(float) * kPitchCount);
	{
		float* band = &features[2 * kPitchCount];
		float top = kVeryNegative;
		const Spectrum& s = slot.pitch_spectrum;
		for (int k = 0; k < 64; k++) {
			float sum = kTiny;
			for (int j = 0; j < 4; j++) {
				sum = sum + s.re[4 * k + j] * s.re[4 * k + j];
				sum = sum + s.im[4 * k + j] * s.im[4 * k + j];
			}
			float v = fast_log2(sum);
			band[k] = v;
			top = v > top ? v : top;
		}
		for (int k = 0; k < 64; k++) band[k] = band[k] - top;
		features[kNetFeatures - 1] = slot.level_db * kLevelFeatureScale;
	}
	if (o.pitch_net_weight > 0.0f) {
		for (int i = 0; i < kNetFrames; i++) {
			int64_t m = count - 5 + i;
			float* in = &e.net_input[static_cast<size_t>(i) * kNetFeatures];
			if (m < 0)
				std::memset(in, 0, sizeof(float) * kNetFeatures);
			else
				std::memcpy(in, e.features[static_cast<size_t>(m % 6)].data(), sizeof(float) * kNetFeatures);
		}
		pitch_net_forward(e.net_input.data(), e.net_output.data());
		float w = o.pitch_net_weight;
		for (int i = 0; i < kPitchCount; i++) e.pitch_cost[i] = e.pitch_cost[i] - e.net_output[i] * w;
	}
	if (e.transitions_dirty) {
		update_transitions(e);
		e.transitions_dirty = false;
	}
	tracker_push(e.tracker, e.pitch_cost.data(), e.transition.data());
	history.frame_count = count + 1;
}

// The expected gain target of the frame `ahead` (1 or 2) frames after `frame`, for the gain look-ahead.
float lookahead_target(Encoder& e, int64_t frame, int ahead) {
	const EncoderOptions& o = e.options;
	const AnalysisSlot& slot = e.history.slots[static_cast<size_t>((ahead + frame) % 3)];
	int pitch = tracker_decide(e.tracker, 2 - ahead);
	float level = slot.level_db;
	if ((level > o.gate_level_db && level > e.noise_floor_db + o.gate_above_floor_db) || !(ahead >= e.gate_hang)) {
		HarmonicFit fit;
		uint8_t flags[kMaxFitHarmonics];
		float amps[kMaxFitHarmonics];
		fit_harmonics(e.long_window, slot.pitch_spectrum, pitch, harmonics_of(pitch), fundamental_of(pitch), fit);
		e.low_band = slot.low_band;
		const float cost = slot.fit_cost[pitch];
		int voicing = choose_voicing(e, fit, 32, pitch, cost);
		if (!is_voiced(voicing) && cost < o.unvoiced_fit_guard) voicing = choose_voicing(e, fit, 16, pitch, cost);
		float f0 = fundamental_of(pitch);
		int n = harmonics_of(pitch);
		fit_harmonics(e.short_window, slot.amplitude_spectrum, pitch, harmonics_of(pitch), fundamental_of(pitch), fit);
		float s = 0.0f;
		if (n >= 1) {
			voicing_flags(voicing, n, f0, flags);
			harmonic_amplitudes(e, fit, flags, n, amps, f0);
			for (int i = 0; i < n; i++) s = s + amps[i];
		}
		return s * tables::kReciprocalCount[n] + tables::kHalfLog2Count[n];
	}
	float amps[kSilenceBands];
	silence_amplitudes(e.short_window, slot.amplitude_spectrum, amps);
	float s = 0.0f;
	for (int i = 0; i < kSilenceBands; i++) s = o.silence_offset + (o.unvoiced_offset + (s + amps[i]));
	return tables::kHalfLog2Count[kToneHarmonics] + tables::kReciprocalCount[kToneHarmonics] * s;
}

}  // namespace

void encoder_reset(Encoder& e) {
	e.history.samples.fill(0.0f);
	e.history.frame_count = 0;
	for (AnalysisSlot& slot : e.history.slots) std::memset(&slot, 0, sizeof slot);
	tracker_reset(e.tracker, kPitchCount, kEncoderDelay);
	e.noise_floor_db = 0.0f;
	e.gate_hang = 0;
	core::dequant_reset(e.predictor);
	e.lookahead_gain.fill(0.0f);
	e.lookahead_count = 0;
	e.previous_amplitudes.fill(0.0f);
	e.previous_fundamental = kInitialPreviousFundamental;
	e.previous_harmonic_count = 0;
	e.have_previous = 0;
	e.previous_voicing = -1;
	e.after_silence = 0;
	e.low_band = 0;
	e.previous_unvoiced = 1;
	e.highpass.x1 = e.highpass.x2 = e.highpass.y1 = e.highpass.y2 = 0.0f;
	e.highpass.designed_hz = -1.0f;
	for (auto& f : e.features) f.fill(0.0f);
	e.net_input.fill(0.0f);
	e.net_output.fill(0.0f);
	e.transitions_dirty = true;
}

void encoder_init(Encoder& e) {
	for (int i = 0; i < kPitchCount; i++) {
		float f0 = fundamental_of(i);
		e.inverse_fundamental[i] = 1.0f / f0;
		e.log2_fundamental[i] = fast_log2(f0);
	}
	for (int i = 0; i < kPitchCount; i++) e.log2_ratio_to_lowest[i] = e.log2_fundamental[0] - e.log2_fundamental[i];
	for (int i = 0; i < kPitchCount; i++) {
		float t = e.log2_fundamental[i] + -1.0f;
		int best = -1;
		float m = kOctaveMatch;
		for (int k = 0; k < kPitchCount; k++) {
			float d = e.log2_fundamental[k] - t;
			d = d < 0.0f ? -d : d;
			if (d < m) {
				best = k;
				m = d;
			}
		}
		e.octave_below[i] = best;
	}
	init_analysis_window(e.long_window, tables::kLongWindow, 256);
	init_analysis_window(e.short_window, tables::kShortWindow, 160);
	init_shape_basis(e.shape_basis, codebook());
	encoder_reset(e);
}

void encode_frame(Encoder& e, const int16_t* pcm, core::FrameFields& fields) {
	const EncoderOptions& o = e.options;
	SignalHistory& history = e.history;
	std::memmove(&history.samples[0], &history.samples[kFrameSamples], sizeof(float) * kNewSamples);
	filter_input(e, pcm, &history.samples[kNewSamples]);

	int64_t count = history.frame_count;
	analyse_frame(e, count);

	float amps[kMaxFitHarmonics];
	uint8_t flags[kMaxFitHarmonics] = {};
	int64_t frame = count - kEncoderDelay;
	if (frame < 0) {
		// the frames before the first: silent, at a fixed low level
		e.lookahead_count = 0;
		for (float& a : amps) a = kStartupAmplitude;
		quantize(e, kTonePitch, silent_voicing_index(o), amps, kToneHarmonics, nullptr, fields);
		return;
	}
	int pitch = tracker_decide(e.tracker, 2);
	// take the octave below when its fit is nearly as good
	if (o.octave_down_tolerance > 0.0f) {
		int below = e.octave_below[pitch];
		float divisor = e.previous_unvoiced == 0 ? 1.0f : o.octave_down_after_unvoiced;
		if (below >= 0) {
			float s0 = o.octave_down_tolerance / divisor;
			const std::array<float, kPitchCount>& cost = history.slots[static_cast<size_t>(frame % 3)].fit_cost;
			float s1 = cost[below];
			s0 = cost[pitch] - s0;
			if (!(s1 > s0 || s1 != s1 || s0 != s0)) pitch = below;
		}
	}
	AnalysisSlot& slot = history.slots[static_cast<size_t>(frame % 3)];
	// after an unvoiced frame, take the octave below when one of the later frames is already there
	if (o.octave_down_lookback > 0.0f && e.previous_unvoiced != 0) {
		int below = e.octave_below[pitch];
		if (below >= 0) {
			for (int lag = 1; lag != -1; lag--) {
				int r = tracker_decide(e.tracker, lag);
				if (std::fabs(e.log2_fundamental[r] - e.log2_fundamental[below]) < kOctaveLookbackTolerance) {
					pitch = below;
					break;
				}
			}
		}
	}
	if (frame == 0) init_floor(e);
	bool gate = update_gate(e, slot.level_db);
	e.lookahead_count = 0;
	if (o.gain_lookahead_weight > 0.0f) {
		for (int ahead = 1; ahead != 3; ahead++) e.lookahead_gain[ahead - 1] = lookahead_target(e, frame, ahead);
		e.lookahead_count = 2;
	}

	// voicing at the decided pitch
	HarmonicFit fit;
	fit_harmonics(e.long_window, slot.pitch_spectrum, pitch, harmonics_of(pitch), fundamental_of(pitch), fit);
	e.low_band = slot.low_band;
	const float cost = slot.fit_cost[pitch];
	int voicing = choose_voicing(e, fit, 32, pitch, cost);
	if (!is_voiced(voicing) && cost < o.unvoiced_fit_guard) voicing = choose_voicing(e, fit, 16, pitch, cost);

	RescoreInput rescore;
	rescore.enabled = o.rescore_candidates > 1.0f ? 1 : 0;
	if (gate && !(o.gated_as_unvoiced > 0.0f)) {
		// silent frame
		silence_amplitudes(e.short_window, slot.amplitude_spectrum, amps);
		for (int i = 0; i < kSilenceBands; i++) amps[i] = o.silence_offset + (o.unvoiced_offset + amps[i]);
		e.have_previous = 0;
		if (rescore.enabled)
			band_energies(e, rescore, amps, kSilenceBands, nullptr, e.short_window.silence_norm.data(), slot,
			              kToneFundamental);
		e.previous_voicing = -1;
		e.after_silence = 1;
		e.previous_unvoiced = 1;
		quantize(e, kTonePitch, silent_voicing_index(o), amps, kToneHarmonics, &rescore, fields);
		return;
	}
	if (gate) voicing = kAllUnvoiced;
	int after_silence = e.after_silence;
	e.after_silence = 0;
	if (after_silence && o.onset_unvoiced > 0.0f) voicing = kAllUnvoiced;
	if (!is_voiced(voicing)) {
		float s0 = o.unvoiced_pitch_index;
		int v = to_int32_sat(s0);
		if (!(s0 < 0.0f || s0 != s0)) pitch = v < kPitchCount ? v : kPitchCount - 1;
	}
	int n = harmonics_of(pitch);
	float f0 = fundamental_of(pitch);
	fit_harmonics(e.short_window, slot.amplitude_spectrum, pitch, n, f0, fit);
	if (n > 0) {
		voicing_flags(voicing, n, f0, flags);
		harmonic_amplitudes(e, fit, flags, n, amps, f0);
		if (gate)
			for (int i = 0; i < n; i++) amps[i] = o.gated_offset + amps[i];
		if (after_silence)
			for (int i = 0; i < n; i++) amps[i] = o.onset_offset + amps[i];
	}
	// smoothing with the previous frame and the next one
	float next_weight = o.smooth_next;
	if (!(next_weight == 0.0f) || !(o.smooth_previous == 0.0f)) {
		float previous_weight = e.have_previous ? o.smooth_previous : 0.0f;
		float next[kMaxFitHarmonics];
		if (!(next_weight == 0.0f)) {
			const AnalysisSlot& following = history.slots[static_cast<size_t>((count - 1) % 3)];
			HarmonicFit next_fit;
			fit_harmonics(e.short_window, following.amplitude_spectrum, pitch, n, f0, next_fit);
			harmonic_amplitudes(e, next_fit, flags, n, next, f0);
		}
		if (n >= 1) {
			float own = (1.0f - previous_weight) - next_weight;
			const float* prev = e.previous_amplitudes.data();
			for (int i = 0; i < n; i++) {
				float a = amps[i];
				float s3 = own * a;
				if (!(previous_weight == 0.0f)) {
					float pos = (f0 * static_cast<float>(static_cast<uint32_t>(i + 1))) / e.previous_fundamental;
					pos = pos + -1.0f;
					pos = pos < 0.0f ? 0.0f : pos;
					int32_t pn = e.previous_harmonic_count;
					float last = static_cast<float>(pn - 1);
					pos = pos > last ? last : pos;
					int k = to_int32_sat(pos);
					float p;
					if (k + 1 < pn) {
						float p0 = prev[k];
						p = p0 + (pos - static_cast<float>(k)) * (prev[k + 1] - p0);
					} else {
						p = prev[k];
					}
					s3 = s3 + previous_weight * p;
				}
				if (!(next_weight == 0.0f)) {
					float b = next[i];
					a = a - o.smooth_next_margin;
					a = b > a ? b : a;
					s3 = s3 + next_weight * a;
				}
				amps[i] = s3;
			}
		}
	}
	if (n >= 1) std::memcpy(e.previous_amplitudes.data(), amps, sizeof(float) * static_cast<size_t>(n));
	e.previous_fundamental = f0;
	e.previous_harmonic_count = n;
	e.have_previous = 1;
	if (n >= 1 && slot.peak >= o.peak_threshold)
		for (int i = 0; i < n; i++) amps[i] = amps[i] - o.peak_reduction;
	if (rescore.enabled) {
		int first = e.short_window.first_harmonic[pitch];
		band_energies(e, rescore, amps, n, flags, &e.short_window.weight_energy[first], slot, f0);
	}
	bool unvoiced = !is_voiced(voicing);
	int keep;
	if (unvoiced)
		keep = (!gate && o.voicing_hold_unvoiced > 0.0f) ? voicing : -1;
	else
		keep = gate ? -1 : voicing;
	e.previous_voicing = keep;
	e.previous_unvoiced = (gate || unvoiced) ? 1 : 0;
	quantize(e, pitch, voicing, amps, n, &rescore, fields);
}

}  // namespace halfrate::enc
