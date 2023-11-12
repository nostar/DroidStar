/*
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
#define _USE_MATH_DEFINES
#include <stdio.h>
#include <cstring>
#include <cmath>

#include "vocoder_plugin.h"
#include "vocoder_tables.h"
#include "ambe3600x2400_const.h"
#include "ambe3600x2450_const.h"

static const short b0_lookup[] = {
	0, 0, 0, 1, 1, 2, 2, 2, 
	3, 3, 4, 4, 4, 5, 5, 5, 
	6, 6, 7, 7, 7, 8, 8, 8, 
	9, 9, 9, 10, 10, 11, 11, 11, 
	12, 12, 12, 13, 13, 13, 14, 14, 
	14, 15, 15, 15, 16, 16, 16, 17, 
	17, 17, 17, 18, 18, 18, 19, 19, 
	19, 20, 20, 20, 21, 21, 21, 21, 
	22, 22, 22, 23, 23, 23, 24, 24, 
	24, 24, 25, 25, 25, 25, 26, 26, 
	26, 27, 27, 27, 27, 28, 28, 28, 
	29, 29, 29, 29, 30, 30, 30, 30, 
	31, 31, 31, 31, 31, 32, 32, 32, 
	32, 33, 33, 33, 33, 34, 34, 34, 
	34, 35, 35, 35, 35, 36, 36, 36, 
	36, 37, 37, 37, 37, 38, 38, 38, 
	38, 38, 39, 39, 39, 39, 40, 40, 
	40, 40, 40, 41, 41, 41, 41, 42, 
	42, 42, 42, 42, 43, 43, 43, 43, 
	43, 44, 44, 44, 44, 45, 45, 45, 
	45, 45, 46, 46, 46, 46, 46, 47, 
	47, 47, 47, 47, 48, 48, 48, 48, 
	48, 49, 49, 49, 49, 49, 49, 50, 
	50, 50, 50, 50, 51, 51, 51, 51, 
	51, 52, 52, 52, 52, 52, 52, 53, 
	53, 53, 53, 53, 54, 54, 54, 54, 
	54, 54, 55, 55, 55, 55, 55, 56, 
	56, 56, 56, 56, 56, 57, 57, 57, 
	57, 57, 57, 58, 58, 58, 58, 58, 
	58, 59, 59, 59, 59, 59, 59, 60, 
	60, 60, 60, 60, 60, 61, 61, 61, 
	61, 61, 61, 62, 62, 62, 62, 62, 
	62, 63, 63, 63, 63, 63, 63, 63, 
	64, 64, 64, 64, 64, 64, 65, 65, 
	65, 65, 65, 65, 65, 66, 66, 66, 
	66, 66, 66, 67, 67, 67, 67, 67, 
	67, 67, 68, 68, 68, 68, 68, 68, 
	68, 69, 69, 69, 69, 69, 69, 69, 
	70, 70, 70, 70, 70, 70, 70, 71, 
	71, 71, 71, 71, 71, 71, 72, 72, 
	72, 72, 72, 72, 72, 73, 73, 73, 
	73, 73, 73, 73, 73, 74, 74, 74, 
	74, 74, 74, 74, 75, 75, 75, 75, 
	75, 75, 75, 75, 76, 76, 76, 76, 
	76, 76, 76, 76, 77, 77, 77, 77, 
	77, 77, 77, 77, 77, 78, 78, 78, 
	78, 78, 78, 78, 78, 79, 79, 79, 
	79, 79, 79, 79, 79, 80, 80, 80, 
	80, 80, 80, 80, 80, 81, 81, 81, 
	81, 81, 81, 81, 81, 81, 82, 82, 
	82, 82, 82, 82, 82, 82, 83, 83, 
	83, 83, 83, 83, 83, 83, 83, 84, 
	84, 84, 84, 84, 84, 84, 84, 84, 
	85, 85, 85, 85, 85, 85, 85, 85, 
	85, 86, 86, 86, 86, 86, 86, 86, 
	86, 86, 87, 87, 87, 87, 87, 87, 
	87, 87, 87, 88, 88, 88, 88, 88, 
	88, 88, 88, 88, 89, 89, 89, 89, 
	89, 89, 89, 89, 89, 89, 90, 90, 
	90, 90, 90, 90, 90, 90, 90, 90, 
	91, 91, 91, 91, 91, 91, 91, 91, 
	91, 92, 92, 92, 92, 92, 92, 92, 
	92, 92, 92, 93, 93, 93, 93, 93, 
	93, 93, 93, 93, 93, 94, 94, 94, 
	94, 94, 94, 94, 94, 94, 94, 94, 
	95, 95, 95, 95, 95, 95, 95, 95, 
	95, 95, 96, 96, 96, 96, 96, 96, 
	96, 96, 96, 96, 96, 97, 97, 97, 
	97, 97, 97, 97, 97, 97, 97, 98, 
	98, 98, 98, 98, 98, 98, 98, 98, 
	98, 98, 99, 99, 99, 99, 99, 99, 
	99, 99, 99, 99, 99, 99, 100, 100, 
	100, 100, 100, 100, 100, 100, 100, 100, 
	100, 101, 101, 101, 101, 101, 101, 101, 
	101, 101, 101, 101, 102, 102, 102, 102, 
	102, 102, 102, 102, 102, 102, 102, 102, 
	103, 103, 103, 103, 103, 103, 103, 103, 
	103, 103, 103, 103, 104, 104, 104, 104, 
	104, 104, 104, 104, 104, 104, 104, 104, 
	105, 105, 105, 105, 105, 105, 105, 105, 
	105, 105, 105, 105, 106, 106, 106, 106, 
	106, 106, 106, 106, 106, 106, 106, 106, 
	107, 107, 107, 107, 107, 107, 107, 107, 
	107, 107, 107, 107, 107, 108, 108, 108, 
	108, 108, 108, 108, 108, 108, 108, 108, 
	108, 109, 109, 109, 109, 109, 109, 109, 
	109, 109, 109, 109, 109, 109, 110, 110, 
	110, 110, 110, 110, 110, 110, 110, 110, 
	110, 110, 110, 111, 111, 111, 111, 111, 
	111, 111, 111, 111, 111, 111, 111, 111, 
	112, 112, 112, 112, 112, 112, 112, 112, 
	112, 112, 112, 112, 112, 112, 113, 113, 
	113, 113, 113, 113, 113, 113, 113, 113, 
	113, 113, 113, 113, 114, 114, 114, 114, 
	114, 114, 114, 114, 114, 114, 114, 114, 
	114, 115, 115, 115, 115, 115, 115, 115, 
	115, 115, 115, 115, 115, 115, 115, 116, 
	116, 116, 116, 116, 116, 116, 116, 116, 
	116, 116, 116, 116, 116, 116, 117, 117, 
	117, 117, 117, 117, 117, 117, 117, 117, 
	117, 117, 117, 117, 118, 118, 118, 118, 
	118, 118, 118, 118, 118, 118, 118, 118, 
	118, 118, 118, 119, 119, 119, 119, 119, 
	119, 119, 119
};

static const int m_list[] = {0, 1, 2, 3, 4, 5, 11, 12, 13, 14, 17, 18, 19, 20, 21, 22, 23, 26, 27, 28, 29, 30, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 7, 8, 9, 10, 15, 16, 24, 25, 31, 32, 6};
static const int d_list[] = {7, 1, 11, 21, 31, 25, 35, 45, 55, 49, 59, 69, 6, 0, 10, 20, 30, 24, 34, 44, 54, 48, 58, 68, 5, 15, 9, 19, 29, 39, 33, 43, 53, 63, 57, 67, 4, 14, 8, 18, 28, 38, 32, 42, 52, 62, 56, 66, 3, 13, 23, 17, 27, 37, 47, 41, 51, 61, 71, 65, 2, 12, 22, 16, 26, 36, 46, 40, 50, 60, 70, 64};
//static const int alt_d_list[] = {0, 12, 24, 36, 48, 60, 1, 13, 25, 37, 49, 61, 2, 14, 26, 38, 50, 62, 3, 15, 27, 39, 51, 63, 4, 16, 28, 40, 52, 64, 5, 17, 29, 41, 53, 65, 6, 18, 30, 42, 54, 66, 7, 19, 31, 43, 55, 67, 8, 20, 32, 44, 56, 68, 9, 21, 33, 45, 57, 69, 10, 22, 34, 46, 58, 70, 11, 23, 35, 47, 59, 71};
static const int b_lengths[] = {7,4,6,9,7,4,4,4,3};

const int dW[72] = {0,0,3,2,1,1,0,0,1,1,0,0,3,2,1,1,3,2,1,1,0,0,3,2,0,0,3,2,1,1,0,0,1,1,0,0,
                                3,2,1,1,3,2,1,1,0,0,3,2,0,0,3,2,1,1,0,0,1,1,0,0,3,2,1,1,3,3,2,1,0,0,3,3,};

const int dX[72] = {10,22,11,9,10,22,11,23,8,20,9,21,10,8,9,21,8,6,7,19,8,20,9,7,6,18,7,5,6,18,7,19,4,16,5,17,6,
                                4,5,17,4,2,3,15,4,16,5,3,2,14,3,1,2,14,3,15,0,12,1,13,2,0,1,13,0,12,10,11,0,12,1,13,};
const int rW[36] = {
  0, 1, 0, 1, 0, 1,
  0, 1, 0, 1, 0, 1,
  0, 1, 0, 1, 0, 1,
  0, 1, 0, 1, 0, 2,
  0, 2, 0, 2, 0, 2,
  0, 2, 0, 2, 0, 2
};

const int rX[36] = {
  23, 10, 22, 9, 21, 8,
  20, 7, 19, 6, 18, 5,
  17, 4, 16, 3, 15, 2,
  14, 1, 13, 0, 12, 10,
  11, 9, 10, 8, 9, 7,
  8, 6, 7, 5, 6, 4
};

// bit 0
const int rY[36] = {
  0, 2, 0, 2, 0, 2,
  0, 2, 0, 3, 0, 3,
  1, 3, 1, 3, 1, 3,
  1, 3, 1, 3, 1, 3,
  1, 3, 1, 3, 1, 3,
  1, 3, 1, 3, 1, 3
};

const int rZ[36] = {
  5, 3, 4, 2, 3, 1,
  2, 0, 1, 13, 0, 12,
  22, 11, 21, 10, 20, 9,
  19, 8, 18, 7, 17, 6,
  16, 5, 15, 4, 14, 3,
  13, 2, 12, 1, 11, 0
};


static inline uint32_t golay_24_encode(uint32_t code_word_in)
{
   static const uint32_t encoding[12] = {
      040006165,
      020003073,
      010007550,
      04003664,
      02001732,
      01006631,
      0403315,
      0201547,
      0106706,
      045227,
      024476,
      014353
   };

	uint32_t code_word_out = 0;
	for(uint16_t i = 0; i < 12; i++) {
		uint32_t temp_word = code_word_in & (1 << (11 - i));
		if(temp_word >= 1) {
			code_word_out = code_word_out ^ encoding[i];
		}
	}
	return(code_word_out);
}

static inline uint32_t golay_23_encode(uint32_t code_word_in)
{
	return golay_24_encode(code_word_in) >> 1;
}

static inline void store_reg(int reg, uint8_t val[], int len)
{
	for (int i=0; i<len; i++){
		val[i] = (reg >> (len-1-i)) & 1;
	}
}

static inline int load_reg(const uint8_t val[], int len)
{
	int acc = 0;
	for (int i=0; i<len; i++){
		acc = (acc << 1) + (val[i] & 1);
	}
	return acc;
}

inline float make_f0(int b0) {
	return (powf(2, (-4.311767578125 - (2.1336e-2 * ((float)b0+0.5)))));
}

int
mbe_dequantizeAmbeParms (mbe_parms * cur_mp, mbe_parms * prev_mp, const int *b, int dstar)
{

  int ji, i, j, k, l, L, m, am, ak;
  int intkl[57];
  int b0, b1, b2, b3, b4, b5, b6, b7, b8;
  float f0, Cik[5][18], flokl[57], deltal[57];
  float Sum42, Sum43, Tl[57], Gm[9], Ri[9], sum, c1, c2;
  //char tmpstr[13];
  int silence;
  int Ji[5], jl;
  float deltaGamma, BigGamma;
  float unvc, rconst;

  b0 = b[0];
  b1 = b[1];
  b2 = b[2];
  b3 = b[3];
  b4 = b[4];
  b5 = b[5];
  b6 = b[6];
  b7 = b[7];
  b8 = b[8];

  silence = 0;

#ifdef AMBE_DEBUG
  fprintf (stderr, "\n");
#endif

  // copy repeat from prev_mp
  cur_mp->repeat = prev_mp->repeat;

  if ((b0 >= 120) && (b0 <= 123))
    {
#ifdef AMBE_DEBUG
      fprintf (stderr, "AMBE Erasure Frame\n");
#endif
      return (2);
    }
  else if ((b0 == 124) || (b0 == 125))
    {
#ifdef AMBE_DEBUG
      fprintf (stderr, "AMBE Silence Frame\n");
#endif
      silence = 1;
      cur_mp->w0 = ((float) 2 * M_PI) / (float) 32;
      f0 = (float) 1 / (float) 32;
      L = 14;
      cur_mp->L = 14;
      for (l = 1; l <= L; l++)
        {
          cur_mp->Vl[l] = 0;
        }
    }
  else if ((b0 == 126) || (b0 == 127))
    {
#ifdef AMBE_DEBUG
      fprintf (stderr, "AMBE Tone Frame\n");
#endif
      return (3);
    }

  if (silence == 0)
    {
      if (dstar)
        f0 = powf(2, (-4.311767578125 - (2.1336e-2 * ((float)b0+0.5))));
      else
      // w0 from specification document
        f0 = AmbeW0table[b0];
      cur_mp->w0 = f0 * (float) 2 *M_PI;
      // w0 from patent filings
      //f0 = powf (2, ((float) b0 + (float) 195.626) / -(float) 45.368);
      //cur_mp->w0 = f0 * (float) 2 *M_PI;
    }

  unvc = (float) 0.2046 / sqrtf (cur_mp->w0);
  //unvc = (float) 1;
  //unvc = (float) 0.2046 / sqrtf (f0);

  // decode L
  if (silence == 0)
    {
      // L from specification document 
      // lookup L in tabl3
      if (dstar)
        L = AmbePlusLtable[b0];
      else
        L = AmbeLtable[b0];
      // L formula form patent filings
      //L=(int)((float)0.4627 / f0);
      cur_mp->L = L;
    }

  // decode V/UV parameters
  for (l = 1; l <= L; l++)
    {
      // jl from specification document
      jl = (int) ((float) l * (float) 16.0 * f0);
      // jl from patent filings?
      //jl = (int)(((float)l * (float)16.0 * f0) + 0.25);

      if (silence == 0)
        {
          if (dstar)
            cur_mp->Vl[l] = AmbePlusVuv[b1][jl];
          else
            cur_mp->Vl[l] = AmbeVuv[b1][jl];
        }
#ifdef AMBE_DEBUG
      fprintf (stderr, "jl[%i]:%i Vl[%i]:%i\n", l, jl, l, cur_mp->Vl[l]);
#endif
    }
#ifdef AMBE_DEBUG
  fprintf (stderr, "\nb0:%i w0:%f L:%i b1:%i\n", b0, cur_mp->w0, L, b1);
#endif
  if (dstar) {
    deltaGamma = AmbePlusDg[b2];
    cur_mp->gamma = deltaGamma + ((float) 0.5 * prev_mp->gamma);
  } else {
    deltaGamma = AmbeDg[b2];
    cur_mp->gamma = deltaGamma + ((float) 0.5 * prev_mp->gamma);
  }
#ifdef AMBE_DEBUG
  fprintf (stderr, "b2: %i, deltaGamma: %f gamma: %f gamma-1: %f\n", b2, deltaGamma, cur_mp->gamma, prev_mp->gamma);
#endif


  // decode PRBA vectors
  Gm[1] = 0;

  if (dstar) {
    Gm[2] = AmbePlusPRBA24[b3][0];
    Gm[3] = AmbePlusPRBA24[b3][1];
    Gm[4] = AmbePlusPRBA24[b3][2];

    Gm[5] = AmbePlusPRBA58[b4][0];
    Gm[6] = AmbePlusPRBA58[b4][1];
    Gm[7] = AmbePlusPRBA58[b4][2];
    Gm[8] = AmbePlusPRBA58[b4][3];
  } else {
    Gm[2] = AmbePRBA24[b3][0];
    Gm[3] = AmbePRBA24[b3][1];
    Gm[4] = AmbePRBA24[b3][2];

    Gm[5] = AmbePRBA58[b4][0];
    Gm[6] = AmbePRBA58[b4][1];
    Gm[7] = AmbePRBA58[b4][2];
    Gm[8] = AmbePRBA58[b4][3];
  }

#ifdef AMBE_DEBUG
  fprintf (stderr, "b3: %i Gm[2]: %f Gm[3]: %f Gm[4]: %f b4: %i Gm[5]: %f Gm[6]: %f Gm[7]: %f Gm[8]: %f\n", b3, Gm[2], Gm[3], Gm[4], b4, Gm[5], Gm[6], Gm[7], Gm[8]);
#endif

  // compute Ri
  for (i = 1; i <= 8; i++)
    {
      sum = 0;
      for (m = 1; m <= 8; m++)
        {
          if (m == 1)
            {
              am = 1;
            }
          else
            {
              am = 2;
            }
          sum = sum + ((float) am * Gm[m] * cosf ((M_PI * (float) (m - 1) * ((float) i - (float) 0.5)) / (float) 8));
        }
      Ri[i] = sum;
#ifdef AMBE_DEBUG
      fprintf (stderr, "R%i: %f ", i, Ri[i]);
#endif
    }
#ifdef AMBE_DEBUG
  fprintf (stderr, "\n");
#endif

  // generate first to elements of each Ci,k block from PRBA vector
  rconst = ((float) 1 / ((float) 2 * M_SQRT2));
  Cik[1][1] = (float) 0.5 *(Ri[1] + Ri[2]);
  Cik[1][2] = rconst * (Ri[1] - Ri[2]);
  Cik[2][1] = (float) 0.5 *(Ri[3] + Ri[4]);
  Cik[2][2] = rconst * (Ri[3] - Ri[4]);
  Cik[3][1] = (float) 0.5 *(Ri[5] + Ri[6]);
  Cik[3][2] = rconst * (Ri[5] - Ri[6]);
  Cik[4][1] = (float) 0.5 *(Ri[7] + Ri[8]);
  Cik[4][2] = rconst * (Ri[7] - Ri[8]);

  // decode HOC

  // lookup Ji
  if (dstar) {
    Ji[1] = AmbePlusLmprbl[L][0];
    Ji[2] = AmbePlusLmprbl[L][1];
    Ji[3] = AmbePlusLmprbl[L][2];
    Ji[4] = AmbePlusLmprbl[L][3];
  } else {
    Ji[1] = AmbeLmprbl[L][0];
    Ji[2] = AmbeLmprbl[L][1];
    Ji[3] = AmbeLmprbl[L][2];
    Ji[4] = AmbeLmprbl[L][3];
  }
#ifdef AMBE_DEBUG
  fprintf (stderr, "Ji[1]: %i Ji[2]: %i Ji[3]: %i Ji[4]: %i\n", Ji[1], Ji[2], Ji[3], Ji[4]);
  fprintf (stderr, "b5: %i b6: %i b7: %i b8: %i\n", b5, b6, b7, b8);
#endif

  // Load Ci,k with the values from the HOC tables
  // there appear to be a couple typos in eq. 37 so we will just do what makes sense
  // (3 <= k <= Ji and k<=6)
  for (k = 3; k <= Ji[1]; k++)
    {
      if (k > 6)
        {
          Cik[1][k] = 0;
        }
      else
        {
          if (dstar)
            Cik[1][k] = AmbePlusHOCb5[b5][k - 3];
          else
            Cik[1][k] = AmbeHOCb5[b5][k - 3];
#ifdef AMBE_DEBUG
          fprintf (stderr, "C1,%i: %f ", k, Cik[1][k]);
#endif
        }
    }
  for (k = 3; k <= Ji[2]; k++)
    {
      if (k > 6)
        {
          Cik[2][k] = 0;
        }
      else
        {
          if (dstar)
            Cik[2][k] = AmbePlusHOCb6[b6][k - 3];
          else
            Cik[2][k] = AmbeHOCb6[b6][k - 3];
#ifdef AMBE_DEBUG
          fprintf (stderr, "C2,%i: %f ", k, Cik[2][k]);
#endif
        }
    }
  for (k = 3; k <= Ji[3]; k++)
    {
      if (k > 6)
        {
          Cik[3][k] = 0;
        }
      else
        {
          if (dstar)
            Cik[3][k] = AmbePlusHOCb7[b7][k - 3];
          else
            Cik[3][k] = AmbeHOCb7[b7][k - 3];
#ifdef AMBE_DEBUG
          fprintf (stderr, "C3,%i: %f ", k, Cik[3][k]);
#endif
        }
    }
  for (k = 3; k <= Ji[4]; k++)
    {
      if (k > 6)
        {
          Cik[4][k] = 0;
        }
      else
        {
          if (dstar)
            Cik[4][k] = AmbePlusHOCb8[b8][k - 3];
          else
            Cik[4][k] = AmbeHOCb8[b8][k - 3];
#ifdef AMBE_DEBUG
          fprintf (stderr, "C4,%i: %f ", k, Cik[4][k]);
#endif
        }
    }
#ifdef AMBE_DEBUG
  fprintf (stderr, "\n");
#endif

  // inverse DCT each Ci,k to give ci,j (Tl)
  l = 1;
  for (i = 1; i <= 4; i++)
    {
      ji = Ji[i];
      for (j = 1; j <= ji; j++)
        {
          sum = 0;
          for (k = 1; k <= ji; k++)
            {
              if (k == 1)
                {
                  ak = 1;
                }
              else
                {
                  ak = 2;
                }
#ifdef AMBE_DEBUG
              fprintf (stderr, "j: %i Cik[%i][%i]: %f ", j, i, k, Cik[i][k]);
#endif
              sum = sum + ((float) ak * Cik[i][k] * cosf ((M_PI * (float) (k - 1) * ((float) j - (float) 0.5)) / (float) ji));
            }
          Tl[l] = sum;
#ifdef AMBE_DEBUG
          fprintf (stderr, "Tl[%i]: %f\n", l, Tl[l]);
#endif
          l++;
        }
    }

  // determine log2Ml by applying ci,j to previous log2Ml

  // fix for when L > L(-1)
  if (cur_mp->L > prev_mp->L)
    {
      for (l = (prev_mp->L) + 1; l <= cur_mp->L; l++)
        {
          prev_mp->Ml[l] = prev_mp->Ml[prev_mp->L];
          prev_mp->log2Ml[l] = prev_mp->log2Ml[prev_mp->L];
        }
    }
  prev_mp->log2Ml[0] = prev_mp->log2Ml[1];
  prev_mp->Ml[0] = prev_mp->Ml[1];

  // Part 1
  Sum43 = 0;
  for (l = 1; l <= cur_mp->L; l++)
    {

      // eq. 40
      flokl[l] = ((float) prev_mp->L / (float) cur_mp->L) * (float) l;
      intkl[l] = (int) (flokl[l]);
#ifdef AMBE_DEBUG
      fprintf (stderr, "flok%i: %f, intk%i: %i ", l, flokl[l], l, intkl[l]);
#endif
      // eq. 41
      deltal[l] = flokl[l] - (float) intkl[l];
#ifdef AMBE_DEBUG
      fprintf (stderr, "delta%i: %f ", l, deltal[l]);
#endif
      // eq 43
      Sum43 = Sum43 + ((((float) 1 - deltal[l]) * prev_mp->log2Ml[intkl[l]]) + (deltal[l] * prev_mp->log2Ml[intkl[l] + 1]));
    }
  Sum43 = (((float) 0.65 / (float) cur_mp->L) * Sum43);
#ifdef AMBE_DEBUG
  fprintf (stderr, "\n");
  fprintf (stderr, "Sum43: %f\n", Sum43);
#endif

  // Part 2
  Sum42 = 0;
  for (l = 1; l <= cur_mp->L; l++)
    {
      Sum42 += Tl[l];
    }
  Sum42 = Sum42 / (float) cur_mp->L;
  BigGamma = cur_mp->gamma - ((float) 0.5 * (log ((float) cur_mp->L) / log ((float) 2))) - Sum42;
  //BigGamma=cur_mp->gamma - ((float)0.5 * log((float)cur_mp->L)) - Sum42;

  // Part 3
  for (l = 1; l <= cur_mp->L; l++)
    {
      c1 = ((float) 0.65 * ((float) 1 - deltal[l]) * prev_mp->log2Ml[intkl[l]]);
      c2 = ((float) 0.65 * deltal[l] * prev_mp->log2Ml[intkl[l] + 1]);
      cur_mp->log2Ml[l] = Tl[l] + c1 + c2 - Sum43 + BigGamma;
      // inverse log to generate spectral amplitudes
      if (cur_mp->Vl[l] == 1)
        {
          cur_mp->Ml[l] = exp ((float) 0.693 * cur_mp->log2Ml[l]);
        }
      else
        {
          cur_mp->Ml[l] = unvc * exp ((float) 0.693 * cur_mp->log2Ml[l]);
        }
#ifdef AMBE_DEBUG
      fprintf (stderr, "flokl[%i]: %f, intkl[%i]: %i ", l, flokl[l], l, intkl[l]);
      fprintf (stderr, "deltal[%i]: %f ", l, deltal[l]);
      fprintf (stderr, "prev_mp->log2Ml[%i]: %f\n", l, prev_mp->log2Ml[intkl[l]]);
      fprintf (stderr, "BigGamma: %f c1: %f c2: %f Sum43: %f Tl[%i]: %f log2Ml[%i]: %f Ml[%i]: %f\n", BigGamma, c1, c2, Sum43, l, Tl[l], l, cur_mp->log2Ml[l], l, cur_mp->Ml[l]);
#endif
    }

  return (0);
}

int
mbe_dequantizeAmbe2400Parms (mbe_parms * cur_mp, mbe_parms * prev_mp, const int *b){
	int dstar = 1;
	return (mbe_dequantizeAmbeParms (cur_mp, prev_mp, b, dstar));
}

int
mbe_dequantizeAmbe2250Parms (mbe_parms * cur_mp, mbe_parms * prev_mp, const int *b){
	int dstar = 0;
	return (mbe_dequantizeAmbeParms (cur_mp, prev_mp, b, dstar));
}


void encode_ambe(const IMBE_PARAM *imbe_param, int b[], mbe_parms*cur_mp, mbe_parms*prev_mp, bool dstar, float gain_adjust) {
	static const float SQRT_2 = sqrtf(2.0);
	static const int b0_lmax = sizeof(b0_lookup) / sizeof(b0_lookup[0]);
	// int b[9];

	// ref_pitch is Q8_8 in range 19.875 - 123.125
	int b0_i = (imbe_param->ref_pitch >> 5) - 159;
	if (b0_i < 0 || b0_i > b0_lmax) {
		fprintf(stderr, "encode error b0_i %d\n", b0_i);
		return;
	}
	b[0] = b0_lookup[b0_i];
	int L;
	if (dstar)
		L = (int) AmbePlusLtable[b[0]];
	else
		L = (int) AmbeLtable[b[0]];
#if 1
	// adjust b0 until L agrees
	while (L != imbe_param->num_harms) {
		if (L < imbe_param->num_harms)
			b0_i ++;
		else if (L > imbe_param->num_harms)
			b0_i --;
		if (b0_i < 0 || b0_i > b0_lmax) {
			fprintf(stderr, "encode error2 b0_i %d\n", b0_i);
			return;
		}
		b[0] = b0_lookup[b0_i];
		if (dstar)
			L = (int) AmbePlusLtable[b[0]];
		else
			L = (int) AmbeLtable[b[0]];
	}
#endif
	float m_float2[NUM_HARMS_MAX];
	for (int l=1; l <= L; l++) {
		m_float2[l-1] = (float)imbe_param->sa[l-1] ;
		m_float2[l-1] = m_float2[l-1] * m_float2[l-1];
	}

	float en_min = 0;
	b[1] = 0;
	int vuv_max = (dstar) ? 16 : 17;
	for (int n=0; n < vuv_max; n++) {
		float En = 0;
		for (int l=1; l <= L; l++) {
			int jl;
			if (dstar)
				jl = (int) ((float) l * (float) 16.0 * make_f0(b[0]));
			else
				jl = (int) ((float) l * (float) 16.0 * AmbeW0table[b[0]]);
			int kl = 12;
			if (l <= 36)
				kl = (l + 2) / 3;
			if (dstar) {
				if (imbe_param->v_uv_dsn[(kl-1)*3] != AmbePlusVuv[n][jl])
					En += m_float2[l-1];
			} else {
				if (imbe_param->v_uv_dsn[(kl-1)*3] != AmbeVuv[n][jl])
					En += m_float2[l-1];
			}
		}
		if (n == 0)
			en_min = En;
		else if (En < en_min) {
			b[1] = n;
			en_min = En;
		}
	}

	// log spectral amplitudes
	float num_harms_f = (float) imbe_param->num_harms;
	float log_l_2 =  0.5 * log2f(num_harms_f);	// fixme: table lookup
	float log_l_w0;
	if (dstar)
		log_l_w0 = 0.5 * log2f(num_harms_f * make_f0(b[0]) * 2.0 * M_PI) + 2.289;
	else
		log_l_w0 = 0.5 * log2f(num_harms_f * AmbeW0table[b[0]] * 2.0 * M_PI) + 2.289;
	float lsa[NUM_HARMS_MAX];
	float lsa_sum=0.0;

	for (int i1 = 0; i1 < imbe_param->num_harms; i1++) {
		float sa = (float)imbe_param->sa[i1];
		if (sa < 1) sa = 1.0;
		if (imbe_param->v_uv_dsn[i1])
			lsa[i1] = log_l_2 + log2f(sa);
		else
			lsa[i1] = log_l_w0 + log2f(sa);
		lsa_sum += lsa[i1];
	}

	float gain = lsa_sum / num_harms_f;
	float diff_gain;
	if (dstar)
		diff_gain = gain;
	else
		diff_gain = gain - 0.5 * prev_mp->gamma;

	diff_gain -= gain_adjust;

	float error;
	int error_index;
	int max_dg = (dstar) ? 64 : 32;
	for (int i1 = 0; i1 < max_dg; i1++) {
		float diff;
		if (dstar)
			diff = fabsf(diff_gain - AmbePlusDg[i1]);
		else
			diff = fabsf(diff_gain - AmbeDg[i1]);
		//fprintf(stderr, "%2.4f:%2.4f ", diff, error);
		if ((i1 == 0) || (diff < error)) {
			error = diff;
			error_index = i1;
		}
	}
	//fprintf(stderr, "\n");
	b[2] = error_index;

	// prediction residuals
	float l_prev_l = (float) (prev_mp->L) / num_harms_f;
    //float tmp_s = 0.0;
	prev_mp->log2Ml[0] = prev_mp->log2Ml[1];
	for (int i1 = 0; i1 < imbe_param->num_harms; i1++) {
        //float kl = l_prev_l * (float)(i1+1);
        //int kl_floor = (int) kl;
        //float kl_frac = kl - kl_floor;
        //tmp_s += (1.0 - kl_frac) * prev_mp->log2Ml[kl_floor  +0] + kl_frac * prev_mp->log2Ml[kl_floor+1  +0];
	}
	float T[NUM_HARMS_MAX];
	for (int i1 = 0; i1 < imbe_param->num_harms; i1++) {
		float kl = l_prev_l * (float)(i1+1);
		int kl_floor = (int) kl;
		float kl_frac = kl - kl_floor;
		T[i1] = lsa[i1] - 0.65 * (1.0 - kl_frac) * prev_mp->log2Ml[kl_floor  +0]	\
				- 0.65 * kl_frac * prev_mp->log2Ml[kl_floor+1  +0];
	}

	// DCT
	const int * J;
	if (dstar)
		J = AmbePlusLmprbl[imbe_param->num_harms];
	else
		J = AmbeLmprbl[imbe_param->num_harms];
	float * c[4];
	int acc = 0;
	for (int i=0; i<4; i++) {
		c[i] = &T[acc];
		acc += J[i];
	}
	float C[4][17];
	for (int i=1; i<=4; i++) {
		for (int k=1; k<=J[i-1]; k++) {
			float s = 0.0;
			for (int j=1; j<=J[i-1]; j++) {
				//fixme: lut?
				s += (c[i-1][j-1] * cosf((M_PI * (((float)k) - 1.0) * (((float)j) - 0.5)) / (float)J[i-1]));
			}
			C[i-1][k-1] = s / (float)J[i-1];
		}
	}
	float R[8];
	R[0] = C[0][0] + SQRT_2 * C[0][1];
	R[1] = C[0][0] - SQRT_2 * C[0][1];
	R[2] = C[1][0] + SQRT_2 * C[1][1];
	R[3] = C[1][0] - SQRT_2 * C[1][1];
	R[4] = C[2][0] + SQRT_2 * C[2][1];
	R[5] = C[2][0] - SQRT_2 * C[2][1];
	R[6] = C[3][0] + SQRT_2 * C[3][1];
	R[7] = C[3][0] - SQRT_2 * C[3][1];

	// encode PRBA
	float G[8];
	for (int m=1; m<=8; m++) {
		G[m-1] = 0.0;
		for (int i=1; i<=8; i++) {
			//fixme: lut?
			G[m-1] += (R[i-1] * cosf((M_PI * (((float)m) - 1.0) * (((float)i) - 0.5)) / 8.0));
		}
		G[m-1] /= 8.0;
	}
	for (int i=0; i<512; i++) {
		float err=0.0;
		float diff;
		if (dstar) {
			diff = G[1] - AmbePlusPRBA24[i][0];
			err += (diff * diff);
			diff = G[2] - AmbePlusPRBA24[i][1];
			err += (diff * diff);
			diff = G[3] - AmbePlusPRBA24[i][2];
			err += (diff * diff);
		} else {
			diff = G[1] - AmbePRBA24[i][0];
			err += (diff * diff);
			diff = G[2] - AmbePRBA24[i][1];
			err += (diff * diff);
			diff = G[3] - AmbePRBA24[i][2];
			err += (diff * diff);
		}
		if (i == 0 || err < error) {
			error = err;
			error_index = i;
		}
	}
	b[3] = error_index;

	// PRBA58
	for (int i=0; i<128; i++) {
		float err=0.0;
		float diff;
		if (dstar) {
			diff = G[4] - AmbePlusPRBA58[i][0];
			err += (diff * diff);
			diff = G[5] - AmbePlusPRBA58[i][1];
			err += (diff * diff);
			diff = G[6] - AmbePlusPRBA58[i][2];
			err += (diff * diff);
			diff = G[7] - AmbePlusPRBA58[i][3];
			err += (diff * diff);
		} else {
			diff = G[4] - AmbePRBA58[i][0];
			err += (diff * diff);
			diff = G[5] - AmbePRBA58[i][1];
			err += (diff * diff);
			diff = G[6] - AmbePRBA58[i][2];
			err += (diff * diff);
			diff = G[7] - AmbePRBA58[i][3];
			err += (diff * diff);
		}
		if (i == 0 || err < error) {
			error = err;
			error_index = i;
		}
	}
	b[4] = error_index;

	// higher order coeffs b5
	int ii = 1;
	if (J[ii-1] <= 2) {
		b[4+ii] = 0.0;
	} else {
		int max_5 = (dstar) ? 16 : 32;
		for (int n=0; n < max_5; n++) {
			float err=0.0;
			float diff;
			for (int j=1; j <= J[ii-1]-2 && j <= 4; j++) {
				if (dstar)
					diff = AmbePlusHOCb5[n][j-1] - C[ii-1][j+2-1];
				else
					diff = AmbeHOCb5[n][j-1] - C[ii-1][j+2-1];
				err += (diff * diff);
			}
			if (n == 0 || err < error) {
				error = err;
				error_index = n;
			}
		}
		b[4+ii] = error_index;
	}

	// higher order coeffs b6
	ii = 2;
	if (J[ii-1] <= 2) {
		b[4+ii] = 0.0;
	} else {
		for (int n=0; n < 16; n++) {
			float err=0.0;
			float diff;
			for (int j=1; j <= J[ii-1]-2 && j <= 4; j++) {
				if (dstar)
					diff = AmbePlusHOCb6[n][j-1] - C[ii-1][j+2-1];
				else
					diff = AmbeHOCb6[n][j-1] - C[ii-1][j+2-1];
				err += (diff * diff);
			}
			if (n == 0 || err < error) {
				error = err;
				error_index = n;
			}
		}
		b[4+ii] = error_index;
	}

	// higher order coeffs b7
	ii = 3;
	if (J[ii-1] <= 2) {
		b[4+ii] = 0.0;
	} else {
		for (int n=0; n < 16; n++) {
			float err=0.0;
			float diff;
			for (int j=1; j <= J[ii-1]-2 && j <= 4; j++) {
				if (dstar)
					diff = AmbePlusHOCb7[n][j-1] - C[ii-1][j+2-1];
				else
					diff = AmbeHOCb7[n][j-1] - C[ii-1][j+2-1];
				err += (diff * diff);
			}
			if (n == 0 || err < error) {
				error = err;
				error_index = n;
			}
		}
		b[4+ii] = error_index;
	}

	// higher order coeffs b8
	ii = 4;
	if (J[ii-1] <= 2) {
		b[4+ii] = 0.0;
	} else {
		int max_8 = (dstar) ? 16 : 8;
		for (int n=0; n < max_8; n++) {
			float err=0.0;
			float diff;
			for (int j=1; j <= J[ii-1]-2 && j <= 4; j++) {
				if (dstar)
					diff = AmbePlusHOCb8[n][j-1] - C[ii-1][j+2-1];
				else
					diff = AmbeHOCb8[n][j-1] - C[ii-1][j+2-1];
				err += (diff * diff);
			}
			if (n == 0 || err < error) {
				error = err;
				error_index = n;
			}
		}
		b[4+ii] = error_index;
	}
	//fprintf (stderr, "B\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8]);
	//int rc;
	if (dstar)
		mbe_dequantizeAmbe2400Parms (cur_mp, prev_mp, b);
	else
		mbe_dequantizeAmbe2250Parms (cur_mp, prev_mp, b);
	mbe_moveMbeParms (cur_mp, prev_mp);
}

	VocoderPlugin::VocoderPlugin()
	{
		m_mbelibParms = new mbelibParms();
		m_audio_out_temp_buf_p = m_audio_out_temp_buf;

		memset(m_audio_out_buf, 0, sizeof(short) * 2 * 48000);
		m_audio_out_buf_p = m_audio_out_buf;
		m_audio_out_nb_samples = 0;
		m_audio_out_buf_size = 48000; // given in number of unique samples

		initMbeParms();
		memset(ambe_d, 0, 49);
	}
	
	VocoderPlugin::~VocoderPlugin()
	{
	}
	
	void VocoderPlugin::decode_2400x1200(int16_t *pcm, uint8_t *ambe)
	{
		int samples = 0;
		process_2400x1200(ambe);
		int16_t *p = getAudio(samples);
		memcpy(pcm, p, samples * sizeof(int16_t));
		resetAudio();
	}

	void VocoderPlugin::decode_2450x1150(int16_t *pcm, uint8_t *ambe)
	{
		int samples = 0;
		process_2450x1150(ambe);
		int16_t *p = getAudio(samples);
		memcpy(pcm, p, samples * sizeof(int16_t));
		resetAudio();
	}

	void VocoderPlugin::decode_2450(int16_t *pcm, uint8_t *ambe)
	{
		int samples = 0;
		process_2450(ambe);
		int16_t *p = getAudio(samples);
		memcpy(pcm, p, samples * sizeof(int16_t));
		resetAudio();
	}
	
	void VocoderPlugin::encode_2400x1200(int16_t *pcm, uint8_t *ambe)
	{
		int b[9];
		int16_t frame_vector[8];	// result ignored
		uint8_t ambe_frame[72];
		
		uint8_t pbuf[48];
		uint8_t tbuf[48];
		int tbufp = 0;
		
		vocoder.imbe_encode(frame_vector, pcm);
		encode_ambe(vocoder.param(), b, m_mbelibParms->m_cur_mp, m_mbelibParms->m_prev_mp, true, 1.0);
		
		for (int i=0; i < 9; i++) {
			store_reg(b[i], &tbuf[tbufp], b_lengths[i]);
			tbufp += b_lengths[i];
		}
		
		for (int i=0; i < 48; i++)
			pbuf[i] = tbuf[m_list[i]];
			
		int u0 = load_reg(pbuf+0, 12);
		int u1 = load_reg(pbuf+12, 12);

		int m1 = PRNG_TABLE[u0];
		int c0 = golay_24_encode(u0);
		int c1 = golay_24_encode(u1) ^ m1;

		uint8_t pre_buf[72];
		store_reg(c0, pre_buf, 24);
		store_reg(c1, pre_buf+24, 24);
		memcpy(pre_buf+48, pbuf+24, 24);

		for (int i=0; i < 72; i++)
			ambe_frame[d_list[i]] = pre_buf[i];

		for(int i = 0; i < 9; ++i){
			for(int j = 0; j < 8; ++j){
				ambe[i] |= (ambe_frame[(i*8)+j] << j);
			}
		}
	}
	
	void VocoderPlugin::encode_2450x1150(int16_t *pcm, uint8_t *ambe)
	{
		unsigned int aOrig = 0U;
		unsigned int bOrig = 0U;
		unsigned int cOrig = 0U;
		unsigned int MASK = 0x000800U;
		uint8_t tmp[9];
		
		memset(tmp, 0, 9);
		memset(ambe, 0, 9);
		encode_2450(pcm, tmp);
		
		for (unsigned int i = 0U; i < 12U; i++, MASK >>= 1) {
			unsigned int n1 = i;
			unsigned int n2 = i + 12U;
			if (READ_BIT(tmp, n1))
				aOrig |= MASK;
			if (READ_BIT(tmp, n2))
				bOrig |= MASK;
		}

		MASK = 0x1000000U;
		for (unsigned int i = 0U; i < 25U; i++, MASK >>= 1) {
			unsigned int n = i + 24U;
			if (READ_BIT(tmp, n))
				cOrig |= MASK;
		}

		unsigned int a = golay_24_encode(aOrig);

		// The PRNG
		unsigned int p = PRNG_TABLE[aOrig] >> 1;

		unsigned int b = golay_23_encode(bOrig);
		b ^= p;

		MASK = 0x800000U;
		for (unsigned int i = 0U; i < 24U; i++, MASK >>= 1) {
			unsigned int aPos = A_TABLE[i];
			WRITE_BIT(ambe, aPos, a & MASK);
		}

		MASK = 0x400000U;
		for (unsigned int i = 0U; i < 23U; i++, MASK >>= 1) {
			unsigned int bPos = B_TABLE[i];
			WRITE_BIT(ambe, bPos, b & MASK);
		}

		MASK = 0x1000000U;
		for (unsigned int i = 0U; i < 25U; i++, MASK >>= 1) {
			unsigned int cPos = C_TABLE[i];
			WRITE_BIT(ambe, cPos, cOrig & MASK);
		}
	}
	
	void VocoderPlugin::encode_2450(int16_t *pcm, uint8_t *ambe)
	{
		int b[9];
		int16_t frame_vector[8];	// result ignored
		uint8_t ambe_frame[49];
		
		vocoder.imbe_encode(frame_vector, pcm);
		encode_ambe(vocoder.param(), b, m_mbelibParms->m_cur_mp, m_mbelibParms->m_prev_mp, false, 1.0);
		
		ambe_frame[0] = (b[0] >> 6) & 1;
		ambe_frame[1] = (b[0] >> 5) & 1;
		ambe_frame[2] = (b[0] >> 4) & 1;
		ambe_frame[3] = (b[0] >> 3) & 1;
		ambe_frame[4] = (b[1] >> 4) & 1;
		ambe_frame[5] = (b[1] >> 3) & 1;
		ambe_frame[6] = (b[1] >> 2) & 1;
		ambe_frame[7] = (b[1] >> 1) & 1;
		ambe_frame[8] = (b[2] >> 4) & 1;
		ambe_frame[9] = (b[2] >> 3) & 1;
		ambe_frame[10] = (b[2] >> 2) & 1;
		ambe_frame[11] = (b[2] >> 1) & 1;
		ambe_frame[12] = (b[3] >> 8) & 1;
		ambe_frame[13] = (b[3] >> 7) & 1;
		ambe_frame[14] = (b[3] >> 6) & 1;
		ambe_frame[15] = (b[3] >> 5) & 1;
		ambe_frame[16] = (b[3] >> 4) & 1;
		ambe_frame[17] = (b[3] >> 3) & 1;
		ambe_frame[18] = (b[3] >> 2) & 1;
		ambe_frame[19] = (b[3] >> 1) & 1;
		ambe_frame[20] = (b[4] >> 6) & 1;
		ambe_frame[21] = (b[4] >> 5) & 1;
		ambe_frame[22] = (b[4] >> 4) & 1;
		ambe_frame[23] = (b[4] >> 3) & 1;
		ambe_frame[24] = (b[5] >> 4) & 1;
		ambe_frame[25] = (b[5] >> 3) & 1;
		ambe_frame[26] = (b[5] >> 2) & 1;
		ambe_frame[27] = (b[5] >> 1) & 1;
		ambe_frame[28] = (b[6] >> 3) & 1;
		ambe_frame[29] = (b[6] >> 2) & 1;
		ambe_frame[30] = (b[6] >> 1) & 1;
		ambe_frame[31] = (b[7] >> 3) & 1;
		ambe_frame[32] = (b[7] >> 2) & 1;
		ambe_frame[33] = (b[7] >> 1) & 1;
		ambe_frame[34] = (b[8] >> 2) & 1;
		ambe_frame[35] = b[1] & 1;
		ambe_frame[36] = b[2] & 1;
		ambe_frame[37] = (b[0] >> 2) & 1;
		ambe_frame[38] = (b[0] >> 1) & 1;
		ambe_frame[39] = b[0] & 1;
		ambe_frame[40] = b[3] & 1;
		ambe_frame[41] = (b[4] >> 2) & 1;
		ambe_frame[42] = (b[4] >> 1) & 1;
		ambe_frame[43] = b[4] & 1;
		ambe_frame[44] = b[5] & 1;
		ambe_frame[45] = b[6] & 1;
		ambe_frame[46] = b[7] & 1;
		ambe_frame[47] = (b[8] >> 1) & 1;
		ambe_frame[48] = b[8] & 1;
		
		for(int i = 0; i < 7; ++i){
			for(int j = 0; j < 8; ++j){
				ambe[i] |= (ambe_frame[(i*8)+j] << (7-j));
			}
		}
	}

	void VocoderPlugin::initMbeParms()
	{
		mbe_initMbeParms(m_mbelibParms->m_cur_mp, m_mbelibParms->m_prev_mp, m_mbelibParms->m_prev_mp_enhanced);
		//m_errs = 0;
		m_errs2 = 0;
		m_err_str[0] = 0;
	}
	
	void VocoderPlugin::process_2400x1200(unsigned char *d)
	{
		char ambe_fr[4][24];
    
		memset(ambe_fr, 0, 96);
		w = dW;
		x = dX;
	
		for(int i = 0; i < 9; ++i){
			for(int j = 0; j < 8; ++j){
				ambe_fr[*w][*x] = (1 & (d[i] >> j));
				w++;
				x++;
			}
		}

		mbe_processAmbe3600x2400Framef(m_audio_out_temp_buf, &m_errs2, m_err_str, ambe_fr, ambe_d,m_mbelibParms-> m_cur_mp, m_mbelibParms->m_prev_mp, m_mbelibParms->m_prev_mp_enhanced, 3);
		processAudio();
	}

	void VocoderPlugin::process_2450x1150(unsigned char *d)
	{
		char ambe_fr[4][24];

		memset(ambe_fr, 0, 96);
		w = rW;
		x = rX;
		y = rY;
		z = rZ;

		for(int i = 0; i < 9; ++i){
			for(int j = 0; j < 8; j+=2){
				ambe_fr[*y][*z] = (1 & (d[i] >> (7 - (j+1))));
				ambe_fr[*w][*x] = (1 & (d[i] >> (7 - j)));
				w++;
				x++;
				y++;
				z++;
			}
		}

		mbe_processAmbe3600x2450Framef(m_audio_out_temp_buf, &m_errs2, m_err_str, ambe_fr, ambe_d,m_mbelibParms-> m_cur_mp, m_mbelibParms->m_prev_mp, m_mbelibParms->m_prev_mp_enhanced, 3);
		processAudio();
	}

	void VocoderPlugin::process_2450(unsigned char *d)
	{
		char ambe_data[49];
		char dvsi_data[7];
		memset(dvsi_data, 0, 7);

		for(int i = 0; i < 6; ++i){
			for(int j = 0; j < 8; j++){
				ambe_data[j+(8*i)] = (1 & (d[i] >> (7 - j)));
			}
		}
		ambe_data[48] = (1 & (d[6] >> 7));
		processData(ambe_data);
	}
	
	void VocoderPlugin::processData(char ambe_data[49])
	{
		mbe_processAmbe2450Dataf(m_audio_out_temp_buf, &m_errs2, m_err_str, ambe_data, m_mbelibParms->m_cur_mp,m_mbelibParms->m_prev_mp, m_mbelibParms->m_prev_mp_enhanced, 3);
		processAudio();
	}
	
	short * VocoderPlugin::getAudio(int& nbSamples)
	{
		nbSamples = m_audio_out_nb_samples;
		return m_audio_out_buf;
	}

	void VocoderPlugin::resetAudio()
	{
		m_audio_out_nb_samples = 0;
		m_audio_out_buf_p = m_audio_out_buf;
	}
	
	void VocoderPlugin::processAudio()
	{
		m_audio_out_temp_buf_p = m_audio_out_temp_buf;

		if (m_audio_out_nb_samples + 160 >= m_audio_out_buf_size){
            resetAudio();
		}

		for (int i = 0; i < 160; i++){
			if (*m_audio_out_temp_buf_p > static_cast<float>(32760)){
				*m_audio_out_temp_buf_p = static_cast<float>(32760);
			}
			else if (*m_audio_out_temp_buf_p < static_cast<float>(-32760)){
				*m_audio_out_temp_buf_p = static_cast<float>(-32760);
			}

			*m_audio_out_buf_p = static_cast<short>(*m_audio_out_temp_buf_p);
			m_audio_out_buf_p++;

			m_audio_out_nb_samples++;
			m_audio_out_temp_buf_p++;
		}
	}
