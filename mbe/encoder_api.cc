// The encoder half of the C API (include/halfrate.h).
#include <cstring>
#include <new>

#include "frame.h"
#include "numeric.h"
#include "encoder.h"
#include "halfrate.h"

struct hr_encoder {
	halfrate::enc::Encoder encoder;
};

extern "C" {

hr_encoder* hr_encoder_new(void) {
	hr_encoder* e = new (std::nothrow) hr_encoder;
	if (!e) return nullptr;
	halfrate::enc::encoder_init(e->encoder);
	return e;
}

void hr_encoder_free(hr_encoder* e) { delete e; }

int hr_encoder_set(hr_encoder* e, const char* name, double value) {
	if (!e || !name) return -1;
	if (!halfrate::enc::set_option(e->encoder.options, name, value)) return -1;
	e->encoder.transitions_dirty = true;
	return 0;
}

void hr_encode(hr_encoder* e, const int16_t* pcm, uint8_t* frame) {
	using namespace halfrate;
	if (!e || !pcm || !frame) return;
	core::FrameFields fields;
	enc::encode_frame(e->encoder, pcm, fields);
	uint8_t bits[core::kFrameBits], rotated[core::kFrameBits];
	core::bits_from_fields(fields, bits);
	if (core::to_int32_sat(e->encoder.options.rotated_layout) == 1) {
		core::rotated_from_standard(bits, rotated);
		core::bytes_from_bits(rotated, frame);
	} else {
		core::bytes_from_bits(bits, frame);
	}
}

}  // extern "C"
