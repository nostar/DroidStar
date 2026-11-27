// The 49-bit frame: nine parameter fields at fixed bit positions.
//
//   field  bits  meaning
//   0      7     pitch index (0..119 voice; 120..123 erasure; 124..125 tone/fixed pitch; 126..127 reserved)
//   1      5     voicing index (16/17 silence, 18 and up pulse codes, see the decoder)
//   2      5     gain index
//   3      9     shape, 8-point transform coefficients 1..3
//   4      7     shape, 8-point transform coefficients 4..7
//   5..8   5 4 4 3  higher-order shape coefficients of blocks 1..4
//
// Field bits are spread over the frame (tables::kFieldBitPosition). A bit array holds one bit per byte, frame
// bit 0 first; a byte frame is 7 bytes, MSB first, the last byte holding bit 48 in its top bit.
//
// The rotated layout (a decoder/encoder option) is the same frame with bits 40..43 rotated: what the standard
// layout has at 40, 41, 42, 43 is sent at 43, 40, 41, 42.
#pragma once
#include <array>
#include <cstdint>

namespace halfrate::core {

constexpr int kFrameBits = 49;
constexpr int kFrameBytes = 7;
constexpr int kFieldCount = 9;

// Field indices
enum Field : int {
	kFieldPitch = 0,
	kFieldVoicing = 1,
	kFieldGain = 2,
	kFieldShapeLow = 3,
	kFieldShapeHigh = 4,
	kFieldDetail1 = 5,  // 5..8: blocks 1..4
};

using FrameFields = std::array<int32_t, kFieldCount>;

// 7 bytes <-> 49 bits
void bits_from_bytes(const uint8_t* frame, uint8_t* bits49);
void bytes_from_bits(const uint8_t* bits49, uint8_t* frame);

// 49 bits <-> the nine fields (standard layout). bits_from_fields writes every bit of the frame.
FrameFields fields_from_bits(const uint8_t* bits49);
void bits_from_fields(const FrameFields& fields, uint8_t* bits49);

// Rotated layout <-> standard layout (bits 40..43); the other bits are copied.
void standard_from_rotated(const uint8_t* rotated49, uint8_t* standard49);
void rotated_from_standard(const uint8_t* standard49, uint8_t* rotated49);

}  // namespace halfrate::core
