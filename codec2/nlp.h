/*---------------------------------------------------------------------------*\

  FILE........: nlp.c
  AUTHOR......: David Rowe
  DATE CREATED: 23/3/93

  Non Linear Pitch (NLP) estimation functions.

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

#ifndef __NLP__
#define __NLP__

#include <complex>
#include <vector>

#include "defines.h"

/*---------------------------------------------------------------------------*\

 				DEFINES

\*---------------------------------------------------------------------------*/

#define PMAX_M      320		/* maximum NLP analysis window size     */
#define COEFF       0.95	/* notch filter parameter               */
#define PE_FFT_SIZE 512		/* DFT size for pitch estimation        */
#define DEC         5		/* decimation factor                    */
#define SAMPLE_RATE 8000
#define PI          3.141592654	/* mathematical constant                */
//#define T           0.1         /* threshold for local minima candidate */
#define F0_MAX      500
#define CNLP        0.3	        /* post processor constant              */
#define NLP_NTAP 48	        /* Decimation LPF order */

/* 8 to 16 kHz sample rate conversion */

#define FDMDV_OS                 2                            /* oversampling rate                   */
#define FDMDV_OS_TAPS_16K       48                            /* number of OS filter taps at 16kHz   */
#define FDMDV_OS_TAPS_8K        (FDMDV_OS_TAPS_16K/FDMDV_OS)  /* number of OS filter taps at 8kHz    */


using NLP = struct nlp_tag
{
	int           Fs;                /* sample rate in Hz            */
	int           m;
	float         w[PMAX_M/DEC];     /* DFT window                   */
	float         sq[PMAX_M];	     /* squared speech samples       */
	float         mem_x,mem_y;       /* memory for notch filter      */
	float         mem_fir[NLP_NTAP]; /* decimation FIR filter memory */
	FFT_STATE     fft_cfg;           /* kiss FFT config              */
	std::vector<float> Sn16k;	     /* Fs=16kHz input speech vector */
};


class Cnlp {
public:
	void nlp_create(C2CONST *c2const);
	void nlp_destroy();
	float nlp(float Sn[], int n, float *pitch_samples, float *prev_f0);
	void codec2_fft_inplace(FFT_STATE &cfg, std::complex<float> *inout);

private:
	float post_process_sub_multiples(std::complex<float> Fw[], int pmax, float gmax, int gmax_bin, float *prev_f0);
	void fdmdv_16_to_8(float out8k[], float in16k[], int n);

	NLP snlp;
};

#endif
