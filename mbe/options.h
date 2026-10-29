// Encoder options (docs/options.md). Every option is a float, set by name with set_option; options that act as
// counts, indices or switches are truncated toward zero where they are used, as the encoder always has.
#pragma once
#include <array>

namespace halfrate::enc {

struct EncoderOptions {
	// frame and input
	float rotated_layout = 0x1p+0f;              // 1: send frame bits 40..43 rotated (frame.h); else standard
	float highpass_hz = 0x1.0da482p+7f;          // input high-pass corner (Hz), off at 0 or below
	float highpass_compensation = 0x1p+0f;       // amplitude lift that undoes the high-pass, 0 = off
	float highpass_compensation_from_hz = 0.0f;  // harmonics below this frequency get no compensation
	float pitch_window_offset = -0x1.ep+7f;      // pitch analysis centre minus 944 (samples into the history)
	float amplitude_window_offset = -0x1.e2p+7f; // amplitude analysis centre, likewise
	// pitch search
	float yin_weight = 0x1.3ab3ccp-1f;           // weight of the YIN cost against the harmonic fit cost
	float low_pitch_penalty = 0x1.a0d09ep-3f;    // cost per octave below the highest grid pitch
	float pitch_jump_cost = 0x1.ccccccp-1f;      // tracker cost per octave of pitch change beyond the free range
	float pitch_jump_free = 0x1.99999ap-4f;      // pitch change (octaves) that costs nothing
	float octave_jump_cost = 0x1p+0f;            // extra cost of a jump of about one octave
	float pitch_net_weight = 0x1.99999ap-4f;     // weight of the pitch network's log posterior, 0 = off
	float low_band_threshold_db = -0x1.ep+3f;    // < 0: a frame whose low band trails by more than -this is "weak low band"
	float low_band_penalty_scale = 0x1.333334p-1f;  // low_pitch_penalty factor on such frames
	float low_band_ignore_first = 0x1p+0f;       // > 0: band 0 does not count for voicing on such frames
	float onset_db = 0.0f;                       // > 0: level rise over two frames that counts as an onset
	float onset_low_pitch_scale = 0x1p+0f;       // low_pitch_penalty factor at an onset
	float octave_down_tolerance = 0.0f;          // > 0: take the octave below if its fit cost is within this
	float octave_down_after_unvoiced = 0x1p+0f;  // tolerance divisor after an unvoiced or silent frame
	float octave_down_lookback = 0.0f;           // > 0: after unvoiced, take the octave below if a later frame has it
	// voicing
	float voicing_threshold = 0x1.5c28f6p-1f;    // band fit ratio below which a band is voiced
	float voicing_threshold_slope = 0.0f;        // threshold change across the bands
	float voicing_table = 0x1p+0f;               // > 0: thresholds from the pitch- and fit-dependent table instead
	float voicing_table_offset = 0x1.99999ap-5f; // added to the table thresholds
	float voicing_band_weight_exponent = 0x1.96966p-3f;  // band weight = (band energy share)^this, 0 = equal
	float voicing_hysteresis = 0.0f;             // cost per band that differs from the previous frame's pattern
	float voicing_hold_unvoiced = 0.0f;          // > 0: the hysteresis also follows unvoiced frames
	float unvoiced_fit_guard = 0.0f;             // all-unvoiced choice is retried among the voiced patterns below this fit
	float low_pitch_voicing_strength = 0.0f;     // > 0: raise the upper bands' thresholds on good low-pitch fits
	float low_pitch_voicing_fit = 0x1.333334p-2f;  // fit cost below which that applies
	float low_pitch_voicing_index = 0x1.ep+6f;     // pitch indices below which that applies
	// gate
	float gate_level_db = -0x1.17fe1ap+6f;       // frames below this level are silent
	float gate_above_floor_db = 0x1.8bf50ep+2f;  // or below the noise floor plus this
	float floor_rise = 0x1.47ae14p-6f;           // share of the level excess the noise floor rises by per frame
	float gate_hang_frames = 0x1p+3f;            // frames the gate stays open after speech
	float gated_as_unvoiced = 0x1p+0f;           // > 0: gated frames are coded as unvoiced speech, not silence
	float gated_offset = -0x1.8p+1f;             // log2 amplitude change of gated frames coded as speech
	float silent_voicing_index = 0x1p+4f;        // voicing field of silent frames (0..31)
	float silence_offset = 0x1.77c7c6p-1f;       // log2 amplitude offset of silent frames
	float onset_unvoiced = 0.0f;                 // > 0: the frame after silence is unvoiced
	float onset_offset = 0.0f;                   // log2 amplitude change of the frame after silence
	// harmonic amplitudes
	float voiced_offset = 0x1.eb3334p+3f;        // log2 offset of voiced harmonics
	float unvoiced_offset = 0x1.e471e4p+3f;      // log2 offset of unvoiced harmonics
	float voiced_amplitude_blend = 0x1p-1f;      // voiced amplitude: fitted (0) to spectrum rms (1)
	std::array<float, 9> voiced_band_offset = {-0x1p-2f, -0x1.eb851ep-6f, 0, 0, 0, 0, 0, 0, 0};  // per 500 Hz
	std::array<float, 9> unvoiced_band_offset = {-0x1.cadc74p-3f, -0x1.4f13cep-5f, -0x1.41b196p-3f, 0,
	                                             -0x1.3e425ap-6f,  0x1.a7c7c6p-3f, 0, 0, 0};
	float unvoiced_pitch_slope = 0.0f;           // unvoiced log2 offset per octave of pitch
	float unvoiced_pitch_index = 0x1.dcp+6f;     // >= 0: pitch index sent with an all-unvoiced pattern
	float smooth_previous = 0x1.99999ap-5f;      // weight of the previous frame's amplitudes
	float smooth_next = 0.0f;                    // weight of the next frame's amplitudes
	float smooth_next_margin = 0x1p+0f;          // the next frame counts as at least this much below this one
	float peak_threshold = 0x1.e66666p-1f;       // frames with a sample peak at or above this ...
	float peak_reduction = 0x1.333334p-3f;       // ... are lowered by this (log2)
	// quantiser
	float gain_bias = -0x1.666666p-3f;           // log2 offset of the gain target
	float gain_lookahead_weight = 0x1.dbcdc2p-2f;  // > 0: gain choice looks two frames ahead, this weight
	float shape_weight_exponent = 0x1.155bb2p-1f;  // harmonic weight = 2^((a - max) * this), 0 = equal
	float shape_high_weight = 0.0f;              // log2 weight tilt of the top 45% of the harmonics
	float shape_search_beam = 0x1p+3f;           // candidates kept per search stage (1..16)
	float rescore_candidates = 0x1p+1f;          // shape candidates re-scored (1 = none, up to 16)
	float rescore_gain_span = 0x1p+0f;           // gain indices either side re-scored with them, < 0 = none
	float rescore_enhanced = 0.0f;               // > 0: score against the enhanced decoded amplitudes (> 1.5 tilted)
	float rescore_enhanced_offset = 0.0f;        // log2 offset of the target in that score
	float rescore_from_spectrum = 0.0f;          // != 0: reference band energies from the spectrum, not the amplitudes
	float rescore_floor = 0x1.4bbf9p+3f;         // band energy floor, log2 below the strongest band
	float rescore_weight_exponent = 0.0f;        // > 0: band weight = (band energy share)^this
	float rescore_asymmetry = 0x1p+0f;           // weight of bands coded too loud
	float rescore_shape_mix = 0.0f;              // weight of the shape distortion in the band score
	float rescore_gain_weight = 0.0f;            // cost of moving the gain index
	// Options that belong to other codec modes: accepted by name and kept, but nothing in this
	// codec reads them (see docs/options.md).
	float other_voicing_offset_low = 0.0f;
	float other_voicing_offset_high = 0.0f;
	float other_gated_pitch_zero = 0.0f;
	float other_onset_bridge = 0.0f;
	float other_onset_bridge_db = 0x1.4p+3f;
	float other_voiced_island = 0.0f;
	float other_voiced_island_db = 0x1.8p+2f;
	float other_voiced_onset = 0.0f;
	float other_voiced_onset_db = 0x1.4p+4f;
	float other_unvoiced_low_cut_hz = 0.0f;
	float other_unvoiced_low_cut_db = 0x1.4p+4f;
	float other_low_cut_quiet_db = 0.0f;
	float other_unvoiced_pitch_index = -0x1p+0f;
	float other_voiced_low = 0.0f;
	float other_voiced_low_db = 0x1.ep+3f;
};

// Sets the option `name` to value (converted to float). Returns false for an unknown name.
bool set_option(EncoderOptions& options, const char* name, double value);

}  // namespace halfrate::enc
