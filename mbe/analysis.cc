#include "analysis.h"

#include <cstring>

#include "fastmath.h"
#include "fft.h"
#include "numeric.h"
#include "constants.h"
#include "history.h"
#include "tables.h"

namespace halfrate::enc {

using core::fast_log2;
using core::fast_sqrt;
using core::to_int32_sat;

namespace {

// The window response at p (1/16 bins, 256 = the centre), linearly interpolated; clamped at both ends.
float response_at(const AnalysisWindow& an, float p) {
	if (p <= 0.0f) return an.response[0];
	if (p >= 512.0f) return an.response[512];
	int i = to_int32_sat(p);
	float fraction = p - static_cast<float>(to_int32_sat(p));
	return an.response[i] + fraction * (an.response[i + 1] - an.response[i]);
}

// The harmonic (rounded bin / spacing) a bin belongs to.
inline int harmonic_of_bin(float bin, float spacing) { return to_int32_sat(bin / spacing + 0.5f); }

}  // namespace

void init_analysis_window(AnalysisWindow& an, const float* window, int length) {
	an.response.fill(0.0f);
	an.first_harmonic.fill(0);
	an.first_bin.fill(0);
	an.bin_count.fill(0);
	an.weight_offset.fill(0);
	an.weight_energy.fill(0.0f);
	an.gain.fill(0.0f);
	an.weights.fill(0.0f);
	an.silence_first_bin.fill(0);
	an.silence_bin_count.fill(0);
	an.silence_norm.fill(0.0f);
	int half = length / 2;
	an.length = length;
	an.half = half;
	an.window = window;
	for (int m = -256; m <= 256; m++) {
		float s = 0.0f;
		for (int i = 0; i < length; i++)
			s = s + window[i] * tables::kCosine[static_cast<uint32_t>(m * (i - half)) & 0x1fff];
		an.response[m + 256] = s;
	}
	int h = 0, offset = 0;
	for (int j = 0; j < kPitchCount; j++) {
		float f0 = tables::kPitchFundamental[j];
		int count = tables::kPitchHarmonicCount[j];
		an.first_harmonic[j] = h;
		if (count < 1) continue;
		int next = 0;
		float spacing = f0 * 512.0f;  // bins between harmonics
		for (int k = 1;; k++) {
			int first, n = 0, end;
			float energy;
			if (next > kFitBins - 1) {
				first = next;
				end = next;
				energy = kTiny;
			} else {
				int b = next;
				for (; b < kFitBins; b++)
					if (k <= harmonic_of_bin(static_cast<float>(b), spacing)) break;
				if (b >= kFitBins) {
					first = kFitBins;
					end = kFitBins;
					energy = kTiny;
				} else {
					first = b;
					float centre = spacing * static_cast<float>(static_cast<unsigned>(k));
					float* w = &an.weights[offset];
					energy = 0.0f;
					for (end = b; end < kFitBins; end++) {
						float bin = static_cast<float>(end);
						if (k != harmonic_of_bin(bin, spacing)) break;
						float v = response_at(an, (bin - centre) * 16.0f + 256.0f);
						w[n++] = v;
						energy = energy + v * v;
					}
					energy = energy + kTiny;
				}
			}
			an.first_bin[h] = static_cast<int16_t>(first);
			an.bin_count[h] = static_cast<int16_t>(n);
			an.weight_offset[h] = offset;
			an.weight_energy[h] = energy;
			an.gain[h] = 1.0f / energy;
			offset = n + offset;
			h++;
			next = end;
			if (k == count) break;
		}
	}
	an.first_harmonic[kPitchCount] = h;
	// silence band b (1..14): the bins i of 0..256 whose rounded i / 16 is b
	for (int b = 1; b < kSilenceBands + 1; b++) {
		float centre = static_cast<float>(static_cast<unsigned>(b)) * 16.0f;
		float s = 0.0f;
		int n = 0, first = -1;
		for (int i = 0; i < kSpectrumBins; i++) {
			if (b != to_int32_sat(static_cast<float>(i) * 0.0625f + 0.5f)) continue;
			float v = response_at(an, (static_cast<float>(i) - centre) * 16.0f + 256.0f);
			if (first < 0) first = i;
			s = s + v * v;
			n++;
		}
		an.silence_first_bin[b - 1] = static_cast<int16_t>(first);
		an.silence_bin_count[b - 1] = static_cast<int16_t>(n);
		an.silence_norm[b - 1] = s;
	}
}

void analysis_spectrum(const AnalysisWindow& an, const SignalHistory& history, int center, Spectrum& out) {
	int n = an.half;
	const float* w = an.window;
	float re[512], im[512];
	std::memset(re, 0, sizeof re);
	std::memset(im, 0, sizeof im);
	for (int k = 0; k < n; k++) {
		re[k] = history.sample(center + k) * w[n + k];
		re[512 - n + k] = history.sample(center - n + k) * w[k];
	}
	core::fft512(re, im);
	for (int k = 0; k < kSpectrumBins; k++) {
		out.re[k] = re[k];
		out.im[k] = im[k];
	}
}

void harmonic_fit_cost(const AnalysisWindow& an, const Spectrum& s, float* cost, float* energy) {
	float e = 0.0f;
	for (int i = 0; i < kFitBins; i += 4) {
		float lane[4];
		for (int j = 0; j < 4; j++) lane[j] = s.re[i + j] * s.re[i + j] + s.im[i + j] * s.im[i + j];
		e = e + lane[0];
		e = e + lane[1];
		e = e + lane[2];
		e = e + lane[3];
	}
	float total = e + kTiny;
	*energy = total;
	float inverse = 1.0f / total;
	int h = an.first_harmonic[0];
	for (int j = 0; j < kPitchCount; j++) {
		int end = an.first_harmonic[j + 1];
		float explained = 0.0f;
		for (; h < end; h++) {
			int n = an.bin_count[h];
			float ar = 0.0f, ai = 0.0f;
			if (n >= 1) {
				const float* w = &an.weights[an.weight_offset[h]];
				int first = an.first_bin[h];
				for (int k = 0; k < n; k++) {
					ar = ar + s.re[first + k] * w[k];
					ai = ai + w[k] * s.im[first + k];
				}
			}
			explained = explained + (ar * ar + ai * ai) * an.gain[h];
		}
		cost[j] = 1.0f - inverse * explained;
		h = end;
	}
}

void yin_difference(const SignalHistory& history, int center, const float* lags, int lag_count, int max_lag,
                    float* out) {
	float difference[163], normalised[161];
	if (max_lag >= 0) {
		int base = center + (-(max_lag / 2)) - 100;
		for (int tau = 0; tau <= max_lag; tau++) {
			float s = 0.0f;
			for (int i = 0; i < 200; i += 4) {
				float lane[4];
				for (int j = 0; j < 4; j++) {
					float v = history.sample(base + i + j) - history.sample(base + tau + i + j);
					lane[j] = v * v;
				}
				s = s + lane[0];
				s = s + lane[1];
				s = s + lane[2];
				s = s + lane[3];
			}
			difference[tau] = s;
		}
		normalised[0] = 1.0f;
		if (max_lag != 0) {
			float s = 0.0f;
			for (unsigned tau = 1; tau <= static_cast<unsigned>(max_lag); tau++) {
				s = s + difference[tau];
				normalised[tau] = difference[tau] / (s / static_cast<float>(tau) + kTiny);
			}
		}
	} else {
		normalised[0] = 1.0f;
	}
	for (int i = 0; i < lag_count; i++) {
		float lag = lags[i];
		int k = to_int32_sat(lag);
		float v;
		if (max_lag > k) {
			float fraction = lag - static_cast<float>(k);
			v = normalised[k] + fraction * (normalised[k + 1] - normalised[k]);
		} else {
			v = normalised[max_lag];
		}
		out[i] = v;
	}
}

void fit_harmonics(const AnalysisWindow& an, const Spectrum& s, int pitch, int harmonic_count, float fundamental,
                   HarmonicFit& out) {
	out = HarmonicFit{};
	std::array<float, 8> band_energy{}, band_error{};
	out.harmonic_count = harmonic_count;
	if (harmonic_count >= 1) {
		long h = an.first_harmonic[pitch];
		for (unsigned k = 1; k <= static_cast<unsigned>(harmonic_count); k++, h++) {
			int n = an.bin_count[h];
			int band = to_int32_sat(fundamental * (static_cast<float>(k) * 16.0f));
			band = band < 7 ? band : 7;
			float ar, ai, energy;
			if (n >= 1) {
				const float* w = &an.weights[an.weight_offset[h]];
				int first = an.first_bin[h];
				ar = 0.0f;
				ai = 0.0f;
				for (int i = 0; i < n; i++) {
					ar = ar + s.re[first + i] * w[i];
					ai = ai + w[i] * s.im[first + i];
				}
				float g = an.gain[h];
				ar = ar * g;
				ai = ai * g;
				float error = 0.0f;
				energy = 0.0f;
				for (int i = 0; i < n; i++) {
					float r = s.re[first + i], q = s.im[first + i], ww = w[i];
					float e1 = r - ar * ww;
					float e2 = q - ai * ww;
					error = error + (e1 * e1 + e2 * e2);
					energy = energy + (r * r + q * q);
				}
				band_error[band] = error + band_error[band];
				band_energy[band] = energy + band_energy[band];
				out.band_has_data[band] = 1;
			} else {
				energy = 0.0f;
				ar = an.gain[h] * energy;
				ai = ar;
			}
			out.log2_amplitude[k - 1] = fast_log2(fast_sqrt(ar * ar + ai * ai) + kAmplitudeFloor);
			float norm = an.weight_energy[h] + kMinusTiny;
			norm = norm < kTiny ? kTiny : norm;
			out.log2_rms[k - 1] = fast_log2(fast_sqrt(energy / norm) + kAmplitudeFloor);
		}
	}
	for (int b = 0; b < 8; b++) {
		out.band_energy[b] = band_energy[b];
		out.band_ratio[b] = out.band_has_data[b] ? band_error[b] / (band_energy[b] + kEnergyFloor) : 0.0f;
	}
}

void silence_amplitudes(const AnalysisWindow& an, const Spectrum& s, float* out) {
	for (int b = 0; b < kSilenceBands; b++) {
		int n = an.silence_bin_count[b];
		float sum = 0.0f;
		if (n >= 1) {
			int first = an.silence_first_bin[b];
			for (int k = 0; k < n; k++) sum = sum + (s.re[first + k] * s.re[first + k] + s.im[first + k] * s.im[first + k]);
		}
		out[b] = fast_log2(fast_sqrt(sum / an.silence_norm[b]) + kAmplitudeFloor);
	}
}

}  // namespace halfrate::enc
