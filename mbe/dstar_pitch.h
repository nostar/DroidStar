/*
 * QSO One local addition (2026-10-06; listed in ../../../../NOTICE.md). GPL v3 or later.
 *
 * D-Star (AMBE 3600x2400) constants measured on a real DVSI AMBE-3000 chip (a DVMEGA
 * DVstick 30 used as a sealed box: frames in, audio out; no DVSI code or firmware read).
 * See QSO One Research/DSTAR_STICK_2026-10-06.md.
 *
 * 1. The pitch scale. mbelib decodes the pitch code b0 with its "w0 guess"
 *    f0 = 2^(-4.311767578125 - 0.021336 (b0 + 0.5)). The chip plays b0 at
 *    f0 = 2^(-DSTAR_F0_C0 - DSTAR_F0_C1 (b0 + 0.5))  (cycles per sample), measured on
 *    29 codes from 4 to 116: the fit residual is 0.3 cents. The earlier "two codes
 *    higher" rule (2026-10-06 morning) was right only near 190 Hz; it was 1-2 % wrong on
 *    male voices.
 * 2. The spectral prediction. Each frame's log spectral amplitudes are coded as a
 *    residual from DSTAR_SPEC_PRED x the previous frame's. mbelib uses 0.65 (the AMBE+2 /
 *    DMR value). Decoding the chip's own frames, 0.8 matches the chip best (band-envelope
 *    error 10.8 -> 6.9 dB); our frames coded against 0.65 played 16 dB too loud and
 *    smeared on the chip, and 6.7 dB too loud once coded against 0.78.
 * Used by the encoder, its internal decoder model (mbevocoder.cpp) and the decoder
 * (ambe3600x2400.c). D-Star only; DMR / YSF (AMBE+2) keep AmbeW0table and 0.65.
 */
#ifndef QSO_DSTAR_PITCH_H
#define QSO_DSTAR_PITCH_H
#define DSTAR_F0_C0 4.24734f
#define DSTAR_F0_C1 0.021762f
#define DSTAR_SPEC_PRED 0.8f
/* Encoder loudness (log2 units subtracted from the coded gain) and smoothing of the
 * frame-to-frame gain, set so the chip plays our frames at the input level with the
 * chip's own frame-to-frame gain movement (mean |change of b2| 3.85 vs the chip's 3.92). */
#define DSTAR_GAIN_ADJUST 1.8f
#define DSTAR_GAIN_SMOOTH 0.55f
/* Decoder output gain (dB): with the fixes above our decoder plays the chip's frames
 * about 5 dB under the chip; this brings received stations and our echo to the chip's
 * level. */
#define DSTAR_OUT_GAIN_DB 5.4f
/* 3. The harmonic count L for each pitch code (QSO One, 2026-10-06 evening; NOTICE.md).
 *    mbelib's AmbePlusLtable gives 1 to 4 harmonics MORE than the chip plays for 102 of the
 *    120 codes (e.g. 36 vs 34 at 108 Hz). The spectral envelope is coded as L samples spread
 *    over the L harmonics, so a decoder with a different L lays the same envelope over a
 *    different stretch of frequency: real radios played our frames with the voice shape
 *    (formants) ~5 % LOW ("bassy, a quarter octave lower"), and we played real radios' frames
 *    ~5 % HIGH ("chipmunkish"); pitch was right both ways. Measured on the chip (frames with
 *    b0 held, all voiced, flat envelope; the highest harmonic it plays, the next one is
 *    25-60 dB down): L = max(9, floor(0.46288 / f0)) with f0 the chip pitch above, every one
 *    of the 120 codes (Research/DSTAR_STICK2_2026-10-06/lprobe_chip_harmonics.json). Used by the
 *    encoder, its decoder model and the decoder. D-Star only (DMR keeps AmbeLtable). */
static const unsigned char DSTAR_CHIP_L[120] = {
   9,  9,  9,  9,  9,  9,  9,  9,  9, 10, 10, 10,
  10, 10, 10, 11, 11, 11, 11, 11, 11, 12, 12, 12,
  12, 12, 13, 13, 13, 13, 13, 14, 14, 14, 14, 15,
  15, 15, 15, 15, 16, 16, 16, 16, 17, 17, 17, 17,
  18, 18, 18, 19, 19, 19, 20, 20, 20, 20, 21, 21,
  21, 22, 22, 22, 23, 23, 23, 24, 24, 25, 25, 25,
  26, 26, 27, 27, 27, 28, 28, 29, 29, 30, 30, 30,
  31, 31, 32, 32, 33, 33, 34, 34, 35, 36, 36, 37,
  37, 38, 38, 39, 40, 40, 41, 41, 42, 43, 43, 44,
  45, 45, 46, 47, 47, 48, 49, 50, 50, 51, 52, 53
};
#define DSTAR_L(b0) ((int) DSTAR_CHIP_L[((b0) < 0) ? 0 : (((b0) > 119) ? 119 : (b0))])
/* 4. Decoder level per harmonic class and pitch (QSO One, 2026-10-06 evening; NOTICE.md).
 *    On the chip's own frames (8 clips, 5 G4KLX prompt sets, 4 on-air captures; loudness
 *    contours aligned), mbelib played all-unvoiced frames 3.6 dB louder than the chip and
 *    all-voiced frames 1.4 dB quieter, and low voices louder than high ones (1.7 dB per
 *    octave). Applied to the decoder's harmonic amplitudes after mbelib's enhancement (the
 *    coded state is untouched): per-frame level spread vs the chip 3.54 -> 3.08 dB, PESQ
 *    against the chip's output 2.82 -> 2.93 (mean of 17 sets). The pitch slope is held at
 *    1.0 (the fit says 1.7) so a loud high voice plays at the chip's level (dv_test's loud
 *    clip: chip -12.5 dBFS, ours -12.4, 3 clipped samples; at 1.7: -11.2, 95). Decoder only. */
#define DSTAR_DEC_VOICED_DB 0.0f
#define DSTAR_DEC_UNVOICED_DB (-4.5f)
#define DSTAR_DEC_PITCH_DB_PER_OCT 1.0f
#define DSTAR_DEC_PITCH_REF_HZ 125.0f
#endif
