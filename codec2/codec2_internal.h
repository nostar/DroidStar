/*---------------------------------------------------------------------------*\

  FILE........: codec2_internal.h
  AUTHOR......: David Rowe
  DATE CREATED: April 16 2012

  Header file for Codec2 internal states, exposed via this header
  file to assist in testing.

\*---------------------------------------------------------------------------*/

/*
  Copyright (C) 2012 David Rowe

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

#ifndef __CODEC2_INTERNAL__
#define __CODEC2_INTERNAL__

#include "kiss_fft.h"

using CODEC2 = struct codec2_tag {
	int                mode;
	int                Fs;
	int                n_samp;
	int                m_pitch;
	int                gray;                     /* non-zero for gray encoding                */
	int                lpc_pf;                   /* LPC post filter on                        */
	int                bass_boost;               /* LPC post filter bass boost                */
	int                smoothing;                /* enable smoothing for channels with errors */
	float              ex_phase;                 /* excitation model phase track              */
	float              bg_est;                   /* background noise estimate for post filter */
	float              prev_f0_enc;              /* previous frame's f0    estimate           */
	float              prev_e_dec;               /* previous frame's LPC energy               */
	float              beta;                     /* LPC post filter parameters                */
	float              gamma;
	float              xq_enc[2];                /* joint pitch and energy VQ states          */
	float              xq_dec[2];
	float              W[FFT_ENC];	             /* DFT of w[]                                */
	float              hpf_states[2];            /* high pass filter states                   */
	float              prev_lsps_dec[LPC_ORD];   /* previous frame's LSPs                     */
	float             *softdec;                  /* optional soft decn bits from demod        */
	MODEL              prev_model_dec;           /* previous frame's model parameters         */
	C2CONST            c2const;
	FFT_STATE          fft_fwd_cfg;              /* forward FFT config                        */
	FFTR_STATE         fftr_fwd_cfg;             /* forward real FFT config                   */
	FFTR_STATE         fftr_inv_cfg;             /* inverse FFT config                        */
	std::vector<float> w;	                     /* [m_pitch] time domain hamming window      */
	std::vector<float> Pn;	                     /* [2*n_samp] trapezoidal synthesis window   */
	std::vector<float> Sn;                       /* [m_pitch] input speech                    */
	std::vector<float> Sn_;	                     /* [2*n_samp] synthesised output speech      */
	std::vector<float> bpf_buf;                  /* buffer for band pass filter               */
};

#endif
