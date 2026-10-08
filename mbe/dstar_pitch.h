#ifndef QSO_DSTAR_PITCH_H
#define QSO_DSTAR_PITCH_H
#define DSTAR_F0_C0 4.24734f
#define DSTAR_F0_C1 0.021762f
#define DSTAR_SPEC_PRED 0.8f
#define DSTAR_GAIN_ADJUST 1.8f
#define DSTAR_GAIN_SMOOTH 0.55f
#define DSTAR_OUT_GAIN_DB 5.4f
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
#define DSTAR_DEC_VOICED_DB 0.0f
#define DSTAR_DEC_UNVOICED_DB (-4.5f)
#define DSTAR_DEC_PITCH_DB_PER_OCT 1.0f
#define DSTAR_DEC_PITCH_REF_HZ 125.0f
#endif
