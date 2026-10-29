// Numeric constants of the encoder. They are written as hex floats because the output depends on their exact
// values (most are the nearest float to the decimal value in the comment, some are not).
#pragma once

namespace halfrate::enc {

constexpr float kTiny = 0x1.197998p-40f;            // 1e-12: energy floor added before divisions and logs
constexpr float kMinusTiny = -0x1.197998p-40f;      // -1e-12
constexpr float kAmplitudeFloor = 0x1.12e0bep-30f;  // 1e-9: added to an amplitude before its log2
constexpr float kEnergyFloor = 0x1.203afap-50f;     // 1e-15
constexpr float kHuge = 0x1.c363ccp+127f;           // 3e38: "no candidate yet" for minimum searches
constexpr float kMinusHuge = -0x1.c363ccp+127f;     // -3e38
constexpr float kVeryLarge = 0x1.93e594p+99f;       // 1e30
constexpr float kVeryNegative = -0x1.93e594p+99f;   // -1e30: start value of maximum searches
constexpr float kDbPerLog2 = 0x1.815182p+1f;        // 10 log10(2): log2 energy to dB
constexpr float kTwoPi = 0x1.921fb6p+2f;
constexpr float kInvSqrt2 = 0x1.6a09e6p-1f;         // high-pass design: 1 / sqrt(2)
constexpr float kHalfInvSqrt2 = 0x1.6a09e6p-2f;     // shape basis: scale of the pair difference
constexpr float kOctaveMatch = 0x1.eb851ep-5f;      // 0.06: log2 distance accepted as "one octave below"
constexpr float kFloorCreep = 0x1.99999ap-5f;       // 0.05 dB: noise floor rise per frame
constexpr float kHzToVoicingBand = 0x1.0624dep-9f;  // 0.002: Hz to the 500 Hz voicing bands
constexpr float kOneSeventh = 0x1.24924ap-3f;       // 1/7
constexpr float kTwoSevenths = 0x1.24924ap-2f;      // 2/7
constexpr float kVoicingCostFloor = 0x1.47ae14p-6f; // 0.02: cost of a voiced band that fits
constexpr float kCompensationFloor = 0x1.0c6f7ap-20f;  // 1e-6
constexpr float kLevelFeatureScale = 0x1.99999ap-4f;   // 0.1: level (dB) as a pitch-net feature
constexpr float kOctaveJumpScale = 0x1.aaaaaap+2f;     // 6.6667: log2 distance minus one octave, scaled
constexpr float kOctaveJumpExponent = -0x1.715476p+0f; // -1/ln 2: Gaussian of the octave-jump cost
constexpr float kOctaveLookbackTolerance = 0x1.47ae14p-4f;  // 0.08: log2 distance
constexpr float kHighShapeStart = 0x1.19999ap-1f;      // 0.55: share of the harmonics where the high weight starts
constexpr float kMinusHighShapeStart = -0x1.19999ap-1f;
constexpr float kHighShapeScale = 0x1.1c71c8p+1f;      // 2.2222 = 1 / (1 - 0.55)
constexpr float kRescoreDistanceFloor = 0x1.0c6f7ap-20f;  // 1e-6
constexpr float kRescoreBandFloor = 0x1.79ca1p-67f;       // 1e-20
constexpr float kHighPassFlush = 0x1.4484cp-100f;      // 1e-30: high-pass state below this is flushed to 0
constexpr float kStartupAmplitude = -0x1.8p+3f;        // -12: log2 amplitudes of the frames before the first
constexpr float kInitialPreviousFundamental = 0x1.47ae14p-7f;  // 0.01
constexpr float kToneFundamental = 0x1p-5f;            // 1/32: fundamental of the fixed-pitch (silence) frame

// pitch network
constexpr float kNetInputScale = 0x1.ecc05ep+3f;
constexpr float kNetLayer1Scale = 0x1.0a001p-4f;
constexpr float kNetLayer2Scale = 0x1.918048p-3f;
constexpr float kNetLayer3Scale = 0x1.1defbap-3f;
constexpr float kNetHidden1Scale = 0x1.467474p+2f;     // hidden layer 1 output to int8
constexpr float kNetHidden2Scale = 0x1.ca6546p+2f;     // hidden layer 2 output to int8
constexpr float kLog2E = 0x1.715476p+0f;
constexpr float kLn2 = 0x1.62e43p-1f;

}  // namespace halfrate::enc
