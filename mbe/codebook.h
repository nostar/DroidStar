// The parameter codebook set: everything the dequantiser (and the encoder's quantiser) looks up to turn the nine
// frame fields into the frame parameters. tables::kCodebook is the one the half-rate codec uses.
#pragma once
#include <array>
#include <cstdint>

namespace halfrate::core {

struct Codebook {
	int32_t codec_id;                  // 1: this half-rate codec
	const float* fundamental;          // [pitch_count] fundamental, cycles per sample, per pitch index
	const int32_t* harmonic_count;     // [pitch_count] number of harmonics L per pitch index
	int32_t voicing_count;             // entries of voicing
	const int32_t (*voicing)[8];       // per voicing index: band 0..7 voiced (nonzero) or unvoiced
	int32_t plain_voicing_count;       // voicing indices below this are plain patterns (not read by the code)
	const int32_t (*band_sizes)[4];    // per L: sizes of the 4 blocks the L harmonics split into
	int32_t gain_count;                // entries of gain
	const float* gain;                 // gain codebook (log2 units)
	const float (*shape_low)[3];       // 8-point transform coefficients 1..3, per shape-low index
	const float (*shape_high)[4];      // 8-point transform coefficients 4..7, per shape-high index
	std::array<const float*, 4> detail;     // per block: higher-order coefficients, rows of 4 floats
	std::array<int32_t, 4> detail_count;    // entries of each detail codebook
	std::array<int32_t, 4> detail_stride;   // rows per index step of each detail codebook
	float prediction;                  // weight of the previous frame's shape in the prediction
	int32_t pitch_count;               // pitch indices with an entry (fundamental, harmonic_count)
	float gain_weight;                 // gain = gain_weight * gain[index] + previous_gain_weight * previous gain
	float previous_gain_weight;
};

}  // namespace halfrate::core
