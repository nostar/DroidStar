// Spectral analysis of the encoder: the analysis windows with their per-pitch harmonic tables, the windowed
// spectrum, the per-pitch harmonic fit cost, the YIN difference function, the harmonic fit at one pitch, and the
// band amplitudes of a silent frame.
#pragma once
#include <array>
#include <cstdint>

namespace halfrate::enc {

constexpr int kPitchCount = 120;      // pitch grid = pitch indices of the codebook
constexpr int kSpectrumBins = 257;    // bins 0..256 of the 512-point FFT
constexpr int kFitBins = 244;         // bins the harmonic fits use (0..243, up to 3.8 kHz)
constexpr int kGridHarmonics = 3097;  // sum of the harmonic counts of the grid
constexpr int kGridWeights = 27683;   // window weights of all grid harmonics
constexpr int kSilenceBands = 14;     // 250 Hz bands of a silent frame
constexpr int kMaxFitHarmonics = 56;

// The first 257 bins of a windowed 512-point FFT.
struct Spectrum {
	std::array<float, kSpectrumBins> re;
	std::array<float, kSpectrumBins> im;
};

// One analysis window and what is precomputed from it: the window's transform sampled every 1/16 bin, and for
// each grid pitch the FFT bins of each harmonic with their weights (the window response at the bin's distance from
// the harmonic). Harmonics of all pitches are numbered together: pitch j has harmonics first_harmonic[j] ..
// first_harmonic[j + 1] - 1.
struct AnalysisWindow {
	int32_t length = 0;
	int32_t half = 0;
	const float* window = nullptr;
	std::array<float, 513> response{};  // window transform at (index - 256) / 16 bins
	std::array<int32_t, kPitchCount + 1> first_harmonic{};
	std::array<int16_t, kGridHarmonics> first_bin{};
	std::array<int16_t, kGridHarmonics> bin_count{};
	std::array<int32_t, kGridHarmonics> weight_offset{};  // into weights
	std::array<float, kGridHarmonics> weight_energy{};    // sum of squared weights (+ 1e-12)
	std::array<float, kGridHarmonics> gain{};             // 1 / weight_energy
	std::array<float, kGridWeights> weights{};
	std::array<int16_t, kSilenceBands> silence_first_bin{};
	std::array<int16_t, kSilenceBands> silence_bin_count{};
	std::array<float, kSilenceBands> silence_norm{};  // sum of squared window weights of the band
};

// The harmonic fit of one frame at one pitch.
struct HarmonicFit {
	int32_t harmonic_count = 0;
	std::array<float, 8> band_ratio{};       // per 500 Hz band: fit residual / band energy (0 if no data)
	std::array<float, 8> band_energy{};
	std::array<uint8_t, 8> band_has_data{};
	std::array<float, kMaxFitHarmonics> log2_amplitude{};  // log2 of the fitted harmonic amplitude
	std::array<float, kMaxFitHarmonics> log2_rms{};        // log2 rms of the spectrum around the harmonic
};

// Builds the tables of window[length] for the grid (the codebook's pitch table).
void init_analysis_window(AnalysisWindow& an, const float* window, int length);

// Reads input samples by index (the analysis history, see encoder.h).
struct SignalHistory;

// Windowed spectrum of the samples around center (the window's second half from center on, its first half
// before it).
void analysis_spectrum(const AnalysisWindow& an, const SignalHistory& history, int center, Spectrum& out);

// Per grid pitch: 1 - (energy the pitch's harmonics explain) / (total energy). *energy gets the total energy of
// the fitted bins (+ 1e-12).
void harmonic_fit_cost(const AnalysisWindow& an, const Spectrum& s, float* cost, float* energy);

// YIN cumulative-mean-normalised difference (200-sample frames, lags 0..max_lag) at the fractional lags given.
void yin_difference(const SignalHistory& history, int center, const float* lags, int lag_count, int max_lag,
                    float* out);

// Fits the harmonic_count harmonics of grid pitch `pitch` (fundamental in cycles per sample) to the spectrum.
void fit_harmonics(const AnalysisWindow& an, const Spectrum& s, int pitch, int harmonic_count, float fundamental,
                   HarmonicFit& out);

// log2 rms amplitudes of the 14 silence bands.
void silence_amplitudes(const AnalysisWindow& an, const Spectrum& s, float* out);

}  // namespace halfrate::enc
