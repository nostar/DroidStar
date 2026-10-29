#include "options.h"

#include <cmath>
#include <cstring>

namespace halfrate::enc {

namespace {

// One named option: a float field, or element `index` of a per-band array.
struct OptionEntry {
	const char* name;
	float EncoderOptions::*field;
	std::array<float, 9> EncoderOptions::*bands;
	int index;
};

constexpr OptionEntry real(const char* name, float EncoderOptions::*field) { return {name, field, nullptr, 0}; }
constexpr OptionEntry band(const char* name, std::array<float, 9> EncoderOptions::*bands, int index) {
	return {name, nullptr, bands, index};
}

const OptionEntry kOptionTable[] = {
	real("rotated_layout", &EncoderOptions::rotated_layout),
	real("highpass_hz", &EncoderOptions::highpass_hz),
	real("highpass_compensation", &EncoderOptions::highpass_compensation),
	real("highpass_compensation_from_hz", &EncoderOptions::highpass_compensation_from_hz),
	real("pitch_window_offset", &EncoderOptions::pitch_window_offset),
	real("amplitude_window_offset", &EncoderOptions::amplitude_window_offset),
	real("yin_weight", &EncoderOptions::yin_weight),
	real("low_pitch_penalty", &EncoderOptions::low_pitch_penalty),
	real("pitch_jump_cost", &EncoderOptions::pitch_jump_cost),
	real("pitch_jump_free", &EncoderOptions::pitch_jump_free),
	real("octave_jump_cost", &EncoderOptions::octave_jump_cost),
	real("pitch_net_weight", &EncoderOptions::pitch_net_weight),
	real("low_band_threshold_db", &EncoderOptions::low_band_threshold_db),
	real("low_band_penalty_scale", &EncoderOptions::low_band_penalty_scale),
	real("low_band_ignore_first", &EncoderOptions::low_band_ignore_first),
	real("onset_db", &EncoderOptions::onset_db),
	real("onset_low_pitch_scale", &EncoderOptions::onset_low_pitch_scale),
	real("octave_down_tolerance", &EncoderOptions::octave_down_tolerance),
	real("octave_down_after_unvoiced", &EncoderOptions::octave_down_after_unvoiced),
	real("octave_down_lookback", &EncoderOptions::octave_down_lookback),
	real("voicing_threshold", &EncoderOptions::voicing_threshold),
	real("voicing_threshold_slope", &EncoderOptions::voicing_threshold_slope),
	real("voicing_table", &EncoderOptions::voicing_table),
	real("voicing_table_offset", &EncoderOptions::voicing_table_offset),
	real("voicing_band_weight_exponent", &EncoderOptions::voicing_band_weight_exponent),
	real("voicing_hysteresis", &EncoderOptions::voicing_hysteresis),
	real("voicing_hold_unvoiced", &EncoderOptions::voicing_hold_unvoiced),
	real("unvoiced_fit_guard", &EncoderOptions::unvoiced_fit_guard),
	real("low_pitch_voicing_strength", &EncoderOptions::low_pitch_voicing_strength),
	real("low_pitch_voicing_fit", &EncoderOptions::low_pitch_voicing_fit),
	real("low_pitch_voicing_index", &EncoderOptions::low_pitch_voicing_index),
	real("gate_level_db", &EncoderOptions::gate_level_db),
	real("gate_above_floor_db", &EncoderOptions::gate_above_floor_db),
	real("floor_rise", &EncoderOptions::floor_rise),
	real("gate_hang_frames", &EncoderOptions::gate_hang_frames),
	real("gated_as_unvoiced", &EncoderOptions::gated_as_unvoiced),
	real("gated_offset", &EncoderOptions::gated_offset),
	real("silent_voicing_index", &EncoderOptions::silent_voicing_index),
	real("silence_offset", &EncoderOptions::silence_offset),
	real("onset_unvoiced", &EncoderOptions::onset_unvoiced),
	real("onset_offset", &EncoderOptions::onset_offset),
	real("voiced_offset", &EncoderOptions::voiced_offset),
	real("unvoiced_offset", &EncoderOptions::unvoiced_offset),
	real("voiced_amplitude_blend", &EncoderOptions::voiced_amplitude_blend),
	band("voiced_band_offset_0", &EncoderOptions::voiced_band_offset, 0),
	band("voiced_band_offset_1", &EncoderOptions::voiced_band_offset, 1),
	band("voiced_band_offset_2", &EncoderOptions::voiced_band_offset, 2),
	band("voiced_band_offset_3", &EncoderOptions::voiced_band_offset, 3),
	band("voiced_band_offset_4", &EncoderOptions::voiced_band_offset, 4),
	band("voiced_band_offset_5", &EncoderOptions::voiced_band_offset, 5),
	band("voiced_band_offset_6", &EncoderOptions::voiced_band_offset, 6),
	band("voiced_band_offset_7", &EncoderOptions::voiced_band_offset, 7),
	band("voiced_band_offset_8", &EncoderOptions::voiced_band_offset, 8),
	band("unvoiced_band_offset_0", &EncoderOptions::unvoiced_band_offset, 0),
	band("unvoiced_band_offset_1", &EncoderOptions::unvoiced_band_offset, 1),
	band("unvoiced_band_offset_2", &EncoderOptions::unvoiced_band_offset, 2),
	band("unvoiced_band_offset_3", &EncoderOptions::unvoiced_band_offset, 3),
	band("unvoiced_band_offset_4", &EncoderOptions::unvoiced_band_offset, 4),
	band("unvoiced_band_offset_5", &EncoderOptions::unvoiced_band_offset, 5),
	band("unvoiced_band_offset_6", &EncoderOptions::unvoiced_band_offset, 6),
	band("unvoiced_band_offset_7", &EncoderOptions::unvoiced_band_offset, 7),
	band("unvoiced_band_offset_8", &EncoderOptions::unvoiced_band_offset, 8),
	real("unvoiced_pitch_slope", &EncoderOptions::unvoiced_pitch_slope),
	real("unvoiced_pitch_index", &EncoderOptions::unvoiced_pitch_index),
	real("smooth_previous", &EncoderOptions::smooth_previous),
	real("smooth_next", &EncoderOptions::smooth_next),
	real("smooth_next_margin", &EncoderOptions::smooth_next_margin),
	real("peak_threshold", &EncoderOptions::peak_threshold),
	real("peak_reduction", &EncoderOptions::peak_reduction),
	real("gain_bias", &EncoderOptions::gain_bias),
	real("gain_lookahead_weight", &EncoderOptions::gain_lookahead_weight),
	real("shape_weight_exponent", &EncoderOptions::shape_weight_exponent),
	real("shape_high_weight", &EncoderOptions::shape_high_weight),
	real("shape_search_beam", &EncoderOptions::shape_search_beam),
	real("rescore_candidates", &EncoderOptions::rescore_candidates),
	real("rescore_gain_span", &EncoderOptions::rescore_gain_span),
	real("rescore_enhanced", &EncoderOptions::rescore_enhanced),
	real("rescore_enhanced_offset", &EncoderOptions::rescore_enhanced_offset),
	real("rescore_from_spectrum", &EncoderOptions::rescore_from_spectrum),
	real("rescore_floor", &EncoderOptions::rescore_floor),
	real("rescore_weight_exponent", &EncoderOptions::rescore_weight_exponent),
	real("rescore_asymmetry", &EncoderOptions::rescore_asymmetry),
	real("rescore_shape_mix", &EncoderOptions::rescore_shape_mix),
	real("rescore_gain_weight", &EncoderOptions::rescore_gain_weight),
	real("other_voicing_offset_low", &EncoderOptions::other_voicing_offset_low),
	real("other_voicing_offset_high", &EncoderOptions::other_voicing_offset_high),
	real("other_gated_pitch_zero", &EncoderOptions::other_gated_pitch_zero),
	real("other_onset_bridge", &EncoderOptions::other_onset_bridge),
	real("other_onset_bridge_db", &EncoderOptions::other_onset_bridge_db),
	real("other_voiced_island", &EncoderOptions::other_voiced_island),
	real("other_voiced_island_db", &EncoderOptions::other_voiced_island_db),
	real("other_voiced_onset", &EncoderOptions::other_voiced_onset),
	real("other_voiced_onset_db", &EncoderOptions::other_voiced_onset_db),
	real("other_unvoiced_low_cut_hz", &EncoderOptions::other_unvoiced_low_cut_hz),
	real("other_unvoiced_low_cut_db", &EncoderOptions::other_unvoiced_low_cut_db),
	real("other_low_cut_quiet_db", &EncoderOptions::other_low_cut_quiet_db),
	real("other_unvoiced_pitch_index", &EncoderOptions::other_unvoiced_pitch_index),
	real("other_voiced_low", &EncoderOptions::other_voiced_low),
	real("other_voiced_low_db", &EncoderOptions::other_voiced_low_db),
};

}  // namespace

bool set_option(EncoderOptions& options, const char* name, double value) {
	// NaN and infinities would stay in the smoothing and noise floor state after the option is set back
	if (!std::isfinite(value)) return false;
	for (const OptionEntry& entry : kOptionTable) {
		if (std::strcmp(name, entry.name) != 0) continue;
		float v = static_cast<float>(value);
		if (entry.field)
			options.*entry.field = v;
		else
			(options.*entry.bands)[entry.index] = v;
		return true;
	}
	return false;
}

}  // namespace halfrate::enc
