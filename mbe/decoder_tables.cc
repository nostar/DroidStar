#include "decoder_tables.h"

namespace halfrate::dec::tables {

const char kHarmonicClass[32][17] = {
	"VVVVVVVVVVVVVVVV", "VVVVVVVVVVVVVVMM", "VVVVVVVVVVVVVVUU", "VVVVVVVVVVMVVVVV",
	"VVVVVVVVVVVVUUUU", "VVVVUVVVVVVVVVVV", "VVVVVVUVVVVVVVVV", "VVVVVVVVVVUVVVVV",
	"VVVVVVVVUUUUUUUU", "VVVVVVVVVVUUUUUU", "VVVVVVUUUUUUUUUU", "VVVVVVUUUUUUUMMM",
	"VVVVUUUUUUUUUUUU", "VVVVMMUUUUUUUUUU", "VVUUUUUUUUUUUUUU", "MMMMMMUUUUUUUUUU",
	"UUUUUUUUUUUUUUUU", "MMUUUUUUUUUUUUUU", "QQUUUUUUUUUUUUUU", "QQQQUUUUUUUUUUUU",
	"QQQQQQQQQQQQQQQQ", "UUUQQQUUUUUUUUUU", "UUUUUUUUUUUUUQQQ", "UUUUUUUUUQQQQQQQ",
	"PPPPPPPPPPPPPPPP", "UUUPPPPPPPPPPPPP", "QQUUUUUUUUUUUUUU", "UUUQQQUUUUUUUUUU",
	"QQQQQQQQQQQQQQQQ", "QQQQUUUUUUUUUUUU", "UUUUUUUUUUUUUQQQ", "UUUUUUUUUUUQQQQQ",
};

const float kPhaseTapWeight[33] = {
	0x0p+0f, 0x1p+0f, 0x1p-1f, 0x1.555556p-2f, 0x1p-2f, 0x1.99999ap-3f, 0x1.555556p-3f, 0x1.24924ap-3f,
	0x1p-3f, 0x1.c71c72p-4f, 0x1.99999ap-4f, 0x1.745d18p-4f, 0x1.555556p-4f, 0x1.3b13b2p-4f, 0x1.24924ap-4f,
	0x1.111112p-4f, 0x1p-4f, 0x1.e1e1e2p-5f, 0x1.c71c72p-5f, 0x1.af286cp-5f, 0x1.99999ap-5f, 0x1.861862p-5f,
	0x1.745d18p-5f, 0x1.642c86p-5f, 0x1.555556p-5f, 0x1.47ae14p-5f, 0x1.3b13b2p-5f, 0x1.2f684cp-5f,
	0x1.24924ap-5f, 0x1.1a7b96p-5f, 0x1.111112p-5f, 0x1.08421p-5f, 0x1p-5f,
};

const float kKeypadLow[4] = {697.0f, 770.0f, 852.0f, 941.0f};
const float kKeypadHigh[4] = {1209.0f, 1336.0f, 1477.0f, 1633.0f};
const int32_t kKeypadLowIndex[16] = {3, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 1, 2, 3, 3, 3};
const int32_t kKeypadHighIndex[16] = {1, 0, 1, 2, 0, 1, 2, 0, 1, 2, 3, 3, 3, 3, 0, 2};

const float kToneSetLow[4] = {606.0f, 672.0f, 743.0f, 820.0f};
const float kToneSetHigh[4] = {1052.0f, 1162.0f, 1297.0f, 1430.0f};
const int8_t kToneSetIndex[16][2] = {
	{3, 1}, {0, 0}, {0, 1}, {0, 2}, {1, 0}, {1, 1}, {1, 2}, {2, 0},
	{2, 1}, {2, 2}, {0, 3}, {1, 3}, {2, 3}, {3, 3}, {3, 0}, {3, 2},
};

}  // namespace halfrate::dec::tables
