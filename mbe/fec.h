// Forward error correction of the 49-bit frame into the 72-bit DMR on-air frame.
//
// The 72 code bits are: a Golay (24,12) word of frame bits 0..11 (the (23,12) code plus an overall parity bit),
// a Golay (23,12) word of frame bits 12..23 XORed with a 23-bit scrambling sequence seeded by bits 0..11, and frame
// bits 24..48 uncoded. Code bit i goes to channel bit 4 i - 71 floor(i / 18) (the DMR 4 x 18 interleave).
// Bit arrays hold one bit per byte (only bit 0 is read); byte frames are MSB first.
#pragma once
#include <cstdint>

namespace halfrate::core {

// The 11 parity bits of 12 data bits.
uint32_t golay23_parity(uint32_t data);

// Decodes a (23,12) code word (data in bits 22..11, parity in bits 10..0) and returns the 12 corrected data bits.
// If errors is given it receives the corrected data bits plus the parity mismatches left after correction.
uint32_t golay23_decode(uint32_t code, int* errors);

// 23-bit scrambling mask of the second Golay word, from the first word's 12 data bits.
uint32_t fec_scramble_mask(uint32_t first_data);

// Error counts of one decoded frame.
struct FecErrors {
	int first_word = 0;   // errors of the Golay (24,12) word (frame bits 0..11)
	int second_word = 0;  // errors of the scrambled Golay (23,12) word (frame bits 12..23)
};

// 49 frame bits -> 72 channel bits.
void fec_encode_bits(const uint8_t* bits49, uint8_t* bits72);

// 72 channel bits -> 49 frame bits. Returns first_word + second_word, or -1 (frame unreliable) when the first
// word had 3 errors or more and its overall parity bit disagrees with the corrected word.
int fec_decode_bits(const uint8_t* bits72, uint8_t* bits49, FecErrors* errors);

// 7-byte frame -> 9-byte on-air frame.
void fec_encode_frame(const uint8_t* frame, uint8_t* air);

// 9-byte on-air frame -> 7-byte frame. Returns what fec_decode_bits returns; errors as there (may be null).
int fec_decode_frame(const uint8_t* air, uint8_t* frame, FecErrors* errors);

}  // namespace halfrate::core
