#include "pitch.h"

#include "fastmath.h"
#include "numeric.h"
#include "constants.h"
#include "enc_tables.h"

namespace halfrate::enc {

void tracker_reset(PitchTracker& t, int states, int depth) {
	t = PitchTracker{};
	t.state_count = states;
	t.depth = depth < kTrackerMaxDepth ? depth : kTrackerMaxDepth;
}

void tracker_push(PitchTracker& t, const float* cost, const float* transition) {
	int n = t.state_count;
	if (t.frames == 0) {
		for (int i = 0; i < n; i++) t.score[i] = cost[i];
		t.frames = 1;
		return;
	}
	if (n >= 1) {
		std::array<uint8_t, kTrackerStates>& back = t.predecessor[t.frames % (t.depth + 1)];
		float next[kTrackerStates];
		float lowest = kHuge;
		for (int j = 0; j < n; j++) {
			float best = kHuge;
			int from = 0;
			for (int i = 0; i < n; i++) {
				float c = t.score[i] + transition[static_cast<long>(i) * n + j];
				if (c < best) {
					best = c;
					from = i;
				}
			}
			back[j] = static_cast<uint8_t>(from);
			best = best + cost[j];
			next[j] = best;
			if (best < lowest) lowest = best;
		}
		for (int i = 0; i < n; i++) t.score[i] = next[i] - lowest;
	}
	t.frames = t.frames + 1;
}

int tracker_decide(const PitchTracker& t, int lag) {
	int state = 0;
	float best = kHuge;
	for (int i = 0; i < t.state_count; i++)
		if (t.score[i] < best) {
			best = t.score[i];
			state = i;
		}
	int stop = t.frames - lag - 1;
	for (int f = t.frames - 1; f > stop && f >= 0; f--) state = t.predecessor[f % (t.depth + 1)][state];
	return state;
}

namespace {

// Rounds half away from zero and saturates to [-127, 127].
int8_t to_int8(float x) {
	x = x + (x >= 0.0f ? 0.5f : -0.5f);
	int q = core::to_int32_sat(x);
	q = q > -127 ? q : -127;
	q = q < 127 ? q : 127;
	return static_cast<int8_t>(q);
}

}  // namespace

void pitch_net_forward(const float* in, float* out) {
	using namespace tables;
	int8_t input[kNetInputs], hidden1_q[160], hidden2_q[96];
	float hidden1[160], hidden2[96];
	for (int i = 0; i < kNetInputs; i++) {
		int j = i % kNetFeatures;
		input[i] = to_int8((in[i] - kPitchNetMean[j]) * kPitchNetInvStd[j] * kNetInputScale);
	}
	for (int o = 0; o < 160; o++) {
		int32_t acc = 0;
		for (int i = 0; i < kNetInputs; i++) acc += input[i] * kPitchNetWeights1[o * kNetInputs + i];
		float y = kPitchNetBias1[o] + (kPitchNetScale1[o] * kNetLayer1Scale) * static_cast<float>(acc);
		hidden1[o] = y < 0.0f ? 0.0f : y;
	}
	for (int o = 0; o < 160; o++) hidden1_q[o] = to_int8(hidden1[o] * kNetHidden1Scale);
	for (int o = 0; o < 96; o++) {
		int32_t acc = 0;
		for (int i = 0; i < 160; i++) acc += hidden1_q[i] * kPitchNetWeights2[o * 160 + i];
		float y = kPitchNetBias2[o] + (kPitchNetScale2[o] * kNetLayer2Scale) * static_cast<float>(acc);
		hidden2[o] = y < 0.0f ? 0.0f : y;
	}
	for (int o = 0; o < 96; o++) hidden2_q[o] = to_int8(hidden2[o] * kNetHidden2Scale);
	for (int o = 0; o < kNetOutputs; o++) {
		int32_t acc = 0;
		for (int i = 0; i < 96; i++) acc += hidden2_q[i] * kPitchNetWeights3[o * 96 + i];
		out[o] = kPitchNetBias3[o] + (kPitchNetScale3[o] * kNetLayer3Scale) * static_cast<float>(acc);
	}
	// log2 softmax
	float top = kMinusHuge;
	for (int o = 0; o < kNetOutputs; o++) top = out[o] > top ? out[o] : top;
	float sum = 0.0f;
	for (int o = 0; o < kNetOutputs; o++) sum = sum + core::fast_exp2((out[o] - top) * kLog2E);
	float offset = top + core::fast_log2(sum) * kLn2;
	for (int o = 0; o < kNetOutputs; o++) out[o] = out[o] - offset;
}

}  // namespace halfrate::enc
