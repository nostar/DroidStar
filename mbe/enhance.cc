#include "enhance.h"

#include "fastmath.h"
#include "numeric.h"
#include "tables.h"

namespace halfrate::core {

namespace {
constexpr float kResponseScale = 0x1.8209f6p+1f;  // 3.01592898
constexpr float kLog2Floor = 0x1.197998p-40f;     // 1e-12, keeps log2 finite
constexpr float kTiltStart = -0x1.4cccccp-1f;    // tilt starts at harmonic 0.65 L ...
constexpr float kTiltWidth = 0x1.666668p-2f;      // ... reaches full at 0.65 L + 0.35 L
constexpr float kTiltLog2Gain = 0x1.19999ap-1f;   // full tilt: 2^0.55

// cos(2 pi f l) from the table, f in cycles per sample
inline float harmonic_cosine(float fundamental, float harmonic) {
	float phase = fundamental * harmonic;
	phase = phase * 8192.0f;
	phase = phase + 0.5f;
	return tables::kCosine[to_int64_sat(phase) & 8191];
}
}  // namespace

void enhance_formants(const EnhanceOptions& options, float fundamental, float omega, int harmonic_count,
                      float* amplitude) {
	if (options.mode != 1) return;
	int count = harmonic_count;
	if (count < 1) return;
	float r0 = 0.0f, r1 = 0.0f;
	for (int l = 1; l <= count; l++) {
		float power = amplitude[l] * amplitude[l];
		r0 = r0 + power;
		r1 = r1 + power * harmonic_cosine(fundamental, static_cast<float>(static_cast<unsigned>(l)));
	}
	if (r0 <= 0.0f) return;
	float r0_squared = r0 * r0, r1_squared = r1 * r1;
	float squares = r1_squared + r0_squared;
	float twice_product = r1 * (r0 + r0);
	float difference = r0_squared - r1_squared;
	float weight[kMaxHarmonics + 8];
	int32_t every = options.bypass_divisor;
	uint32_t bypass_limit = static_cast<uint32_t>(every);  // l * every, wrapping like a 32-bit counter
	for (int l = 1; l <= count; l++, bypass_limit += static_cast<uint32_t>(every)) {
		float w = 1.0f;
		float denominator = difference * (r0 * omega);
		float harmonic = static_cast<float>(static_cast<unsigned>(l));
		float numerator;
		if (denominator <= 0.0f) goto store;
		numerator = squares - twice_product * harmonic_cosine(fundamental, harmonic);
		numerator = numerator * kResponseScale;
		if (numerator <= 0.0f) goto store;
		if (amplitude[l] <= 0.0f) goto store;
		{
			float log2_amplitude = fast_log2(amplitude[l]);
			float log2_response = fast_log2(numerator / denominator);
			log2_response = log2_response * 0.5f;
			float raw = fast_exp2(options.strength * (log2_amplitude + log2_response));
			if (every >= 1 && static_cast<int32_t>(bypass_limit) <= count)
				w = 1.0f;
			else
				w = raw > options.max_weight ? options.max_weight : (raw < options.min_weight ? options.min_weight : raw);
			float exponent = fundamental * harmonic < options.split_frequency ? options.power_low : options.power_high;
			if (exponent != 1.0f) w = fast_exp2(exponent * fast_log2(w));
		}
	store:
		weight[l] = w;
	}
	float energy = 0.0f;
	for (int l = 1; l <= count; l++) {
		float v = amplitude[l] * weight[l];
		amplitude[l] = v;
		energy = energy + v * v;
	}
	float scale = 1.0f;
	if (energy > 0.0f) scale = fast_sqrt(r0 / energy);
	for (int l = 1; l <= count; l++) amplitude[l] = scale * amplitude[l];
}

void shape_log2(const FrameParams& frame, bool tilt, float* out) {
	int count = frame.harmonic_count;
	float amplitude[kMaxHarmonics + 8];
	amplitude[0] = 0.0f;
	for (int i = 0; i < count; i++) amplitude[1 + i] = fast_exp2(frame.log2_amplitude[i]);
	enhance_formants(EnhanceOptions{}, frame.fundamental, frame.omega, count, amplitude);
	if (tilt) {
		for (int l = 1; l <= count; l++) {
			float x = static_cast<float>(static_cast<unsigned>(l)) / static_cast<float>(count);
			x = x + kTiltStart;
			x = x / kTiltWidth;
			if (!(x > 0.0f)) continue;
			float a = amplitude[l];
			x = x > 1.0f ? 1.0f : x;
			x = x * kTiltLog2Gain;
			amplitude[l] = a * fast_exp2(x);
		}
	}
	for (int i = 0; i < count; i++) out[1 + i] = fast_log2(amplitude[1 + i] + kLog2Floor);
}

}  // namespace halfrate::core
