// The spectral shape quantiser: for each harmonic count L, the 23 basis vectors (each of L values) through which
// the shape fields act on the harmonic amplitudes, and the weighted three-stage tree search over the shape
// codebooks.
//
// Basis rows 0..2 belong to the shape-low coefficients, 3..6 to the shape-high ones (the 8-point transform's
// coefficients 1..7, spread over the four blocks as each block's pair sum and difference), and rows 7 + 4 b + r
// to coefficient r of block b's higher-order (detail) codebook. Every row has its mean removed.
#pragma once
#include <array>
#include <cstdint>

#include "codebook.h"

namespace halfrate::enc {

constexpr int kBasisRows = 23;
constexpr int kMinBasisHarmonics = 9;
constexpr int kMaxBasisHarmonics = 56;
constexpr int kBasisValues = kBasisRows * 1560;  // 23 rows of L values for L = 9..56
constexpr int kMaxShapeResults = 16;

struct ShapeBasis {
	std::array<int32_t, kMaxBasisHarmonics + 1> offset{};  // first value of L's rows in values
	std::array<float, kBasisValues> values{};
	const float* rows(int harmonic_count) const { return &values[offset[harmonic_count]]; }
};

// One search result: the six shape fields and the weighted distortion.
struct ShapeCandidate {
	int32_t low = 0;    // shape-low index (field 3)
	int32_t high = 0;   // shape-high index (field 4)
	std::array<int32_t, 4> detail{};  // detail indices (fields 5..8)
	float distortion = 0.0f;
};

void init_shape_basis(ShapeBasis& basis, const core::Codebook& codebook);

// Searches the shape fields for the target x[L] under weights w[L], keeping the best_low shape-low and best_high
// shape-high candidates per stage (each clamped to 1..16) and choosing each block's detail greedily. Writes the
// result_count best results to out, sorted by distortion, and returns how many there are.
int search_shape(const ShapeBasis& basis, const core::Codebook& codebook, int harmonic_count, const float* x,
                 const float* w, int best_low, int best_high, ShapeCandidate* out, int result_count);

}  // namespace halfrate::enc
