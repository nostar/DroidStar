#include "frame.h"

#include <cstring>

#include "tables.h"

namespace halfrate::core {

void bits_from_bytes(const uint8_t* frame, uint8_t* bits49) {
	for (int i = 0; i < kFrameBits; i++) bits49[i] = (frame[i >> 3] >> (7 - (i & 7))) & 1;
}

void bytes_from_bits(const uint8_t* bits49, uint8_t* frame) {
	std::memset(frame, 0, kFrameBytes);
	for (int i = 0; i < kFrameBits; i++) frame[i >> 3] |= static_cast<uint8_t>((bits49[i] & 1) << (7 - (i & 7)));
}

FrameFields fields_from_bits(const uint8_t* bits49) {
	FrameFields fields{};
	for (int f = 0; f < kFieldCount; f++) {
		uint32_t v = 0;
		for (int k = 0; k < tables::kFieldWidth[f]; k++) v = (v << 1) | (bits49[tables::kFieldBitPosition[f][k]] & 1);
		fields[f] = static_cast<int32_t>(v);
	}
	return fields;
}

void bits_from_fields(const FrameFields& fields, uint8_t* bits49) {
	for (int f = 0; f < kFieldCount; f++) {
		int width = tables::kFieldWidth[f];
		uint32_t v = static_cast<uint32_t>(fields[f]);
		for (int k = 0; k < width; k++) bits49[tables::kFieldBitPosition[f][k]] = (v >> (width - 1 - k)) & 1;
	}
}

void standard_from_rotated(const uint8_t* rotated49, uint8_t* standard49) {
	uint8_t b40 = rotated49[40], b41 = rotated49[41], b42 = rotated49[42], b43 = rotated49[43];
	std::memmove(standard49, rotated49, kFrameBits);
	standard49[40] = b43;
	standard49[41] = b40;
	standard49[42] = b41;
	standard49[43] = b42;
}

void rotated_from_standard(const uint8_t* standard49, uint8_t* rotated49) {
	uint8_t b40 = standard49[40], b41 = standard49[41], b42 = standard49[42], b43 = standard49[43];
	std::memmove(rotated49, standard49, kFrameBits);
	rotated49[40] = b41;
	rotated49[41] = b42;
	rotated49[42] = b43;
	rotated49[43] = b40;
}

}  // namespace halfrate::core
