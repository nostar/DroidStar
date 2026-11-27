// The 49-bit frame layout: width of each of the nine parameter fields, and the frame bit that carries each field
// bit, most significant first (unused entries are 0). Bits 0..23 are the two Golay-protected words.
#include "tables.h"

namespace halfrate::tables {

const int32_t kFieldWidth[9] = {7, 5, 5, 9, 7, 5, 4, 4, 3};

const int8_t kFieldBitPosition[9][9] = {
	{0, 1, 2, 3, 37, 38, 39, 0, 0},
	{4, 5, 6, 7, 35, 0, 0, 0, 0},
	{8, 9, 10, 11, 36, 0, 0, 0, 0},
	{12, 13, 14, 15, 16, 17, 18, 19, 40},
	{20, 21, 22, 23, 41, 42, 43, 0, 0},
	{24, 25, 26, 27, 44, 0, 0, 0, 0},
	{28, 29, 30, 45, 0, 0, 0, 0, 0},
	{31, 32, 33, 46, 0, 0, 0, 0, 0},
	{34, 47, 48, 0, 0, 0, 0, 0, 0},
};

}  // namespace halfrate::tables
