/*
 * Copyright (C) 2010 mbelib Author
 * GPG Key ID: 0xEA5EFE2C (9E7A 5527 9CDC EBF7 BF1B  D772 4F98 E863 EA5E FE2C)
 *
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

#include <stdio.h>
#include <math.h>
#include "mbevocoder.h"
#include "vocoder_tables.h"
	
inline float make_f0(int b0) {
	return powf(2, -DSTAR_F0_C0 - DSTAR_F0_C1 * ((float)b0 + 0.5f));
}

/**
 * \return A pseudo-random float between [0.0, 1.0].
 * See http://www.azillionmonkeys.com/qed/random.html for further improvements
 */
static float
mbe_rand()
{
  return ((float) rand () / (float) RAND_MAX);
}

/**
 * \return A pseudo-random float between [-pi, +pi].
 */
static float
mbe_rand_phase()
{
  return mbe_rand() * (((float)M_PI) * 2.0F) - ((float)M_PI);
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
        f0 = make_f0(b0);
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
        L = DSTAR_L(b0);
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
  Sum43 = (((dstar ? DSTAR_SPEC_PRED : 0.65f) / (float) cur_mp->L) * Sum43);
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
      c1 = ((dstar ? DSTAR_SPEC_PRED : 0.65f) * ((float) 1 - deltal[l]) * prev_mp->log2Ml[intkl[l]]);
      c2 = ((dstar ? DSTAR_SPEC_PRED : 0.65f) * deltal[l] * prev_mp->log2Ml[intkl[l] + 1]);
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
mbe_dequantizeAmbe2450Parms (mbe_parms * cur_mp, mbe_parms * prev_mp, const int *b){
	int dstar = 0;
	return (mbe_dequantizeAmbeParms (cur_mp, prev_mp, b, dstar));
}

void encode_ambe(const IMBE_PARAM *imbe_param, int b[], mbe_parms*cur_mp, mbe_parms*prev_mp, bool dstar, float gain_adjust, float *gain_state) {
	static const float SQRT_2 = sqrtf(2.0);
	static const int b0_lmax = sizeof(b0_lookup) / sizeof(b0_lookup[0]);
	// int b[9];

	int L;
	// The harmonics the rest of this function codes: their count (nh), their
	// amplitudes (sa_w) and voicing (vuv_w). For DMR they are the IMBE
	// analysis' own values, unchanged.
	int nh = imbe_param->num_harms;
	float sa_w[NUM_HARMS_MAX];
	short vuv_w[NUM_HARMS_MAX];
	if (dstar) {
		float f0_in = 256.0f / (float) imbe_param->ref_pitch;	// cycles per sample
		int b0 = (int) lrintf((-log2f(f0_in) - DSTAR_F0_C0) / DSTAR_F0_C1 - 0.5f);	// nearest code on the chip-measured scale
		if (b0 < 0) b0 = 0;
		if (b0 > 119) b0 = 119;	// 120..127 are erasure / silence / tone codes
		b[0] = b0;
		L = DSTAR_L(b0);
		float f0_c = make_f0(b0);
		const int n_in = imbe_param->num_harms;
		for (int l = 1; l <= L; l++) {
			// position of the decoder's harmonic l on the IMBE harmonic axis (1-based)
			float k = (float) l * f0_c / f0_in;
			float a;
			if (k <= 1.0f) {
				a = (float) imbe_param->sa[0];
			} else if (k >= (float) n_in) {
				a = (float) imbe_param->sa[n_in - 1] * exp2f(-1.0f * (k - (float) n_in));
			} else {
				int k0 = (int) k;	// 1-based lower neighbour
				float fr = k - (float) k0;
				float a0 = (float) imbe_param->sa[k0 - 1];
				float a1 = (float) imbe_param->sa[k0];
				if (a0 < 1.0f) a0 = 1.0f;
				if (a1 < 1.0f) a1 = 1.0f;
				a = exp2f((1.0f - fr) * log2f(a0) + fr * log2f(a1));
			}
			sa_w[l - 1] = a;
			int kn = (int) lrintf(k);
			if (kn < 1) kn = 1;
			if (kn > n_in) kn = n_in;
			vuv_w[l - 1] = imbe_param->v_uv_dsn[kn - 1];
		}
		nh = L;
	} else {
	// ref_pitch is Q8_8 in range 19.875 - 123.125
	int b0_i = (imbe_param->ref_pitch >> 5) - 159;
	if (b0_i < 0 || b0_i > b0_lmax) {
		fprintf(stderr, "encode error b0_i %d\n", b0_i);
		return;
	}
	b[0] = b0_lookup[b0_i];
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
		L = (int) AmbeLtable[b[0]];
	}
#endif
		for (int l = 0; l < nh; l++) {
			sa_w[l] = (float) imbe_param->sa[l];
			vuv_w[l] = imbe_param->v_uv_dsn[l];
		}
	}
	float m_float2[NUM_HARMS_MAX];
	for (int l=1; l <= L; l++) {
		m_float2[l-1] = sa_w[l-1] ;
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
				if (vuv_w[l-1] != AmbePlusVuv[n][jl])
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
	float num_harms_f = (float) nh;
	float log_l_2 =  0.5 * log2f(num_harms_f);	// fixme: table lookup
	float log_l_w0;
	if (dstar)
		log_l_w0 = 0.5 * log2f(num_harms_f * make_f0(b[0]) * 2.0 * M_PI) + 2.289;
	else
		log_l_w0 = 0.5 * log2f(num_harms_f * AmbeW0table[b[0]] * 2.0 * M_PI) + 2.289;
	float lsa[NUM_HARMS_MAX];
	float lsa_sum=0.0;

	for (int i1 = 0; i1 < nh; i1++) {
		float sa = sa_w[i1];
		if (sa < 1) sa = 1.0;
		if (vuv_w[i1])
			lsa[i1] = log_l_2 + log2f(sa);
		else
			lsa[i1] = log_l_w0 + log2f(sa);
		lsa_sum += lsa[i1];
	}

	float gain = lsa_sum / num_harms_f;
	if (dstar && gain_state) {
		if (*gain_state > -98.0f) gain = DSTAR_GAIN_SMOOTH * (*gain_state) + (1.0f - DSTAR_GAIN_SMOOTH) * gain;
		*gain_state = gain;
	}
	float diff_gain;
	(void)dstar;
	diff_gain = gain - 0.5 * prev_mp->gamma;

	diff_gain -= gain_adjust;

	float error = 0.0;
	int error_index = 0;
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
	for (int i1 = 0; i1 < nh; i1++) {
        //float kl = l_prev_l * (float)(i1+1);
        //int kl_floor = (int) kl;
        //float kl_frac = kl - kl_floor;
        //tmp_s += (1.0 - kl_frac) * prev_mp->log2Ml[kl_floor  +0] + kl_frac * prev_mp->log2Ml[kl_floor+1  +0];
	}
	float T[NUM_HARMS_MAX];
	for (int i1 = 0; i1 < nh; i1++) {
		float kl = l_prev_l * (float)(i1+1);
		int kl_floor = (int) kl;
		float kl_frac = kl - kl_floor;
		const float pc = dstar ? DSTAR_SPEC_PRED : 0.65f;
		T[i1] = lsa[i1] - pc * (1.0 - kl_frac) * prev_mp->log2Ml[kl_floor  +0]	\
				- pc * kl_frac * prev_mp->log2Ml[kl_floor+1  +0];
	}

	// DCT
	const int * J;
	if (dstar)
		J = AmbePlusLmprbl[nh];
	else
		J = AmbeLmprbl[nh];
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
		const int step_8 = (dstar) ? 2 : 1;
		for (int n=0; n < max_8; n += step_8) {
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
		mbe_dequantizeAmbe2450Parms (cur_mp, prev_mp, b);
	mbe_moveMbeParms (cur_mp, prev_mp);
}

void
mbe_moveMbeParms (mbe_parms * cur_mp, mbe_parms * prev_mp)
{

  int l;

  prev_mp->w0 = cur_mp->w0;
  prev_mp->L = cur_mp->L;
  prev_mp->K = cur_mp->K;       // necessary?
  prev_mp->Ml[0] = (float) 0;
  prev_mp->gamma = cur_mp->gamma;
  prev_mp->repeat = cur_mp->repeat;
  for (l = 0; l <= 56; l++)
    {
      prev_mp->Ml[l] = cur_mp->Ml[l];
      prev_mp->Vl[l] = cur_mp->Vl[l];
      prev_mp->log2Ml[l] = cur_mp->log2Ml[l];
      prev_mp->PHIl[l] = cur_mp->PHIl[l];
      prev_mp->PSIl[l] = cur_mp->PSIl[l];
    }
}

void
mbe_useLastMbeParms (mbe_parms * cur_mp, mbe_parms * prev_mp)
{

  int l;

  cur_mp->w0 = prev_mp->w0;
  cur_mp->L = prev_mp->L;
  cur_mp->K = prev_mp->K;       // necessary?
  cur_mp->Ml[0] = (float) 0;
  cur_mp->gamma = prev_mp->gamma;
  cur_mp->repeat = prev_mp->repeat;
  for (l = 0; l <= 56; l++)
    {
      cur_mp->Ml[l] = prev_mp->Ml[l];
      cur_mp->Vl[l] = prev_mp->Vl[l];
      cur_mp->log2Ml[l] = prev_mp->log2Ml[l];
      cur_mp->PHIl[l] = prev_mp->PHIl[l];
      cur_mp->PSIl[l] = prev_mp->PSIl[l];
    }
}

void
mbe_initMbeParms (mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced)
{

  int l;

  prev_mp->w0 = 0.09378;
  prev_mp->L = 30;
  prev_mp->K = 10;
  prev_mp->gamma = (float) 0;
  for (l = 0; l <= 56; l++)
    {
      prev_mp->Ml[l] = (float) 0;
      prev_mp->Vl[l] = 0;
      prev_mp->log2Ml[l] = (float) 0;   // log2 of 1 == 0
      prev_mp->PHIl[l] = (float) 0;
      prev_mp->PSIl[l] = (M_PI / (float) 2);
    }
  prev_mp->repeat = 0;
  mbe_moveMbeParms (prev_mp, cur_mp);
  mbe_moveMbeParms (prev_mp, prev_mp_enhanced);
}

void
mbe_spectralAmpEnhance (mbe_parms * cur_mp)
{

  float Rm0, Rm1, R2m0, R2m1, Wl[57];
  int l;
  float sum, gamma, M;

  Rm0 = 0;
  Rm1 = 0;
  for (l = 1; l <= cur_mp->L; l++)
    {
      Rm0 = Rm0 + (cur_mp->Ml[l] * cur_mp->Ml[l]);
      Rm1 = Rm1 + ((cur_mp->Ml[l] * cur_mp->Ml[l]) * cosf (cur_mp->w0 * (float) l));
    }

  R2m0 = (Rm0*Rm0);
  R2m1 = (Rm1*Rm1);

  for (l = 1; l <= cur_mp->L; l++)
    {
      if (cur_mp->Ml[l] != 0)
        {
          Wl[l] = sqrtf (cur_mp->Ml[l]) * powf ((((float) 0.96 * M_PI * ((R2m0 + R2m1) - ((float) 2 * Rm0 * Rm1 * cosf (cur_mp->w0 * (float) l)))) / (cur_mp->w0 * Rm0 * (R2m0 - R2m1))), (float) 0.25);

          if ((8 * l) <= cur_mp->L)
            {
            }
          else if (Wl[l] > 1.2)
            {
              cur_mp->Ml[l] = 1.2 * cur_mp->Ml[l];
            }
          else if (Wl[l] < 0.5)
            {
              cur_mp->Ml[l] = 0.5 * cur_mp->Ml[l];
            }
          else
            {
              cur_mp->Ml[l] = Wl[l] * cur_mp->Ml[l];
            }
        }
    }

  // generate scaling factor
  sum = 0;
  for (l = 1; l <= cur_mp->L; l++)
    {
      M = cur_mp->Ml[l];
      if (M < 0)
        {
          M = -M;
        }
      sum += (M*M);
    }
  if (sum == 0)
    {
      gamma = (float) 1.0;
    }
  else
    {
      gamma = sqrtf (Rm0 / sum);
    }

  // apply scaling factor
  for (l = 1; l <= cur_mp->L; l++)
    {
      cur_mp->Ml[l] = gamma * cur_mp->Ml[l];
    }
}

void
mbe_synthesizeSilencef (float *aout_buf)
{

  int n;
  float *aout_buf_p;

  aout_buf_p = aout_buf;
  for (n = 0; n < 160; n++)
    {
      *aout_buf_p = (float) 0;
      aout_buf_p++;
    }
}

void
mbe_synthesizeSilence (short *aout_buf)
{

  int n;
  short *aout_buf_p;

  aout_buf_p = aout_buf;
  for (n = 0; n < 160; n++)
    {
      *aout_buf_p = (short) 0;
      aout_buf_p++;
    }
}

void
mbe_synthesizeSpeechf (float *aout_buf, mbe_parms * cur_mp, mbe_parms * prev_mp, int uvquality)
{

  int i, l, n, maxl;
  float *Ss, loguvquality;
  float C1, C2, C3, C4;
  //float deltaphil, deltawl, thetaln, aln;
  int numUv;
  float cw0, pw0, cw0l, pw0l;
  float uvsine, uvrand, uvthreshold, uvthresholdf;
  float uvstep, uvoffset;
  float qfactor;
  float rphase[64], rphase2[64];

  const int N = 160;

  uvthresholdf = (float) 2700;
  uvthreshold = ((uvthresholdf * M_PI) / (float) 4000);

  // voiced/unvoiced/gain settings
  uvsine = (float) 1.3591409 *M_E;
  uvrand = (float) 2.0;

  if ((uvquality < 1) || (uvquality > 64))
    {
      //printf ("\nmbelib: Error - uvquality must be within the range 1 - 64, setting to default value of 3\n");
      uvquality = 3;
    }

  // calculate loguvquality
  if (uvquality == 1)
    {
      loguvquality = (float) 1 / M_E;
    }
  else
    {
      loguvquality = log ((float) uvquality) / (float) uvquality;
    }

  // calculate unvoiced step and offset values
  uvstep = (float) 1.0 / (float) uvquality;
  qfactor = loguvquality;
  uvoffset = (uvstep * (float) (uvquality - 1)) / (float) 2;

  // count number of unvoiced bands
  numUv = 0;
  for (l = 1; l <= cur_mp->L; l++)
    {
      if (cur_mp->Vl[l] == 0)
        {
          numUv++;
        }
    }

  cw0 = cur_mp->w0;
  pw0 = prev_mp->w0;

  // init aout_buf
  Ss = aout_buf;
  for (n = 0; n < N; n++)
    {
      *Ss = (float) 0;
      Ss++;
    }

  // eq 128 and 129
  if (cur_mp->L > prev_mp->L)
    {
      maxl = cur_mp->L;
      for (l = prev_mp->L + 1; l <= maxl; l++)
        {
          prev_mp->Ml[l] = (float) 0;
          prev_mp->Vl[l] = 1;
        }
    }
  else
    {
      maxl = prev_mp->L;
      for (l = cur_mp->L + 1; l <= maxl; l++)
        {
          cur_mp->Ml[l] = (float) 0;
          cur_mp->Vl[l] = 1;
        }
    }

  // update phil from eq 139,140
  for (l = 1; l <= 56; l++)
    {
      cur_mp->PSIl[l] = prev_mp->PSIl[l] + ((pw0 + cw0) * ((float) (l * N) / (float) 2));
      if (l <= (int) (cur_mp->L / 4))
        {
          cur_mp->PHIl[l] = cur_mp->PSIl[l];
        }
      else
        {
          cur_mp->PHIl[l] = cur_mp->PSIl[l] + ((numUv * mbe_rand_phase()) / cur_mp->L);
        }
    }

  for (l = 1; l <= maxl; l++)
    {
      cw0l = (cw0 * (float) l);
      pw0l = (pw0 * (float) l);
      if ((cur_mp->Vl[l] == 0) && (prev_mp->Vl[l] == 1))
        {
          Ss = aout_buf;
          // init random phase
          for (i = 0; i < uvquality; i++)
            {
              rphase[i] = mbe_rand_phase();
            }
          for (n = 0; n < N; n++)
            {
              C1 = 0;
              // eq 131
              C1 = Ws[n + N] * prev_mp->Ml[l] * cosf ((pw0l * (float) n) + prev_mp->PHIl[l]);
              C3 = 0;
              // unvoiced multisine mix
              for (i = 0; i < uvquality; i++)
                {
                  C3 = C3 + cosf ((cw0 * (float) n * ((float) l + ((float) i * uvstep) - uvoffset)) + rphase[i]);
                  if (cw0l > uvthreshold)
                    {
                      C3 = C3 + ((cw0l - uvthreshold) * uvrand * mbe_rand());
                    }
                }
              C3 = C3 * uvsine * Ws[n] * cur_mp->Ml[l] * qfactor;
              *Ss = *Ss + C1 + C3;
              Ss++;
            }
        }
      else if ((cur_mp->Vl[l] == 1) && (prev_mp->Vl[l] == 0))
        {
          Ss = aout_buf;
          // init random phase
          for (i = 0; i < uvquality; i++)
            {
              rphase[i] = mbe_rand_phase();
            }
          for (n = 0; n < N; n++)
            {
              C1 = 0;
              // eq 132
              C1 = Ws[n] * cur_mp->Ml[l] * cosf ((cw0l * (float) (n - N)) + cur_mp->PHIl[l]);
              C3 = 0;
              // unvoiced multisine mix
              for (i = 0; i < uvquality; i++)
                {
                  C3 = C3 + cosf ((pw0 * (float) n * ((float) l + ((float) i * uvstep) - uvoffset)) + rphase[i]);
                  if (pw0l > uvthreshold)
                    {
                      C3 = C3 + ((pw0l - uvthreshold) * uvrand * mbe_rand());
                    }
                }
              C3 = C3 * uvsine * Ws[n + N] * prev_mp->Ml[l] * qfactor;
              *Ss = *Ss + C1 + C3;
              Ss++;
            }
        }
//      else if (((cur_mp->Vl[l] == 1) || (prev_mp->Vl[l] == 1)) && ((l >= 8) || (fabsf (cw0 - pw0) >= ((float) 0.1 * cw0))))
      else if ((cur_mp->Vl[l] == 1) || (prev_mp->Vl[l] == 1))
        {
          Ss = aout_buf;
          for (n = 0; n < N; n++)
            {
              C1 = 0;
              // eq 133-1
              C1 = Ws[n + N] * prev_mp->Ml[l] * cosf ((pw0l * (float) n) + prev_mp->PHIl[l]);
              C2 = 0;
              // eq 133-2
              C2 = Ws[n] * cur_mp->Ml[l] * cosf ((cw0l * (float) (n - N)) + cur_mp->PHIl[l]);
              *Ss = *Ss + C1 + C2;
              Ss++;
            }
        }
/*
      // expensive and unnecessary?
      else if ((cur_mp->Vl[l] == 1) || (prev_mp->Vl[l] == 1))
        {
          Ss = aout_buf;
          // eq 137
          deltaphil = cur_mp->PHIl[l] - prev_mp->PHIl[l] - (((pw0 + cw0) * (float) (l * N)) / (float) 2);
          // eq 138
          deltawl = ((float) 1 / (float) N) * (deltaphil - ((float) 2 * M_PI * (int) ((deltaphil + M_PI) / (M_PI * (float) 2))));
          for (n = 0; n < N; n++)
            {
              // eq 136
              thetaln = prev_mp->PHIl[l] + ((pw0l + deltawl) * (float) n) + (((cw0 - pw0) * ((float) (l * n * n)) / (float) (2 * N)));
              // eq 135
              aln = prev_mp->Ml[l] + (((float) n / (float) N) * (cur_mp->Ml[l] - prev_mp->Ml[l]));
              // eq 134
              *Ss = *Ss + (aln * cosf (thetaln));
              Ss++;
            }
        }
*/
      else
        {
          Ss = aout_buf;
          // init random phase
          for (i = 0; i < uvquality; i++)
            {
              rphase[i] = mbe_rand_phase();
            }
          // init random phase
          for (i = 0; i < uvquality; i++)
            {
              rphase2[i] = mbe_rand_phase();
            }
          for (n = 0; n < N; n++)
            {
              C3 = 0;
              // unvoiced multisine mix
              for (i = 0; i < uvquality; i++)
                {
                  C3 = C3 + cosf ((pw0 * (float) n * ((float) l + ((float) i * uvstep) - uvoffset)) + rphase[i]);
                  if (pw0l > uvthreshold)
                    {
                      C3 = C3 + ((pw0l - uvthreshold) * uvrand * mbe_rand());
                    }
                }
              C3 = C3 * uvsine * Ws[n + N] * prev_mp->Ml[l] * qfactor;
              C4 = 0;
              // unvoiced multisine mix
              for (i = 0; i < uvquality; i++)
                {
                  C4 = C4 + cosf ((cw0 * (float) n * ((float) l + ((float) i * uvstep) - uvoffset)) + rphase2[i]);
                  if (cw0l > uvthreshold)
                    {
                      C4 = C4 + ((cw0l - uvthreshold) * uvrand * mbe_rand());
                    }
                }
              C4 = C4 * uvsine * Ws[n] * cur_mp->Ml[l] * qfactor;
              *Ss = *Ss + C3 + C4;
              Ss++;
            }
        }
    }
}

void
mbe_synthesizeSpeech (short *aout_buf, mbe_parms * cur_mp, mbe_parms * prev_mp, int uvquality)
{
  float float_buf[160];

  mbe_synthesizeSpeechf (float_buf, cur_mp, prev_mp, uvquality);
  mbe_floattoshort (float_buf, aout_buf);
}

void
mbe_floattoshort (float *float_buf, short *aout_buf)
{

  short *aout_buf_p;
  float *float_buf_p;
  int i, again;
  float audio;

  again = 7;
  aout_buf_p = aout_buf;
  float_buf_p = float_buf;
  for (i = 0; i < 160; i++)
    {
      audio = again * *float_buf_p;
      if (audio > 32760)
        {
#ifdef MBE_DEBUG
          printf ("audio clip: %f\n", audio);
#endif
          audio = 32760;
        }
      else if (audio < -32760)
        {
#ifdef MBE_DEBUG
          printf ("audio clip: %f\n", audio);
#endif
          audio = -32760;
        }
      *aout_buf_p = (short) (audio);
      aout_buf_p++;
      float_buf_p++;
    }
}

void
mbe_checkGolayBlock (long int *block)
{

  static int i, syndrome, eccexpected, eccbits, databits;
  long int mask, block_l;

  block_l = *block;

  mask = 0x400000l;
  eccexpected = 0;
  for (i = 0; i < 12; i++)
    {
      if ((block_l & mask) != 0l)
        {
          eccexpected ^= golayGenerator[i];
        }
      mask = mask >> 1;
    }
  eccbits = (int) (block_l & 0x7ffl);
  syndrome = eccexpected ^ eccbits;

  databits = (int) (block_l >> 11);
  databits = databits ^ golayMatrix[syndrome];

  *block = (long) databits;
}

int
mbe_golay2312 (char *in, char *out)
{

  int i, errs;
  long block;

  block = 0;
  for (i = 22; i >= 0; i--)
    {
      block = block << 1;
      block = block + in[i];
    }

  mbe_checkGolayBlock (&block);

  for (i = 22; i >= 11; i--)
    {
      out[i] = (block & 2048) >> 11;
      block = block << 1;
    }
  for (i = 10; i >= 0; i--)
    {
      out[i] = in[i];
    }

  errs = 0;
  for (i = 22; i >= 11; i--)
    {
      if (out[i] != in[i])
        {
          errs++;
        }
    }
  return (errs);
}
