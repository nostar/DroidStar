// The half-rate codec's codebook set (core::Codebook), built from the tables in codebook.cc.
#include "tables.h"

namespace halfrate::tables {

const core::Codebook kCodebook = {
	1,
	kPitchFundamental,
	kPitchHarmonicCount,
	32,
	kVoicingCodebook,
	16,
	kBandSizes,
	32,
	kGainCodebook,
	kShapeLowCodebook,
	kShapeHighCodebook,
	{&kShapeDetail1Codebook[0][0], &kShapeDetail2Codebook[0][0], &kShapeDetail3Codebook[0][0],
	 &kShapeDetail4Codebook[0][0]},
	{32, 16, 16, 8},
	{1, 1, 1, 1},
	0x1.4cccccp-1f,  // 0.65
	120,
	1.0f,
	0.5f,
};

}  // namespace halfrate::tables
