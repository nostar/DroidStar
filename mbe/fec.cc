#include "fec.h"

#include <cstring>

#include "tables.h"

namespace halfrate::core {

namespace {

// channel position of code bit i (DMR interleave)
inline int channel_position(int i) { return 4 * i - 71 * (i / 18); }

inline int popcount(uint32_t v) { return __builtin_popcount(v); }

}  // namespace

uint32_t golay23_parity(uint32_t data) {
	uint32_t parity = 0;
	for (int i = 0; i < 12; i++)
		if ((data >> (11 - i)) & 1) parity ^= tables::kGolayParityRows[i];
	return parity;
}

uint32_t golay23_decode(uint32_t code, int* errors) {
	uint32_t data = (code >> 11) & 0xfff;
	uint32_t syndrome = (golay23_parity(data) ^ code) & 0x7ff;
	uint32_t correction = tables::kGolaySyndromeCorrection[syndrome] & 0xfff;
	data ^= correction;
	if (errors) {
		uint32_t left = (golay23_parity(data) ^ code) & 0x7ff;
		*errors = popcount(left) + popcount(correction);
	}
	return data;
}

uint32_t fec_scramble_mask(uint32_t first_data) {
	uint32_t state = first_data << 4, mask = 0;
	for (int i = 0; i < 23; i++) {
		uint32_t v = state * 173 + 13849;
		state = v & 0xffff;
		mask = (mask << 1) | ((v >> 15) & 1);
	}
	return mask;
}

void fec_encode_bits(const uint8_t* bits49, uint8_t* bits72) {
	uint32_t d0 = 0, d1 = 0;
	for (int i = 0; i < 12; i++) d0 = (d0 << 1) | (bits49[i] & 1);
	for (int i = 0; i < 12; i++) d1 = (d1 << 1) | (bits49[12 + i] & 1);
	uint8_t code[72];
	uint32_t word0 = (golay23_parity(d0) & 0x7ff) | (d0 << 11), parity = 0;
	for (int i = 0; i < 23; i++) {
		code[i] = (word0 >> (22 - i)) & 1;
		parity ^= code[i];
	}
	code[23] = static_cast<uint8_t>(parity);
	uint32_t word1 = fec_scramble_mask(d0) ^ ((golay23_parity(d1) & 0x7ff) | (d1 << 11));
	for (int i = 0; i < 23; i++) code[24 + i] = (word1 >> (22 - i)) & 1;
	for (int i = 0; i < 25; i++) code[47 + i] = bits49[24 + i] & 1;
	for (int i = 0; i < 72; i++) bits72[channel_position(i)] = code[i];
}

int fec_decode_bits(const uint8_t* bits72, uint8_t* bits49, FecErrors* errors) {
	uint8_t code[72];
	for (int i = 0; i < 72; i++) code[i] = bits72[channel_position(i)] & 1;
	uint32_t word = 0;
	for (int i = 0; i < 23; i++) word = (word << 1) | code[i];
	int e0, e1;
	uint32_t d0 = golay23_decode(word, &e0);
	uint32_t corrected0 = (golay23_parity(d0) & 0x7ff) | (d0 << 11);
	uint32_t parity = popcount(corrected0 & 0x7fffff) & 1;
	word = 0;
	for (int i = 24; i < 47; i++) word = (word << 1) | code[i];
	uint32_t d1 = golay23_decode(fec_scramble_mask(d0) ^ word, &e1);
	for (int i = 0; i < 12; i++) bits49[i] = (d0 >> (11 - i)) & 1;
	for (int i = 0; i < 12; i++) bits49[12 + i] = (d1 >> (11 - i)) & 1;
	std::memcpy(bits49 + 24, code + 47, 25);
	if (errors) {
		errors->first_word = e0;
		errors->second_word = e1;
	}
	return e0 >= 3 && parity != code[23] ? -1 : e1 + e0;
}

void fec_encode_frame(const uint8_t* frame, uint8_t* air) {
	uint8_t bits49[49], bits72[72];
	for (int i = 0; i < 49; i++) bits49[i] = (frame[i >> 3] >> (7 - (i & 7))) & 1;
	fec_encode_bits(bits49, bits72);
	std::memset(air, 0, 9);
	for (int i = 0; i < 72; i++) air[i >> 3] |= static_cast<uint8_t>(bits72[i] << (7 - (i & 7)));
}

int fec_decode_frame(const uint8_t* air, uint8_t* frame, FecErrors* errors) {
	uint8_t bits72[72], bits49[49];
	for (int i = 0; i < 72; i++) bits72[i] = (air[i >> 3] >> (7 - (i & 7))) & 1;
	int result = fec_decode_bits(bits72, bits49, errors);
	std::memset(frame, 0, 7);
	for (int i = 0; i < 49; i++) frame[i >> 3] |= static_cast<uint8_t>(bits49[i] << (7 - (i & 7)));
	return result;
}

}  // namespace halfrate::core
