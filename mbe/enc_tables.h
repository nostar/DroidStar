// Constant tables of the encoder (generated data, see the files named below). The pitch grid the encoder searches
// is the codebook's pitch table (tables.h: kPitchFundamental, kPitchHarmonicCount).
#pragma once
#include <cstdint>

namespace halfrate::tables {

// enc_pitch_net.cc: the pitch network (an int8 MLP 1830 -> 160 -> 96 -> 121). Weights are [output][input].
extern const float kPitchNetMean[305];    // feature normalisation, per feature of one frame
extern const float kPitchNetInvStd[305];
extern const int8_t kPitchNetWeights1[160 * 1830];
extern const float kPitchNetBias1[160];
extern const float kPitchNetScale1[160];
extern const int8_t kPitchNetWeights2[96 * 160];
extern const float kPitchNetBias2[96];
extern const float kPitchNetScale2[96];
extern const int8_t kPitchNetWeights3[121 * 96];
extern const float kPitchNetBias3[121];
extern const float kPitchNetScale3[121];

// enc_analysis.cc: analysis windows and voicing tables.
extern const float kLongWindow[256];    // window of the pitch spectrum
extern const float kShortWindow[160];   // window of the amplitude spectrum
extern const int32_t kVoicingRowStart[9];          // pitch index where each threshold row starts
extern const float kVoicingThresholdBase[9 * 8];   // [row][band]
extern const float kVoicingThresholdSlope[9 * 8];  // [row][band], times the pitch's fit cost
extern const float kRescoreBandEdge[18];           // Hz; band j is [edge[j], edge[j + 1]), the last one open

}  // namespace halfrate::tables
