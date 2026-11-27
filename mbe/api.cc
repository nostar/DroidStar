// The C API (include/halfrate.h).
#include "halfrate.h"

#include <cstring>

#include "fec.h"
#include "frame.h"

#ifndef HR_VERSION
#define HR_VERSION "unknown"
#endif

using namespace halfrate;

extern "C" {

void hr_fec_encode(const uint8_t* frame, uint8_t* air) { core::fec_encode_frame(frame, air); }

int hr_fec_decode(const uint8_t* air, uint8_t* frame) {
	core::FecErrors errors;
	if (core::fec_decode_frame(air, frame, &errors) < 0) return -1;
	return errors.first_word + errors.second_word;
}

void hr_bits_from_frame(const uint8_t* frame, uint8_t* bits) { core::bits_from_bytes(frame, bits); }

void hr_frame_from_bits(const uint8_t* bits, uint8_t* frame) { core::bytes_from_bits(bits, frame); }

const char* hr_version(void) { return "halfrate " HR_VERSION; }

}  // extern "C"
