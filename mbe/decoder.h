// The frame decoder: voice, silence and tone frames, erasures (repeat, mute, comfort noise), and the output stage
// (high-pass, frame limiter, soft limiter, conversion to 16 bits).
#pragma once
#include <array>
#include <cstdint>

#include "dequant.h"
#include "decoder_options.h"
#include "decoder_synth.h"

namespace halfrate::dec {

// What a frame decoded as.
enum class DecodedKind : int32_t {
	voice = 0,
	silence = 1,
	tone = 2,
	erasure = 3,  // concealed: a repeat of the last good frame, a muted frame or comfort noise
};

// Two cascaded biquad high-pass sections with the same coefficients.
struct HighPass {
	float cutoff = -1.0f;  // the cutoff (Hz) the coefficients are for
	float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
	struct Section {
		float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;
	};
	std::array<Section, 2> sections{};
};

struct Decoder {
	Options options;
	core::DequantState dequant;
	SynthState synth;
	SynthFrame last_good;          // the last voice or silence frame (amplitudes scaled by each repeat)
	bool had_good = false;         // a voice or silence frame was decoded since the start or the last tone
	int32_t bad_count = 0;         // consecutive erased frames
	bool muted = false;            // erasures have gone past repeating
	std::array<uint32_t, 2> tone_phase{};  // tone oscillators, 2^32 = one cycle
	float limiter_gain = 1.0f;     // frame limiter gain reached at the end of the previous frame
	HighPass highpass;

	Decoder();
	// Back to the start-of-stream state; the options are kept.
	void reset();
	// Sets an option by name (options.h); false for an unknown name.
	bool set(const char* name, double value);
	// One 49-bit frame (one bit per byte) to 160 samples.
	DecodedKind decode_bits(const uint8_t* bits49, int16_t* pcm);
	// One erased frame (no frame received) to 160 samples.
	void decode_erased(int16_t* pcm);

private:
	void conceal(float* buf);
	DecodedKind tone_or_erasure(const uint8_t* bits49, float* buf);
	void voice(const core::FrameParams& params, bool silence, float* buf);
	void output(float* buf, int16_t* pcm);
};

}  // namespace halfrate::dec
