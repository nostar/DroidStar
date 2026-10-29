// Spectral (formant) enhancement of the harmonic amplitudes, shared by the decoder (before synthesis) and the
// encoder (which scores candidate frames by what the decoder will produce).
#pragma once
#include <cstdint>

#include "dequant.h"

namespace halfrate::core {

// Enhancement settings. The defaults are the decoder's defaults and what shape_log2 uses.
struct EnhanceOptions {
	int32_t mode = 1;              // the enhancement runs only when this is exactly 1
	float strength = 0.5f;         // exponent applied to the raw weight
	float min_weight = 0.3f;       // weights are clamped to [min_weight, max_weight]
	float max_weight = 2.0f;
	int32_t bypass_divisor = 0;    // >= 1: harmonics l with l * bypass_divisor <= L keep weight 1
	float power_high = 0.85f;      // the weight is raised to this power at or above split_frequency
	float power_low = 0.6f;        // and to this one below it
	float split_frequency = 0.125f;  // cycles per sample (1 kHz)
};

// Enhances amplitude[1..L] (linear; amplitude[0] is not used) in place. Each harmonic gets a weight
// (amplitude * sqrt(R(w)))^strength, where R is the response of a one-pole model fitted to the spectrum's
// autocorrelation R0, R1, clamped and raised to a frequency-dependent power; the result is rescaled to the
// original energy. fundamental is in cycles per sample, omega in radians per sample.
void enhance_formants(const EnhanceOptions& options, float fundamental, float omega, int harmonic_count,
                      float* amplitude);

// log2 harmonic amplitudes of frame after enhancement with the default options and, if tilt, a gain rising toward
// the top harmonics: out[1..L] (out[0] is not written). Reads fundamental, omega, harmonic_count and log2_amplitude.
void shape_log2(const FrameParams& frame, bool tilt, float* out);

}  // namespace halfrate::core
