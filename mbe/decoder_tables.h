// Constant tables of the decoder only.
#pragma once
#include <cstdint>

namespace halfrate::dec::tables {

// How a harmonic is synthesised, per voicing index and frequency slot (16 slots of 1/32 cycle per sample).
enum HarmonicClass : char {
	kVoiced = 'V',
	kUnvoiced = 'U',
	kMixed = 'M',       // voiced and noise, shared by mixed_voiced_share
	kPulse = 'P',       // a pulse (noise when pulses are off)
	kPulseNoise = 'Q',  // a pulse and noise
};
// kHarmonicClass[voicing index][slot]; silence frames use row kSilenceClassRow.
extern const char kHarmonicClass[32][17];
constexpr int kSilenceClassRow = 16;

// Weights of the phase term's taps: [k] = 1/k for k = 1..32 ([0] unused).
extern const float kPhaseTapWeight[33];

// Dual tones, Hz. Tone indices 0x80..0x8f: low kKeypadLow[kKeypadLowIndex[i]], high kKeypadHigh[kKeypadHighIndex[i]].
extern const float kKeypadLow[4];
extern const float kKeypadHigh[4];
extern const int32_t kKeypadLowIndex[16];
extern const int32_t kKeypadHighIndex[16];
// Tone indices 0x90..0x9f: kToneSetLow[kToneSetIndex[i][0]] and kToneSetHigh[kToneSetIndex[i][1]].
extern const float kToneSetLow[4];
extern const float kToneSetHigh[4];
extern const int8_t kToneSetIndex[16][2];

}  // namespace halfrate::dec::tables
