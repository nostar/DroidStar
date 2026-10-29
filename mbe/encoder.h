// The encoder: 160 input samples in, the nine fields of one frame out, two frames behind the input.
//
// Each input frame is high-passed, analysed (two windowed spectra, the per-pitch harmonic fit and YIN costs, the
// pitch network) and pushed into the pitch tracker. The frame two frames back is then decided: pitch from the
// tracker, the noise gate, voicing from the harmonic fit, harmonic amplitudes from the amplitude spectrum, and the
// quantiser chooses gain and shape fields against the decoder's own prediction, which it tracks with a
// dequantiser state of its own.
#pragma once
#include <array>
#include <cstdint>

#include "dequant.h"
#include "frame.h"
#include "analysis.h"
#include "history.h"
#include "options.h"
#include "pitch.h"
#include "shape_quantizer.h"

namespace halfrate::enc {

constexpr int kEncoderDelay = 2;

// Input high-pass: a second-order section (coefficients for the corner the options name).
struct HighPass {
	float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
	float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;
	float designed_hz = -1.0f;  // corner the coefficients are for
};

// What the band-energy rescoring of the shape candidates compares against: per harmonic its band (of the 18
// rescoring bands) and weight, and the reference energy per band.
struct RescoreInput {
	int32_t enabled = 0;
	std::array<int32_t, kMaxFitHarmonics> band{};
	std::array<float, kMaxFitHarmonics> weight{};
	std::array<float, 18> band_energy{};
};

struct Encoder {
	EncoderOptions options;
	bool transitions_dirty = true;  // the tracker's transition costs depend on options

	// pitch grid (the codebook's pitch indices)
	std::array<float, kPitchCount> inverse_fundamental{};   // samples per period
	std::array<float, kPitchCount> log2_fundamental{};
	std::array<float, kPitchCount> log2_ratio_to_lowest{};  // log2 f[0] - log2 f[i]: octaves below index 0
	std::array<int32_t, kPitchCount> octave_below{};        // index one octave lower (-1: none within 0.06)
	std::array<float, kPitchCount * kPitchCount> transition{};  // tracker cost [from * 120 + to]

	AnalysisWindow long_window;   // 256 points: pitch
	AnalysisWindow short_window;  // 160 points: amplitudes
	ShapeBasis shape_basis;

	SignalHistory history;
	PitchTracker tracker;

	// gate
	float noise_floor_db = 0.0f;
	int32_t gate_hang = 0;

	// the decoder's predictor state, and what the gain choice looks ahead to
	core::DequantState predictor;
	std::array<float, 2> lookahead_gain{};  // expected gain targets of the next two frames
	int32_t lookahead_count = 0;            // how many of them are set (0 or 2)

	// previous frame
	std::array<float, kMaxFitHarmonics> previous_amplitudes{};
	float previous_fundamental = 0.0f;
	int32_t previous_harmonic_count = 0;
	int32_t have_previous = 0;       // previous_amplitudes hold a speech frame
	int32_t previous_voicing = -1;   // its voicing index for the hysteresis, -1: none
	int32_t after_silence = 0;       // the previous frame was silent
	int32_t low_band = 0;            // the frame being voiced has a weak low band
	int32_t previous_unvoiced = 1;   // the previous frame was unvoiced or gated

	HighPass highpass;

	// pitch network input: the last six frames' features (frame n in features[n % 6])
	std::array<std::array<float, kNetFeatures>, kNetFrames> features{};
	std::array<float, kNetInputs> net_input{};
	std::array<float, kNetOutputs> net_output{};

	// per grid pitch, this frame
	std::array<float, kPitchCount> fit_cost{};
	std::array<float, kPitchCount> yin_cost{};
	std::array<float, kPitchCount> pitch_cost{};
};

// Builds the tables and resets the state (the options keep their values).
void encoder_init(Encoder& e);
void encoder_reset(Encoder& e);

// Encodes 160 samples; fields gets the frame of the input two frames back.
void encode_frame(Encoder& e, const int16_t* pcm, core::FrameFields& fields);

}  // namespace halfrate::enc
