/*---------------------------------------------------------------------------*\

  FILE........: defines.h
  AUTHOR......: David Rowe
  DATE CREATED: 23/4/93

  Defines and structures used throughout the codec.

\*---------------------------------------------------------------------------*/

/*
  Copyright (C) 2009 David Rowe

  All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 2.1, as
  published by the Free Software Foundation.  This program is
  distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
  License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __DEFINES__
#define __DEFINES__

#include <complex>
#include <vector>

/*---------------------------------------------------------------------------*\

				DEFINES

\*---------------------------------------------------------------------------*/

/* General defines */

#define N_S        0.01         /* internal proc frame length in secs   */
#define TW_S       0.005        /* trapezoidal synth window overlap     */
#define MAX_AMP    160			/* maximum number of harmonics          */
#ifndef PI
#define PI         3.141592654	/* mathematical constant                */
#endif
#define TWO_PI     6.283185307	/* mathematical constant                */
#define MAX_STR    2048         /* maximum string size                  */

#define FFT_ENC    512			/* size of FFT used for encoder         */
#define FFT_DEC    512	    	/* size of FFT used in decoder          */
#define V_THRESH   6.0          /* voicing threshold in dB              */
#define LPC_ORD    10			/* LPC order                            */
#define LPC_ORD_LOW 6			/* LPC order for lower rates            */

/* Pitch estimation defines */

#define M_PITCH_S  0.0400       /* pitch analysis window in s           */
#define P_MIN_S    0.0025		/* minimum pitch period in s            */
#define P_MAX_S    0.0200		/* maximum pitch period in s            */
#define MAXFACTORS 32			// e.g. an fft of length 128 has 4 factors
 								// as far as kissfft is concerned 4*4*4*2

/*---------------------------------------------------------------------------*\

				TYPEDEFS

\*---------------------------------------------------------------------------*/

/* Structure to hold constants calculated at run time based on sample rate */

using C2CONST = struct c2const_tag
{
    int   Fs;            /* sample rate of this instance             */
    int   n_samp;        /* number of samples per 10ms frame at Fs   */
    int   max_amp;       /* maximum number of harmonics              */
    int   m_pitch;       /* pitch estimation window size in samples  */
    int   p_min;         /* minimum pitch period in samples          */
    int   p_max;         /* maximum pitch period in samples          */
    float Wo_min;
    float Wo_max;
    int   nw;            /* analysis window size in samples          */
    int   tw;            /* trapezoidal synthesis window overlap     */
};

/* Structure to hold model parameters for one frame */

using MODEL = struct model_tag
{
    float Wo;		  /* fundamental frequency estimate in radians  */
    int   L;		  /* number of harmonics                        */
    float A[MAX_AMP+1];	  /* amplitiude of each harmonic                */
    float phi[MAX_AMP+1]; /* phase of each harmonic                     */
    int   voiced;	  /* non-zero if this frame is voiced           */
};

/* describes each codebook  */

struct lsp_codebook
{
	int     k; /* dimension of vector  */
	int log2m; /* number of bits in m  */
	int     m; /* elements in codebook */
	float *cb; /* The elements         */
};

using FFT_STATE = struct fft_state_tag
{
    int  nfft;
    bool inverse;
    int  factors[2*MAXFACTORS];
    std::vector<std::complex<float>> twiddles;
};

using FFTR_STATE = struct fftr_state_tag
{
	FFT_STATE substate;
	std::vector<std::complex<float>> tmpbuf;
	std::vector<std::complex<float>> super_twiddles;
};

extern const struct lsp_codebook lsp_cb[];
extern const struct lsp_codebook lsp_cbd[];
extern const struct lsp_codebook ge_cb[];

inline float exp10f(float val)
{
	return pow(10.0, val);
}

#endif
