#include "decoder_options.h"

#include <cstring>

#include "numeric.h"

namespace halfrate::dec {

namespace {

enum class Kind { real, integer, integer_resets_synth };

struct OptionEntry {
	const char* name;
	Kind kind;
	float Options::*real;
	int32_t Options::*integer;
	int32_t low, high;  // integer options are clamped to [low, high]
};

constexpr int32_t kIntMin = INT32_MIN, kIntMax = INT32_MAX;

constexpr OptionEntry real(const char* name, float Options::*field) {
	return {name, Kind::real, field, nullptr, 0, 0};
}
constexpr OptionEntry integer(const char* name, int32_t Options::*field, int32_t low = kIntMin, int32_t high = kIntMax) {
	return {name, Kind::integer, nullptr, field, low, high};
}
constexpr OptionEntry resetting(const char* name, int32_t Options::*field) {
	return {name, Kind::integer_resets_synth, nullptr, field, kIntMin, kIntMax};
}

const OptionEntry kOptionTable[] = {
	real("output_gain", &Options::output_gain),
	real("mixed_voiced_share", &Options::mixed_voiced_share),
	real("mixed_pulse_share", &Options::mixed_pulse_share),
	real("pulse_noise_pulse_weight", &Options::pulse_noise_pulse_weight),
	real("pulse_noise_noise_weight", &Options::pulse_noise_noise_weight),
	real("voiced_weight", &Options::voiced_weight),
	real("high_band_start", &Options::high_band_start),
	real("high_band_tilt", &Options::high_band_tilt),
	// replaces a voice frame's pitch index, so it must be a voice pitch: at most 119 (120 and up are tones and
	// erasures, which have no fundamental)
	integer("pulse_pitch_index", &Options::pulse_pitch_index, kIntMin, 119),
	real("pulse_window", &Options::pulse_window),
	integer("pulse_window_center", &Options::pulse_window_center),
	integer("pulse_enable", &Options::pulse_enable),
	integer("pulse_position_low", &Options::pulse_position_low),
	integer("pulse_position_high", &Options::pulse_position_high),
	real("pulse_pair_first_start", &Options::pulse_pair_first_start),
	real("pulse_pair_first_step", &Options::pulse_pair_first_step),
	real("pulse_pair_second_start", &Options::pulse_pair_second_start),
	real("pulse_pair_second_step", &Options::pulse_pair_second_step),
	real("phase_dispersion", &Options::phase_dispersion),
	real("phase_level_offset", &Options::phase_level_offset),
	// the phase term's tap weights are a table of 32
	integer("phase_taps", &Options::phase_taps, 1, 32),
	integer("phase_odd_taps", &Options::phase_odd_taps),
	real("silence_gain", &Options::silence_gain),
	integer("silence_predictor", &Options::silence_predictor),
	integer("rotated_layout", &Options::rotated_layout),
	real("tone_gain", &Options::tone_gain),
	integer("enhance", &Options::enhance),
	real("enhance_reserved", &Options::enhance_reserved),
	real("enhance_strength", &Options::enhance_strength),
	real("enhance_min_weight", &Options::enhance_min_weight),
	real("enhance_max_weight", &Options::enhance_max_weight),
	integer("enhance_bypass_divisor", &Options::enhance_bypass_divisor),
	real("enhance_power_high", &Options::enhance_power_high),
	real("enhance_power_low", &Options::enhance_power_low),
	real("enhance_split_frequency", &Options::enhance_split_frequency),
	integer("repeat_max", &Options::repeat_max),
	real("repeat_gain", &Options::repeat_gain),
	real("repeat_high_gain", &Options::repeat_high_gain),
	real("repeat_high_start", &Options::repeat_high_start),
	real("comfort_noise_level", &Options::comfort_noise_level),
	integer("fec_error_threshold", &Options::fec_error_threshold),
	real("highpass_hz", &Options::highpass_hz),
	integer("frame_limiter", &Options::frame_limiter),
	real("frame_limiter_knee_db", &Options::frame_limiter_knee_db),
	real("frame_limiter_ratio", &Options::frame_limiter_ratio),
	real("limiter_threshold", &Options::limiter_threshold),
	real("unvoiced_level_db", &Options::unvoiced_level_db),
	real("unvoiced_tilt", &Options::unvoiced_tilt),
	real("unvoiced_gain", &Options::unvoiced_gain),
	real("pulse_gain", &Options::pulse_gain),
	real("pulse_length", &Options::pulse_length),
	integer("pulse_minimum_phase", &Options::pulse_minimum_phase),
	integer("pulse_shift", &Options::pulse_shift),
	real("voiced_fade_start", &Options::voiced_fade_start),
	real("voiced_fade_end", &Options::voiced_fade_end),
	// the single-block noise (320 samples) is added at this offset into 160 + 320 samples of output and overlap
	integer("noise_offset", &Options::noise_offset, 0, 160),
	resetting("initial_phase", &Options::initial_phase),
	real("unvoiced_top_extend", &Options::unvoiced_top_extend),
	integer("unvoiced_low_cut", &Options::unvoiced_low_cut),
	integer("phase_tracking", &Options::phase_tracking),
	integer("amplitude_log_interp", &Options::amplitude_log_interp),
	integer("amplitude_hold", &Options::amplitude_hold),
	integer("amplitude_frequency_map", &Options::amplitude_frequency_map),
	integer("noise_method", &Options::noise_method),
	integer("noise_coherent_bins", &Options::noise_coherent_bins),
	real("noise_phase_jitter", &Options::noise_phase_jitter),
	integer("noise_white", &Options::noise_white),
	real("noise_gain_bin1", &Options::noise_gain_bin1),
	real("noise_gain_bins23", &Options::noise_gain_bins23),
	real("pulse_highpass_hz", &Options::pulse_highpass_hz),
	resetting("noise_seed", &Options::noise_seed),
	real("voicing_off_start", &Options::voicing_off_start),
	real("voicing_off_end", &Options::voicing_off_end),
	real("voicing_on_start", &Options::voicing_on_start),
	real("voicing_on_end", &Options::voicing_on_end),
	real("harmonic_fade_start", &Options::harmonic_fade_start),
	real("harmonic_fade_end", &Options::harmonic_fade_end),
	integer("pitch_glide_start", &Options::pitch_glide_start),
	integer("pitch_glide_end", &Options::pitch_glide_end),
	integer("amplitude_glide_start", &Options::amplitude_glide_start),
	integer("amplitude_glide_end", &Options::amplitude_glide_end),
};

}  // namespace

bool set_option(Options& options, const char* name, double value, SetEffect* effect) {
	if (effect) *effect = SetEffect::none;
	if (!name) return false;
	// NaN and infinities would stay in the filter and predictor state after the option is set back
	if (!std::isfinite(value)) return false;
	double half = value >= 0.0 ? 0.5 : -0.5;
	int32_t rounded = core::to_int32_sat(value + half);
	for (const OptionEntry& entry : kOptionTable) {
		if (std::strcmp(name, entry.name) != 0) continue;
		if (entry.kind == Kind::real) {
			options.*entry.real = static_cast<float>(value);
		} else {
			int32_t v = rounded < entry.low ? entry.low : rounded;
			options.*entry.integer = v > entry.high ? entry.high : v;
			if (entry.kind == Kind::integer_resets_synth && effect) *effect = SetEffect::reset_synth;
		}
		return true;
	}
	return false;
}

}  // namespace halfrate::dec
