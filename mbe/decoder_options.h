// Decoder options: every tuning value of the decoder and its synthesiser, with their defaults, and setting them
// by name. docs/options.md describes each one.
#pragma once
#include <cstdint>

namespace halfrate::dec {

struct Options {
	// ---- frame decoding and parameter shaping
	float output_gain = 0x1.0a3d7p+1f;          // 2.08: every harmonic amplitude is scaled by this
	float mixed_voiced_share = 0.85f;           // mixed harmonics: voiced weight sqrt(share), noise sqrt(1 - share)
	float mixed_pulse_share = 0.5f;             // stored only, no effect
	float pulse_noise_pulse_weight = 0x1.ccccccp-1f;  // 0.9: pulse weight of pulse-plus-noise harmonics
	float pulse_noise_noise_weight = 0x1.3d70a4p-1f;  // 0.62: noise weight of pulse-plus-noise harmonics
	float voiced_weight = 1.0f;                 // voiced weight of voiced harmonics (and scale of mixed ones)
	float high_band_start = 0x1.4cccccp-1f;     // 0.65: the high-band tilt starts at this fraction of L
	float high_band_tilt = 0x1.19999ap-1f;      // 0.55: log2 gain reached at the top harmonic; 0 = off
	int32_t pulse_pitch_index = 119;            // pulse frames dequantise with this pitch index (< 0: their own)
	float pulse_window = 88.0f;                 // a pulse's gain falls linearly to 0 over this many samples
	int32_t pulse_window_center = 62;           // single pulses: distance is counted from this pitch index
	int32_t pulse_enable = 1;                   // 0: pulse codes decode as noise and no pulses are placed
	int32_t pulse_position_low = 98;            // single pulse position = this + pitch index (voicing 18..23)
	int32_t pulse_position_high = 16;           // the same for voicing 26..31
	float pulse_pair_first_start = 101.0f;      // pulse pairs (voicing 24, 25): first position start + step * (b0 / 11)
	float pulse_pair_first_step = 0x1.79999ap+3f;  // 11.8
	float pulse_pair_second_start = 21.0f;      // second position start + step * (b0 % 11)
	float pulse_pair_second_step = 0x1.79999ap+3f;
	float phase_dispersion = 0x1.333334p-2f;    // 0.3: weight of the phase term from the log-amplitude slope
	float phase_level_offset = 0.0f;            // added to each log amplitude the phase term reads
	int32_t phase_taps = 19;                    // taps of the phase term (clamped to 1..32)
	int32_t phase_odd_taps = 1;                 // nonzero: odd taps only
	float silence_gain = 0x1.333334p-2f;        // 0.3: amplitude scale of silence frames
	int32_t silence_predictor = 3;              // what a silence frame does to the predictor (docs/options.md)
	int32_t rotated_layout = 1;                 // 1: frames arrive in the rotated bit layout
	float tone_gain = 1.0f;                     // tone frame level scale

	// ---- enhancement (core::EnhanceOptions)
	int32_t enhance = 1;                        // the formant enhancement runs only when this is exactly 1
	float enhance_reserved = 1.0f;              // stored only, no effect
	float enhance_strength = 0.5f;
	float enhance_min_weight = 0x1.333334p-2f;  // 0.3
	float enhance_max_weight = 2.0f;
	int32_t enhance_bypass_divisor = 0;
	float enhance_power_high = 0x1.b33334p-1f;  // 0.85
	float enhance_power_low = 0x1.333334p-1f;   // 0.6
	float enhance_split_frequency = 0.125f;     // cycles per sample

	// ---- erasures
	int32_t repeat_max = 3;                     // erased frames that repeat the last good one before muting
	float repeat_gain = 1.0f;                   // amplitude scale per repeat (compounding)
	float repeat_high_gain = 1.0f;              // extra scale per repeat at half the sample rate (compounding)
	float repeat_high_start = 0.3f;             // the lift rises in dB from here, cycles per sample (2.4 kHz)
	float comfort_noise_level = 1.0f;           // rms of the comfort noise after muting
	int32_t fec_error_threshold = 6;            // stored only, no effect on this frame format

	// ---- output stage
	float highpass_hz = 0.0f;                   // cutoff of the 4th-order high-pass; <= 0 = off
	int32_t frame_limiter = 1;                  // nonzero: the frame level limiter runs
	float frame_limiter_knee_db = 78.5f;        // frame rms (dB) above which it compresses
	float frame_limiter_ratio = 0x1.333334p-2f; // 0.3: slope above the knee
	float limiter_threshold = 28000.0f;         // soft limiter start (towards 32766); <= 0 = off
	float unvoiced_level_db = 0.0f;             // stored only, no effect
	float unvoiced_tilt = 0.0f;                 // stored only, no effect

	// ---- synthesiser
	float unvoiced_gain = 0x1.b33334p-1f;       // 0.85
	float pulse_gain = 1.0f;
	float pulse_length = 48.0f;                 // half width of the pulse's cosine window, samples
	int32_t pulse_minimum_phase = 0;            // nonzero: minimum-phase pulse shape, else zero phase
	int32_t pulse_shift = 0;                    // samples added to each pulse position
	float voiced_fade_start = 0.0f;             // per-harmonic oscillators: fade-out of a harmonic that stops
	float voiced_fade_end = 160.0f;
	int32_t noise_offset = 0;                   // single-block noise: sample offset (clamped to 0..160)
	int32_t initial_phase = 0;                  // oscillator start phases: 0 zero, 1 random, 2 quadratic
	float unvoiced_top_extend = 0.0f;           // single-block noise and pulses: reach above harmonic L
	int32_t unvoiced_low_cut = 1;               // nonzero: no noise below harmonic 1/2
	int32_t phase_tracking = 1;                 // 1: one fundamental phase track, else per-harmonic oscillators
	int32_t amplitude_log_interp = 1;           // nonzero: amplitudes glide log-linearly
	int32_t amplitude_hold = 0;
	int32_t amplitude_frequency_map = 0;
	int32_t noise_method = 1;                   // 1: four 40-sample blocks, 0: one 320-sample block, else none
	int32_t noise_coherent_bins = 64;           // noise bins below this keep a drifting phase
	float noise_phase_jitter = 0.5f;
	int32_t noise_white = 0;
	float noise_gain_bin1 = 0.0f;
	float noise_gain_bins23 = 0x1.333334p-3f;   // 0.15
	float pulse_highpass_hz = 0.0f;
	int32_t noise_seed = 0;
	float voicing_off_start = 5.0f;             // crossfade window (samples) of a harmonic losing voicing
	float voicing_off_end = 50.0f;
	float voicing_on_start = 55.0f;             // and of one gaining it
	float voicing_on_end = 90.0f;
	float harmonic_fade_start = 0.0f;           // and of one present in only one of the two frames
	float harmonic_fade_end = 160.0f;
	int32_t pitch_glide_start = 0;              // the fundamental glides over these samples
	int32_t pitch_glide_end = 160;
	int32_t amplitude_glide_start = 0;          // the amplitudes glide over these samples
	int32_t amplitude_glide_end = 160;
};

// What setting an option does besides storing the value.
enum class SetEffect { none, reset_synth };

// Sets option name to value: float options take (float)value, integer options value rounded half away from zero
// (saturating), some are clamped (docs/options.md). Returns false for an unknown name.
bool set_option(Options& options, const char* name, double value, SetEffect* effect);

}  // namespace halfrate::dec
