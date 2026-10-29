#include "dequant.h"

#include <cstring>

#include "numeric.h"
#include "tables.h"

namespace halfrate::core {

namespace {
constexpr float kHalfSqrtHalf = 0x1.6a09e6p-2f;  // 1 / (2 sqrt 2)
constexpr float kTwoPi = 0x1.921fb6p+2f;
constexpr int kVoicePitchCount = 120;
constexpr int kTonePitchEnd = 126;
constexpr int kErasurePitchEnd = 124;
constexpr int kToneHarmonicCount = 14;
constexpr float kToneFundamental = 0.03125f;
}  // namespace

void dequant_reset(DequantState& state) { state = DequantState{}; }

void dequant_shape_residual(const Codebook& codebook, int harmonic_count, const FrameFields& fields, float* out) {
	const float* low = codebook.shape_low[fields[kFieldShapeLow]];
	const float* high = codebook.shape_high[fields[kFieldShapeHigh]];
	const float coefficients[8] = {0.0f, low[0], low[1], low[2], high[0], high[1], high[2], high[3]};
	float pairs[8];
	const auto& dct8 = tables::kInverseDct[7];
	for (int r = 0; r < 8; r++) {
		float s = 0.0f;
		for (int c = 0; c < 8; c++) s = s + dct8[r][c] * coefficients[c];
		pairs[r] = s;
	}
	const int32_t* sizes = codebook.band_sizes[harmonic_count];
	int pos = 0;
	for (int block = 0; block < 4; block++) {
		int n = sizes[block];
		float block_coefficients[18];
		std::memset(block_coefficients + 2, 0, 16 * sizeof(float));
		float a = pairs[2 * block], b = pairs[2 * block + 1];
		block_coefficients[0] = (a + b) * 0.5f;
		block_coefficients[1] = (a - b) * kHalfSqrtHalf;
		if (n >= 3) {
			int count = n < 6 ? n : 6;
			int row = codebook.detail_stride[block] * fields[kFieldDetail1 + block];
			std::memcpy(block_coefficients + 2, codebook.detail[block] + row * 4,
			            static_cast<size_t>(count - 2) * sizeof(float));
		} else if (n < 1) {
			continue;
		}
		const auto& dct = tables::kInverseDct[n - 1];
		for (int i = 0; i < n; i++) {
			float s = 0.0f;
			for (int c = 0; c < n; c++) s = s + dct[i][c] * block_coefficients[c];
			out[pos++] = s;
		}
	}
}

void dequant_predict(const Codebook& codebook, const DequantState& state, int harmonic_count, float* out) {
	int previous_count = state.harmonic_count;
	std::array<float, kMaxHarmonics + 2> previous = state.log2_amplitude;
	if (harmonic_count > previous_count) {
		float last = previous[previous_count];
		for (int i = previous_count + 1; i <= harmonic_count; i++) previous[i] = last;
	}
	previous[0] = previous[1];
	if (harmonic_count < 1) return;
	float ratio = static_cast<float>(previous_count) / static_cast<float>(static_cast<unsigned>(harmonic_count));
	float rho = codebook.prediction;
	for (int k = 1; k <= harmonic_count; k++) {
		float x = ratio * static_cast<float>(static_cast<unsigned>(k));
		int i = to_int32_sat(x);
		float frac = x - static_cast<float>(to_int32_sat(x));
		int j = i < kMaxHarmonics ? i : kMaxHarmonics;
		float a = (1.0f - frac) * rho;
		a = a * previous[i];
		float b = frac * rho;
		b = b * previous[j + 1];
		out[k - 1] = a + b;
	}
}

FrameKind dequantize(const Codebook& codebook, DequantState& state, const FrameFields& fields, FrameParams* out) {
	int pitch = fields[kFieldPitch];
	bool has_pitch = pitch < kVoicePitchCount ? true : pitch < codebook.pitch_count;
	FrameKind kind;
	int count = 0;
	float fundamental = 0.0f;
	if (has_pitch) {
		kind = FrameKind::voice;
		fundamental = codebook.fundamental[pitch];
		count = codebook.harmonic_count[pitch];
	} else if (static_cast<unsigned>(pitch) < kErasurePitchEnd) {
		kind = FrameKind::erasure;
	} else if (static_cast<unsigned>(pitch) < kTonePitchEnd) {
		kind = FrameKind::tone;
		count = kToneHarmonicCount;
		fundamental = kToneFundamental;
	} else {
		kind = FrameKind::reserved;
	}
	if (kind == FrameKind::erasure || kind == FrameKind::reserved) {
		dequant_reset(state);
		if (out) {
			*out = FrameParams{};
			out->kind = kind;
			out->fields = fields;
		}
		return kind;
	}

	float new_gain = codebook.gain_weight * codebook.gain[fields[kFieldGain]];
	float carried_gain = codebook.previous_gain_weight * state.log2_gain;
	float residual[60], prediction[58];
	dequant_shape_residual(codebook, count, fields, residual);
	dequant_predict(codebook, state, count, prediction);
	float residual_sum = 0.0f, prediction_sum = 0.0f;
	for (int i = 0; i < count; i++) {
		residual_sum = residual_sum + residual[i];
		prediction_sum = prediction_sum + prediction[i];
	}
	float half_log2_count = tables::kHalfLog2Count[count];
	float reciprocal_count = tables::kReciprocalCount[count];
	float gain = new_gain + carried_gain;
	state.log2_amplitude.fill(0.0f);
	if (count >= 1) {
		float offset = gain - half_log2_count;
		float residual_mean = residual_sum * reciprocal_count;
		offset = offset - residual_mean;
		float prediction_mean = prediction_sum * reciprocal_count;
		for (int k = 0; k < count; k++) {
			float v = residual[k] + prediction[k];
			v = v - prediction_mean;
			v = offset + v;
			state.log2_amplitude[k + 1] = v;
		}
	}
	state.harmonic_count = count;
	state.log2_gain = gain;
	if (!out) return kind;

	out->kind = kind;
	out->fundamental = fundamental;
	out->omega = fundamental * kTwoPi;
	out->harmonic_count = count;
	out->log2_gain = gain;
	out->fields = fields;
	out->harmonic_voiced.fill(0);
	out->log2_amplitude.fill(0.0f);
	const int32_t* bands = codebook.voicing[fields[kFieldVoicing]];
	for (int i = 0; i < count && i <= kMaxHarmonics; i++) {
		int voiced = 0;
		if (has_pitch) {
			int band = to_int32_sat(fundamental * (static_cast<float>(static_cast<unsigned>(i + 1)) * 16.0f));
			voiced = bands[band < 7 ? band : 7];
		}
		out->harmonic_voiced[i] = static_cast<uint8_t>(voiced);
		out->log2_amplitude[i] = state.log2_amplitude[i + 1];
	}
	return kind;
}

FrameKind dequantize(DequantState& state, const FrameFields& fields, FrameParams* out) {
	return dequantize(tables::kCodebook, state, fields, out);
}

}  // namespace halfrate::core
