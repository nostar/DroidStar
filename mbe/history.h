// The encoder's input history and the analysis of its last three frames, kept together in one block.
//
// The analysis reads the input around a position set by two options (pitch_window_offset and
// amplitude_window_offset). With the defaults the reads stay within the 1024 input samples; with positive
// offsets they run past the newest sample into what follows in this block (the frame counter, then the analysis
// slots), and the output depends on those values. sample() returns them the same way: index i reads the i-th
// 32-bit word of the block as a float. The options are clamped so that every read stays inside the block.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "analysis.h"

namespace halfrate::enc {

constexpr int kHistorySamples = 1024;
constexpr int kFrameSamples = 160;

// The analysis of one input frame.
struct AnalysisSlot {
	Spectrum pitch_spectrum;      // 256-point window: pitch search
	Spectrum amplitude_spectrum;  // 160-point window: harmonic amplitudes
	float level_db;               // energy of the pitch spectrum's fitted bins, dB
	float peak;                   // largest sample magnitude around the amplitude window
	int32_t low_band;             // 1: weak energy below 250 Hz (only set with low_band_threshold_db < 0)
	std::array<float, kPitchCount> fit_cost;  // harmonic fit cost per grid pitch
};

struct SignalHistory {
	std::array<float, kHistorySamples> samples;  // newest 160 at the end, scaled to +-1
	int64_t frame_count;                         // frames pushed so far
	std::array<AnalysisSlot, 3> slots;           // frame n is in slots[n % 3]

	static constexpr int kWords = kHistorySamples + 2 + 3 * (sizeof(AnalysisSlot) / 4);

	// 32-bit word i of this block as a float, 0 <= i < kWords.
	float sample(int i) const {
		if (static_cast<unsigned>(i) < static_cast<unsigned>(kHistorySamples)) return samples[i];
		float f;
		std::memcpy(&f, reinterpret_cast<const unsigned char*>(this) + 4 * static_cast<size_t>(i), 4);
		return f;
	}
};

static_assert(sizeof(AnalysisSlot) == 1151 * 4, "analysis slot: 1151 words, no padding");
static_assert(sizeof(SignalHistory) >= SignalHistory::kWords * 4, "history block holds every word");
static_assert(offsetof(SignalHistory, frame_count) == kHistorySamples * 4, "frame counter right after the samples");
static_assert(offsetof(SignalHistory, slots) == (kHistorySamples + 2) * 4, "slots right after the counter");

}  // namespace halfrate::enc
