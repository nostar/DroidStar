// The synthesiser: harmonic oscillators, noise from inverse FFTs, pulses, and a 160-sample overlap-add.
// Each frame interpolates from the previous frame's parameters to the current one's.
#pragma once
#include <array>
#include <cstdint>

#include "decoder_options.h"

namespace halfrate::dec {

constexpr int kFrameSamples = 160;
constexpr int kHarmonicSlots = 58;  // per-harmonic arrays, indexed by harmonic number 1..L (L <= 56)

// What the synthesiser makes of one frame.
struct SynthFrame {
	int32_t active = 0;          // nonzero: the frame has sound
	float fundamental = 0.0f;    // cycles per sample
	int32_t harmonic_count = 0;  // L
	std::array<float, kHarmonicSlots> amplitude{};  // linear
	std::array<float, kHarmonicSlots> voiced{};     // weight of the harmonic's oscillator
	std::array<float, kHarmonicSlots> unvoiced{};   // weight of noise around the harmonic
	std::array<float, kHarmonicSlots> pulse{};      // weight of the harmonic in the pulse shape
	std::array<float, kHarmonicSlots> phase{};      // phase offset, cycles
	int32_t pulse_count = 0;     // 0..2
	std::array<int32_t, 2> pulse_position{};
	std::array<float, 2> pulse_gain{};
};

struct SynthState {
	SynthFrame previous;
	std::array<uint64_t, kHarmonicSlots> harmonic_phase{};  // per-harmonic oscillators, 2^64 = one cycle
	uint64_t fundamental_phase = 0;                         // phase_tracking 1: the fundamental's phase
	std::array<uint32_t, 63> noise_phase{};                 // drifting phases of the low noise bins (13 bits)
	std::array<float, 2 * kFrameSamples> overlap{};         // the next frame's first 160 samples, and the rest
	uint32_t random = 0;                                    // LCG state
	std::array<float, 320> window320{};                     // sine windows
	std::array<float, 80> window80{};
};

// Clears the state, seeds the noise generator and sets the initial oscillator phases.
void synth_reset(SynthState& state, const Options& options);

// One frame: out[0..159].
void synth_frame(SynthState& state, const Options& options, const SynthFrame& frame, float* out);

// 160 samples of uniform noise of rms level.
void synth_comfort_noise(SynthState& state, float level, float* out);

// cos(2 pi x), x in cycles, linearly interpolated in the cosine table.
float cosine_interpolated(float x);

}  // namespace halfrate::dec
