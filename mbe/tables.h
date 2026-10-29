// Constant tables shared by the encoder and decoder. The large ones are generated data (cosine.cc, golay.cc,
// codebook.cc); the rest are written out by hand in the files named below.
#pragma once
#include <cstdint>

#include "codebook.h"

namespace halfrate::tables {

// cosine.cc: cos(2 pi i / 8192) for i = 0..8191, one turn. sin is kCosine[(i + 6144) & 8191].
extern const float kCosine[8192];

// golay.cc: the (23,12) Golay code.
extern const uint32_t kGolayParityRows[12];          // parity bits of each data bit, data bit 11 first
extern const uint32_t kGolaySyndromeCorrection[2048];  // syndrome -> error pattern (data part in the low 12 bits)

// codebook.cc: the parameter codebooks. kCodebook (codebook_set.cc) ties them together.
extern const float kPitchFundamental[120];   // cycles per sample, per pitch index
extern const int32_t kPitchHarmonicCount[120];
extern const int32_t kVoicingCodebook[32][8];  // per voicing index: band 0..7 voiced (1) or not (0)
extern const int32_t kBandSizes[57][4];       // per harmonic count L: sizes of the 4 transform blocks (sum L)
extern const float kGainCodebook[32];
extern const float kShapeLowCodebook[512][3];   // 8-point transform coefficients 1..3
extern const float kShapeHighCodebook[128][4];  // 8-point transform coefficients 4..7
extern const float kShapeDetail1Codebook[32][4];  // higher-order coefficients of block 1..4
extern const float kShapeDetail2Codebook[16][4];
extern const float kShapeDetail3Codebook[16][4];
extern const float kShapeDetail4Codebook[8][4];
// kInverseDct[n - 1][i][k]: element i of basis function k of the size-n inverse DCT, n = 1..17
extern const float kInverseDct[17][18][18];
extern const float kHalfLog2Count[64];    // log2(L) / 2
extern const float kReciprocalCount[64];  // 1 / L (0 for L = 0)

// codebook_set.cc: the half-rate codebook set the dequantiser uses.
extern const core::Codebook kCodebook;

// frame_layout.cc: the 49-bit frame layout.
extern const int32_t kFieldWidth[9];        // bits per parameter field
extern const int8_t kFieldBitPosition[9][9];  // frame bit of each field bit, most significant first

}  // namespace halfrate::tables
