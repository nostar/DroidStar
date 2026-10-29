// The parameter dequantiser: turns the nine frame fields into the frame parameters (pitch, harmonic count,
// voicing, log2 harmonic amplitudes, gain), predicting the spectral shape from the previous frame.
//
// The log2 amplitudes of harmonics 1..L are
//   gain - log2(L)/2 + residual[l] + prediction[l] - mean(residual) - mean(prediction)
// where residual is the transformed shape codebook vector (dequant_shape_residual) and prediction is the previous
// frame's amplitudes resampled to L harmonics and scaled (dequant_predict).
//
// Both the decoder and the encoder (which runs the dequantiser to track what the decoder will predict) keep one
// DequantState per stream.
#pragma once
#include <array>
#include <cstdint>

#include "codebook.h"
#include "frame.h"

namespace halfrate::core {

constexpr int kMaxHarmonics = 56;      // largest harmonic count L of any pitch
constexpr int kResetHarmonicCount = 30;  // previous harmonic count after a reset or erasure

// What the dequantiser decided a frame is (dequantize's result and FrameParams::kind).
enum class FrameKind : int32_t {
	voice = 0,     // pitch index 0..119
	tone = 1,      // pitch index 124..125: fixed fundamental 1/32, 14 harmonics, all unvoiced
	erasure = 2,   // pitch index 120..123: the parameters are cleared and the predictor reset
	reserved = 3,  // pitch index 126..127: treated as an erasure
};

// Predictor state: the previous frame's parameters.
struct DequantState {
	int32_t harmonic_count = kResetHarmonicCount;  // L of the previous frame
	float log2_gain = 0.0f;                         // its gain
	// [l] for l = 1..L: its log2 harmonic amplitudes. [0] and the entries above L are 0.
	std::array<float, kMaxHarmonics + 2> log2_amplitude{};
};

// The decoded frame parameters.
struct FrameParams {
	FrameKind kind = FrameKind::voice;
	float fundamental = 0.0f;  // cycles per sample
	float omega = 0.0f;        // fundamental * 2 pi, radians per sample
	int32_t harmonic_count = 0;  // L
	// [k] for harmonic k + 1, k < L: voiced (1) or unvoiced (0), from the voicing band the harmonic falls in.
	// Entries from L on are 0.
	std::array<uint8_t, kMaxHarmonics + 1> harmonic_voiced{};
	// [k] for harmonic k + 1, k < L: log2 amplitude. Entries from L on are 0.
	std::array<float, kMaxHarmonics + 1> log2_amplitude{};
	float log2_gain = 0.0f;
	FrameFields fields{};  // the fields the frame was decoded from, as given
};

// Resets the predictor (as at the start of a stream).
void dequant_reset(DequantState& state);

// The shape residual of L harmonics from the shape fields (3..8): the 8-point coefficient vector (0, shape_low,
// shape_high) through the inverse 8-point DCT, then for each of the 4 blocks a pair sum/difference plus the
// block's higher-order coefficients through the inverse DCT of the block size. out[0..L-1].
void dequant_shape_residual(const Codebook& codebook, int harmonic_count, const FrameFields& fields, float* out);

// The prediction: the state's amplitudes resampled from its harmonic count to L, times codebook.prediction.
// out[0..L-1].
void dequant_predict(const Codebook& codebook, const DequantState& state, int harmonic_count, float* out);

// Decodes one frame: updates the state and, if out is not null, fills *out (every field). Every field index
// must be within its bit width (frame.h); any such set is valid.
FrameKind dequantize(const Codebook& codebook, DequantState& state, const FrameFields& fields, FrameParams* out);

// The same with the codec's codebook (tables::kCodebook).
FrameKind dequantize(DequantState& state, const FrameFields& fields, FrameParams* out);

}  // namespace halfrate::core
