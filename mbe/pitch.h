// Pitch tracking: a Viterbi tracker over the pitch grid that decides a frame's pitch a fixed number of frames
// late, and the pitch network that adds a learned log-posterior to the per-frame pitch costs.
#pragma once
#include <array>
#include <cstdint>

namespace halfrate::enc {

constexpr int kTrackerStates = 120;
constexpr int kTrackerMaxDepth = 4;
constexpr int kNetFeatures = 305;       // per frame: 120 fit costs, 120 YIN values, 64 band log energies, level
constexpr int kNetFrames = 6;
constexpr int kNetInputs = kNetFeatures * kNetFrames;
constexpr int kNetOutputs = 121;

// Viterbi tracker: the best path cost ending in each state, and for each of the last depth + 1 frames the best
// predecessor of each state (a ring indexed by frame % (depth + 1)).
struct PitchTracker {
	int32_t state_count = 0;
	int32_t depth = 0;
	int32_t frames = 0;  // frames pushed
	std::array<float, kTrackerStates> score{};
	std::array<std::array<uint8_t, kTrackerStates>, kTrackerMaxDepth + 1> predecessor{};
};

void tracker_reset(PitchTracker& t, int states, int depth);

// Adds a frame: cost[state], transition[from * states + to]. Scores are kept relative to the best one.
void tracker_push(PitchTracker& t, const float* cost, const float* transition);

// The state on the best path, `lag` frames before the newest (lag <= depth).
int tracker_decide(const PitchTracker& t, int lag);

// The pitch network over six frames of features (oldest first): out[0..120] are log2 posteriors.
void pitch_net_forward(const float* in, float* out);

}  // namespace halfrate::enc
