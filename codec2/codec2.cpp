/*---------------------------------------------------------------------------*\

  FILE........: codec2.c
  AUTHOR......: David Rowe
  DATE CREATED: 21/8/2010

  Codec2 fully quantised encoder and decoder functions.  If you want use
  codec2, the codec2_xxx functions are for you.

\*---------------------------------------------------------------------------*/

/*
  Copyright (C) 2010 David Rowe

  All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 2.1, as
  published by the free Software Foundation.  This program is
  distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
  License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "nlp.h"
#include "lpc.h"
#include "quantise.h"
#include "codec2_api.h"
#include "codec2_internal.h"

#define HPF_BETA 0.125
#define BPF_N 101

CKissFFT kiss;

/*---------------------------------------------------------------------------* \

                             FUNCTION HEADERS

\*---------------------------------------------------------------------------*/




/*---------------------------------------------------------------------------*\

                                FUNCTIONS

\*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*\

  FUNCTION....: codec2_create
  AUTHOR......: David Rowe
  DATE CREATED: 21/8/2010

  Create and initialise an instance of the codec.  Returns a pointer
  to the codec states or NULL on failure.  One set of states is
  sufficient for a full duuplex codec (i.e. an encoder and decoder).
  You don't need separate states for encoders and decoders.  See
  c2enc.c and c2dec.c for examples.

\*---------------------------------------------------------------------------*/

CCodec2::CCodec2(bool is_3200)
{
	c2.mode = is_3200 ? 3200 : 1600;

	/* store constants in a few places for convenience */

	c2.c2const = c2const_create(8000, N_S);
	c2.Fs = c2.c2const.Fs;
	int n_samp = c2.n_samp = c2.c2const.n_samp;
	int m_pitch = c2.m_pitch = c2.c2const.m_pitch;

	c2.Pn.resize(2*n_samp);
	c2.Sn_.resize(2*n_samp);
	c2.w.resize(m_pitch);
	c2.Sn.resize(m_pitch);

	for(int i=0; i<m_pitch; i++)
		c2.Sn[i] = 1.0;
	c2.hpf_states[0] = c2.hpf_states[1] = 0.0;
	for(int i=0; i<2*n_samp; i++)
		c2.Sn_[i] = 0;
	kiss.fft_alloc(c2.fft_fwd_cfg, FFT_ENC, false);
	kiss.fftr_alloc(c2.fftr_fwd_cfg, FFT_ENC, false);
	make_analysis_window(&c2.c2const, &c2.fft_fwd_cfg, c2.w.data(), c2.W);
	make_synthesis_window(&c2.c2const, c2.Pn.data());
	kiss.fftr_alloc(c2.fftr_inv_cfg, FFT_DEC, true);
	c2.prev_f0_enc = 1/P_MAX_S;
	c2.bg_est = 0.0;
	c2.ex_phase = 0.0;

	for(int l=1; l<=MAX_AMP; l++)
		c2.prev_model_dec.A[l] = 0.0;
	c2.prev_model_dec.Wo = TWO_PI/c2.c2const.p_max;
	c2.prev_model_dec.L = PI/c2.prev_model_dec.Wo;
	c2.prev_model_dec.voiced = 0;

	for(int i=0; i<LPC_ORD; i++)
	{
		c2.prev_lsps_dec[i] = i*PI/(LPC_ORD+1);
	}
	c2.prev_e_dec = 1;

	nlp.nlp_create(&c2.c2const);

	c2.lpc_pf = 1;
	c2.bass_boost = 1;
	c2.beta = LPCPF_BETA;
	c2.gamma = LPCPF_GAMMA;

	c2.xq_enc[0] = c2.xq_enc[1] = 0.0;
	c2.xq_dec[0] = c2.xq_dec[1] = 0.0;

	c2.smoothing = 0;

	c2.bpf_buf.resize(BPF_N+4*c2.n_samp);
	for(int i=0; i<BPF_N+4*c2.n_samp; i++)
		c2.bpf_buf[i] = 0.0;

	c2.softdec = NULL;
	c2.gray = 1;

	// make sure that one of the two decode function pointers is empty
	// for the encode function pointer this is not required since we always set it
	// to a meaningful value

	decode = NULL;

	if ( 3200 == c2.mode)
	{
		encode = &CCodec2::codec2_encode_3200;
		decode = &CCodec2::codec2_decode_3200;
	}
	else
	{
		encode = &CCodec2::codec2_encode_1600;
		decode = &CCodec2::codec2_decode_1600;
	}
}

/*---------------------------------------------------------------------------*\

  FUNCTION....: codec2_destroy
  AUTHOR......: David Rowe
  DATE CREATED: 21/8/2010

  Destroy an instance of the codec.

\*---------------------------------------------------------------------------*/

CCodec2::~CCodec2()
{
	c2.bpf_buf.clear();
	nlp.nlp_destroy();
	c2.fft_fwd_cfg.twiddles.clear();
	c2.fftr_fwd_cfg.substate.twiddles.clear();
	c2.fftr_fwd_cfg.tmpbuf.clear();
	c2.fftr_fwd_cfg.super_twiddles.clear();
	c2.fftr_inv_cfg.substate.twiddles.clear();
	c2.fftr_inv_cfg.tmpbuf.clear();
	c2.fftr_inv_cfg.super_twiddles.clear();
	c2.Pn.clear();
	c2.Sn.clear();
	c2.w.clear();
	c2.Sn_.clear();
}

void CCodec2::codec2_set_mode(bool m)
{
	c2.mode = m ? 3200 : 1600;
	if (c2.mode == 3200){
		encode = &CCodec2::codec2_encode_3200;
		decode = &CCodec2::codec2_decode_3200;
	}
	else{
		encode = &CCodec2::codec2_encode_1600;
		decode = &CCodec2::codec2_decode_1600;
	}
}

/*---------------------------------------------------------------------------*\

  FUNCTION....: codec2_bits_per_frame
  AUTHOR......: David Rowe
  DATE CREATED: Nov 14 2011

  Returns the number of bits per frame.

\*---------------------------------------------------------------------------*/

int CCodec2::codec2_bits_per_frame()
{
	return 64;
}


/*---------------------------------------------------------------------------*\

  FUNCTION....: codec2_samples_per_frame
  AUTHOR......: David Rowe
  DATE CREATED: Nov 14 2011

  Returns the number of speech samples per frame.

\*---------------------------------------------------------------------------*/

int CCodec2::codec2_samples_per_frame()
{
	if (3200 == c2.mode)
		return 160;
	else
		return 320;
	return 0; /* shouldnt get here */
}

void CCodec2::codec2_encode(unsigned char *bits, const short *speech)
{
	assert(encode != NULL);

	(*this.*encode)(bits, speech);
}

void CCodec2::codec2_decode(short *speech, const unsigned char *bits)
{
	assert(decode != NULL);

	(*this.*decode)(speech, bits);
}


/*---------------------------------------------------------------------------*\

  FUNCTION....: codec2_encode_3200
  AUTHOR......: David Rowe
  DATE CREATED: 13 Sep 2012

  Encodes 160 speech samples (20ms of speech) into 64 bits.

  The codec2 algorithm actually operates internally on 10ms (80
  sample) frames, so we run the encoding algorithm twice.  On the
  first frame we just send the voicing bits.  On the second frame we
  send all model parameters.  Compared to 2400 we use a larger number
  of bits for the LSPs and non-VQ pitch and energy.

  The bit allocation is:

    Parameter                      bits/frame
    --------------------------------------
    Harmonic magnitudes (LSPs)     50
    Pitch (Wo)                      7
    Energy                          5
    Voicing (10ms update)           2
    TOTAL                          64

\*---------------------------------------------------------------------------*/

void CCodec2::codec2_encode_3200(unsigned char *bits, const short *speech)
{
	MODEL   model;
	float   ak[LPC_ORD+1];
	float   lsps[LPC_ORD];
	float   e;
	int     Wo_index, e_index;
	int     lspd_indexes[LPC_ORD];
	int     i;
	unsigned int nbit = 0;

	memset(bits, '\0', ((codec2_bits_per_frame() + 7) / 8));

	/* first 10ms analysis frame - we just want voicing */

	analyse_one_frame(&model, speech);
	qt.pack(bits, &nbit, model.voiced, 1);

	/* second 10ms analysis frame */

	analyse_one_frame(&model, &speech[c2.n_samp]);
	qt.pack(bits, &nbit, model.voiced, 1);
	Wo_index = qt.encode_Wo(&c2.c2const, model.Wo, WO_BITS);
	qt.pack(bits, &nbit, Wo_index, WO_BITS);

	e = qt.speech_to_uq_lsps(lsps, ak, c2.Sn.data(), c2.w.data(), c2.m_pitch, LPC_ORD);
	e_index = qt.encode_energy(e, E_BITS);
	qt.pack(bits, &nbit, e_index, E_BITS);

	qt.encode_lspds_scalar(lspd_indexes, lsps, LPC_ORD);
	for(i=0; i<LSPD_SCALAR_INDEXES; i++)
	{
		qt.pack(bits, &nbit, lspd_indexes[i], qt.lspd_bits(i));
	}
	assert(nbit == (unsigned)codec2_bits_per_frame());
}


/*---------------------------------------------------------------------------*\

  FUNCTION....: codec2_decode_3200
  AUTHOR......: David Rowe
  DATE CREATED: 13 Sep 2012

  Decodes a frame of 64 bits into 160 samples (20ms) of speech.

\*---------------------------------------------------------------------------*/

void CCodec2::codec2_decode_3200(short speech[], const unsigned char * bits)
{
	MODEL   model[2];
	int     lspd_indexes[LPC_ORD];
	float   lsps[2][LPC_ORD];
	int     Wo_index, e_index;
	float   e[2];
	float   snr;
	float   ak[2][LPC_ORD+1];
	int     i,j;
	unsigned int nbit = 0;
	std::complex<float>    Aw[FFT_ENC];

	/* only need to zero these out due to (unused) snr calculation */

	for(i=0; i<2; i++)
		for(j=1; j<=MAX_AMP; j++)
			model[i].A[j] = 0.0;

	/* unpack bits from channel ------------------------------------*/

	/* this will partially fill the model params for the 2 x 10ms
	   frames */

	model[0].voiced = qt.unpack(bits, &nbit, 1);
	model[1].voiced = qt.unpack(bits, &nbit, 1);

	Wo_index = qt.unpack(bits, &nbit, WO_BITS);
	model[1].Wo = qt.decode_Wo(&c2.c2const, Wo_index, WO_BITS);
	model[1].L  = PI/model[1].Wo;

	e_index = qt.unpack(bits, &nbit, E_BITS);
	e[1] = qt.decode_energy(e_index, E_BITS);

	for(i=0; i<LSPD_SCALAR_INDEXES; i++)
	{
		lspd_indexes[i] = qt.unpack(bits, &nbit, qt.lspd_bits(i));
	}
	qt.decode_lspds_scalar(&lsps[1][0], lspd_indexes, LPC_ORD);

	/* interpolate ------------------------------------------------*/

	/* Wo and energy are sampled every 20ms, so we interpolate just 1
	   10ms frame between 20ms samples */

	interp_Wo(&model[0], &c2.prev_model_dec, &model[1], c2.c2const.Wo_min);
	e[0] = interp_energy(c2.prev_e_dec, e[1]);

	/* LSPs are sampled every 20ms so we interpolate the frame in
	   between, then recover spectral amplitudes */

	interpolate_lsp_ver2(&lsps[0][0], c2.prev_lsps_dec, &lsps[1][0], 0.5, LPC_ORD);

	for(i=0; i<2; i++)
	{
		lsp_to_lpc(&lsps[i][0], &ak[i][0], LPC_ORD);
		qt.aks_to_M2(&(c2.fftr_fwd_cfg), &ak[i][0], LPC_ORD, &model[i], e[i], &snr, 0, c2.lpc_pf, c2.bass_boost, c2.beta, c2.gamma, Aw);
		qt.apply_lpc_correction(&model[i]);
		synthesise_one_frame(&speech[c2.n_samp*i], &model[i], Aw, 3.0);
	}

	/* update memories for next frame ----------------------------*/

	c2.prev_model_dec = model[1];
	c2.prev_e_dec = e[1];
	for(i=0; i<LPC_ORD; i++)
		c2.prev_lsps_dec[i] = lsps[1][i];
}

/*---------------------------------------------------------------------------*\

  FUNCTION....: codec2_encode_1600
  AUTHOR......: David Rowe
  DATE CREATED: Feb 28 2013

  Encodes 320 speech samples (40ms of speech) into 64 bits.

  The codec2 algorithm actually operates internally on 10ms (80
  sample) frames, so we run the encoding algorithm 4 times:

  frame 0: voicing bit
  frame 1: voicing bit, Wo and E
  frame 2: voicing bit
  frame 3: voicing bit, Wo and E, scalar LSPs

  The bit allocation is:

    Parameter                      frame 2  frame 4   Total
    -------------------------------------------------------
    Harmonic magnitudes (LSPs)      0       36        36
    Pitch (Wo)                      7        7        14
    Energy                          5        5        10
    Voicing (10ms update)           2        2         4
    TOTAL                          14       50        64

\*---------------------------------------------------------------------------*/

void CCodec2::codec2_encode_1600(unsigned char * bits, const short speech[])
{
	MODEL   model;
	float   lsps[LPC_ORD];
	float   ak[LPC_ORD+1];
	float   e;
	int     lsp_indexes[LPC_ORD];
	int     Wo_index, e_index;
	int     i;
	unsigned int nbit = 0;

	memset(bits, '\0',  ((codec2_bits_per_frame() + 7) / 8));

	/* frame 1: - voicing ---------------------------------------------*/

	analyse_one_frame(&model, speech);
	qt.pack(bits, &nbit, model.voiced, 1);

	/* frame 2: - voicing, scalar Wo & E -------------------------------*/

	analyse_one_frame(&model, &speech[c2.n_samp]);
	qt.pack(bits, &nbit, model.voiced, 1);

	Wo_index = qt.encode_Wo(&c2.c2const, model.Wo, WO_BITS);
	qt.pack(bits, &nbit, Wo_index, WO_BITS);

	/* need to run this just to get LPC energy */
	e = qt.speech_to_uq_lsps(lsps, ak, c2.Sn.data(), c2.w.data(), c2.m_pitch, LPC_ORD);
	e_index = qt.encode_energy(e, E_BITS);
	qt.pack(bits, &nbit, e_index, E_BITS);

	/* frame 3: - voicing ---------------------------------------------*/

	analyse_one_frame(&model, &speech[2*c2.n_samp]);
	qt.pack(bits, &nbit, model.voiced, 1);

	/* frame 4: - voicing, scalar Wo & E, scalar LSPs ------------------*/

	analyse_one_frame(&model, &speech[3*c2.n_samp]);
	qt.pack(bits, &nbit, model.voiced, 1);

	Wo_index = qt.encode_Wo(&c2.c2const, model.Wo, WO_BITS);
	qt.pack(bits, &nbit, Wo_index, WO_BITS);

	e = qt.speech_to_uq_lsps(lsps, ak, c2.Sn.data(), c2.w.data(), c2.m_pitch, LPC_ORD);
	e_index = qt.encode_energy(e, E_BITS);
	qt.pack(bits, &nbit, e_index, E_BITS);

	qt.encode_lsps_scalar(lsp_indexes, lsps, LPC_ORD);
	for(i=0; i<LSP_SCALAR_INDEXES; i++)
	{
		qt.pack(bits, &nbit, lsp_indexes[i], qt.lsp_bits(i));
	}

	assert(nbit == (unsigned)codec2_bits_per_frame());
}


/*---------------------------------------------------------------------------*\

  FUNCTION....: codec2_decode_1600
  AUTHOR......: David Rowe
  DATE CREATED: 11 May 2012

  Decodes frames of 64 bits into 320 samples (40ms) of speech.

\*---------------------------------------------------------------------------*/

void CCodec2::codec2_decode_1600(short speech[], const unsigned char * bits)
{
	MODEL   model[4];
	int     lsp_indexes[LPC_ORD];
	float   lsps[4][LPC_ORD];
	int     Wo_index, e_index;
	float   e[4];
	float   snr;
	float   ak[4][LPC_ORD+1];
	int     i,j;
	unsigned int nbit = 0;
	float   weight;
	std::complex<float>    Aw[FFT_ENC];

	/* only need to zero these out due to (unused) snr calculation */

	for(i=0; i<4; i++)
		for(j=1; j<=MAX_AMP; j++)
			model[i].A[j] = 0.0;

	/* unpack bits from channel ------------------------------------*/

	/* this will partially fill the model params for the 4 x 10ms
	   frames */

	model[0].voiced = qt.unpack(bits, &nbit, 1);

	model[1].voiced = qt.unpack(bits, &nbit, 1);
	Wo_index = qt.unpack(bits, &nbit, WO_BITS);
	model[1].Wo = qt.decode_Wo(&c2.c2const, Wo_index, WO_BITS);
	model[1].L  = PI/model[1].Wo;

	e_index = qt.unpack(bits, &nbit, E_BITS);
	e[1] = qt.decode_energy(e_index, E_BITS);

	model[2].voiced = qt.unpack(bits, &nbit, 1);

	model[3].voiced = qt.unpack(bits, &nbit, 1);
	Wo_index = qt.unpack(bits, &nbit, WO_BITS);
	model[3].Wo = qt.decode_Wo(&c2.c2const, Wo_index, WO_BITS);
	model[3].L  = PI/model[3].Wo;

	e_index = qt.unpack(bits, &nbit, E_BITS);
	e[3] = qt.decode_energy(e_index, E_BITS);

	for(i=0; i<LSP_SCALAR_INDEXES; i++)
	{
		lsp_indexes[i] = qt.unpack(bits, &nbit, qt.lsp_bits(i));
	}
	qt.decode_lsps_scalar(&lsps[3][0], lsp_indexes, LPC_ORD);
	qt.check_lsp_order(&lsps[3][0], LPC_ORD);
	qt.bw_expand_lsps(&lsps[3][0], LPC_ORD, 50.0, 100.0);

	/* interpolate ------------------------------------------------*/

	/* Wo and energy are sampled every 20ms, so we interpolate just 1
	   10ms frame between 20ms samples */

	interp_Wo(&model[0], &c2.prev_model_dec, &model[1], c2.c2const.Wo_min);
	e[0] = interp_energy(c2.prev_e_dec, e[1]);
	interp_Wo(&model[2], &model[1], &model[3], c2.c2const.Wo_min);
	e[2] = interp_energy(e[1], e[3]);

	/* LSPs are sampled every 40ms so we interpolate the 3 frames in
	   between, then recover spectral amplitudes */

	for(i=0, weight=0.25; i<3; i++, weight += 0.25)
	{
		interpolate_lsp_ver2(&lsps[i][0], c2.prev_lsps_dec, &lsps[3][0], weight, LPC_ORD);
	}
	for(i=0; i<4; i++)
	{
		lsp_to_lpc(&lsps[i][0], &ak[i][0], LPC_ORD);
		qt.aks_to_M2(&(c2.fftr_fwd_cfg), &ak[i][0], LPC_ORD, &model[i], e[i], &snr, 0, c2.lpc_pf, c2.bass_boost, c2.beta, c2.gamma, Aw);
		qt.apply_lpc_correction(&model[i]);
		synthesise_one_frame(&speech[c2.n_samp*i], &model[i], Aw, 3.0);
	}

	/* update memories for next frame ----------------------------*/

	c2.prev_model_dec = model[3];
	c2.prev_e_dec = e[3];
	for(i=0; i<LPC_ORD; i++)
		c2.prev_lsps_dec[i] = lsps[3][i];

}

/*---------------------------------------------------------------------------* \

  FUNCTION....: synthesise_one_frame()
  AUTHOR......: David Rowe
  DATE CREATED: 23/8/2010

  Synthesise 80 speech samples (10ms) from model parameters.

\*---------------------------------------------------------------------------*/

void CCodec2::synthesise_one_frame(short speech[], MODEL *model, std::complex<float> Aw[], float gain)
{
	int     i;

	/* LPC based phase synthesis */
	std::complex<float> H[MAX_AMP+1];
	sample_phase(model, H, Aw);
	phase_synth_zero_order(c2.n_samp, model, &c2.ex_phase, H);

	postfilter(model, &c2.bg_est);
	synthesise(c2.n_samp, &(c2.fftr_inv_cfg), c2.Sn_.data(), model, c2.Pn.data(), 1);

	for(i=0; i<c2.n_samp; i++)
	{
		c2.Sn_[i] *= gain;
	}

	ear_protection(c2.Sn_.data(), c2.n_samp);

	for(i=0; i<c2.n_samp; i++)
	{
		if (c2.Sn_[i] > 32767.0)
			speech[i] = 32767;
		else if (c2.Sn_[i] < -32767.0)
			speech[i] = -32767;
		else
			speech[i] = c2.Sn_[i];
	}

}


/*---------------------------------------------------------------------------* \

  FUNCTION....: analyse_one_frame()
  AUTHOR......: David Rowe
  DATE CREATED: 23/8/2010

  Extract sinusoidal model parameters from 80 speech samples (10ms of
  speech).

\*---------------------------------------------------------------------------*/

void CCodec2::analyse_one_frame(MODEL *model, const short *speech)
{
	std::complex<float>    Sw[FFT_ENC];
	float   pitch;
	int     i;
	int     n_samp = c2.n_samp;
	int     m_pitch = c2.m_pitch;

	/* Read input speech */

	for(i=0; i<m_pitch-n_samp; i++)
		c2.Sn[i] = c2.Sn[i+n_samp];
	for(i=0; i<n_samp; i++)
		c2.Sn[i+m_pitch-n_samp] = speech[i];

	dft_speech(&c2.c2const, c2.fft_fwd_cfg, Sw, c2.Sn.data(), c2.w.data());

	/* Estimate pitch */
	nlp.nlp(c2.Sn.data(), n_samp, &pitch, &c2.prev_f0_enc);
	model->Wo = TWO_PI/pitch;
	model->L = PI/model->Wo;

	/* estimate model parameters */
	two_stage_pitch_refinement(&c2.c2const, model, Sw);

	/* estimate phases when doing ML experiments */
	estimate_amplitudes(model, Sw, 0);
	est_voicing_mbe(&c2.c2const, model, Sw, c2.W);
}


/*---------------------------------------------------------------------------* \

  FUNCTION....: ear_protection()
  AUTHOR......: David Rowe
  DATE CREATED: Nov 7 2012

  Limits output level to protect ears when there are bit errors or the input
  is overdriven.  This doesn't correct or mask bit errors, just reduces the
  worst of their damage.

\*---------------------------------------------------------------------------*/

void CCodec2::ear_protection(float in_out[], int n)
{
	float max_sample, over, gain;
	int   i;

	/* find maximum sample in frame */

	max_sample = 0.0;
	for(i=0; i<n; i++)
		if (in_out[i] > max_sample)
			max_sample = in_out[i];

	/* determine how far above set point */

	over = max_sample/30000.0;

	/* If we are x dB over set point we reduce level by 2x dB, this
	   attenuates major excursions in amplitude (likely to be caused
	   by bit errors) more than smaller ones */

	if (over > 1.0)
	{
		gain = 1.0/(over*over);
		for(i=0; i<n; i++)
			in_out[i] *= gain;
	}
}

/*---------------------------------------------------------------------------*\

  sample_phase()

  Samples phase at centre of each harmonic from and array of FFT_ENC
  DFT samples.

\*---------------------------------------------------------------------------*/

void CCodec2::sample_phase(MODEL *model,
				  std::complex<float> H[],
				  std::complex<float> A[]        /* LPC analysis filter in freq domain */
)
{
	int   m, b;
	float r;

	r = TWO_PI/(FFT_ENC);

	/* Sample phase at harmonics */

	for(m=1; m<=model->L; m++)
	{
		b = (int)(m*model->Wo/r + 0.5);
		H[m] = std::conj(A[b]);
	}
}


/*---------------------------------------------------------------------------*\

   phase_synth_zero_order()

   Synthesises phases based on SNR and a rule based approach.  No phase
   parameters are required apart from the SNR (which can be reduced to a
   1 bit V/UV decision per frame).

   The phase of each harmonic is modelled as the phase of a synthesis
   filter excited by an impulse.  In many Codec 2 modes the synthesis
   filter is a LPC filter. Unlike the first order model the position
   of the impulse is not transmitted, so we create an excitation pulse
   train using a rule based approach.

   Consider a pulse train with a pulse starting time n=0, with pulses
   repeated at a rate of Wo, the fundamental frequency.  A pulse train
   in the time domain is equivalent to harmonics in the frequency
   domain.  We can make an excitation pulse train using a sum of
   sinsusoids:

     for(m=1; m<=L; m++)
       ex[n] = cos(m*Wo*n)

   Note: the Octave script ../octave/phase.m is an example of this if
   you would like to try making a pulse train.

   The phase of each excitation harmonic is:

     arg(E[m]) = mWo

   where E[m] are the complex excitation (freq domain) samples,
   arg(x), just returns the phase of a complex sample x.

   As we don't transmit the pulse position for this model, we need to
   synthesise it.  Now the excitation pulses occur at a rate of Wo.
   This means the phase of the first harmonic advances by N_SAMP samples
   over a synthesis frame of N_SAMP samples.  For example if Wo is pi/20
   (200 Hz), then over a 10ms frame (N_SAMP=80 samples), the phase of the
   first harmonic would advance (pi/20)*80 = 4*pi or two complete
   cycles.

   We generate the excitation phase of the fundamental (first
   harmonic):

     arg[E[1]] = Wo*N_SAMP;

   We then relate the phase of the m-th excitation harmonic to the
   phase of the fundamental as:

     arg(E[m]) = m*arg(E[1])

   This E[m] then gets passed through the LPC synthesis filter to
   determine the final harmonic phase.

   Comparing to speech synthesised using original phases:

   - Through headphones speech synthesised with this model is not as
     good. Through a loudspeaker it is very close to original phases.

   - If there are voicing errors, the speech can sound clicky or
     staticy.  If V speech is mistakenly declared UV, this model tends to
     synthesise impulses or clicks, as there is usually very little shift or
     dispersion through the LPC synthesis filter.

   - When combined with LPC amplitude modelling there is an additional
     drop in quality.  I am not sure why, theory is interformant energy
     is raised making any phase errors more obvious.

   NOTES:

     1/ This synthesis model is effectively the same as a simple LPC-10
     vocoders, and yet sounds much better.  Why? Conventional wisdom
     (AMBE, MELP) says mixed voicing is required for high quality
     speech.

     2/ I am pretty sure the Lincoln Lab sinusoidal coding guys (like xMBE
     also from MIT) first described this zero phase model, I need to look
     up the paper.

     3/ Note that this approach could cause some discontinuities in
     the phase at the edge of synthesis frames, as no attempt is made
     to make sure that the phase tracks are continuous (the excitation
     phases are continuous, but not the final phases after filtering
     by the LPC spectra).  Technically this is a bad thing.  However
     this may actually be a good thing, disturbing the phase tracks a
     bit.  More research needed, e.g. test a synthesis model that adds
     a small delta-W to make phase tracks line up for voiced
     harmonics.

\*---------------------------------------------------------------------------*/

void CCodec2::phase_synth_zero_order(
	int    n_samp,
	MODEL *model,
	float *ex_phase,            /* excitation phase of fundamental        */
	std::complex<float>   H[]                  /* L synthesis filter freq domain samples */

)
{
	int   m;
	float new_phi;
	std::complex<float>  Ex[MAX_AMP+1];	  /* excitation samples */
	std::complex<float>  A_[MAX_AMP+1];	  /* synthesised harmonic samples */

	/*
	   Update excitation fundamental phase track, this sets the position
	   of each pitch pulse during voiced speech.  After much experiment
	   I found that using just this frame's Wo improved quality for UV
	   sounds compared to interpolating two frames Wo like this:

	   ex_phase[0] += (*prev_Wo+model->Wo)*N_SAMP/2;
	*/

	ex_phase[0] += (model->Wo)*n_samp;
	ex_phase[0] -= TWO_PI*floorf(ex_phase[0]/TWO_PI + 0.5);

	for(m=1; m<=model->L; m++)
	{

		/* generate excitation */

		if (model->voiced)
		{
			Ex[m] = std::polar(1.0f, ex_phase[0] * m);
		}
		else
		{

			/* When a few samples were tested I found that LPC filter
			   phase is not needed in the unvoiced case, but no harm in
			   keeping it.
			*/
			float phi = TWO_PI*(float)codec2_rand()/CODEC2_RAND_MAX;
			Ex[m] = std::polar(1.0f, phi);
		}

		/* filter using LPC filter */

		A_[m].real(H[m].real() * Ex[m].real() - H[m].imag() * Ex[m].imag());
		A_[m].imag(H[m].imag() * Ex[m].real() + H[m].real() * Ex[m].imag());

		/* modify sinusoidal phase */

		new_phi = atan2f(A_[m].imag(), A_[m].real()+1E-12);
		model->phi[m] = new_phi;
	}

}

/*---------------------------------------------------------------------------*\

  postfilter()

  The post filter is designed to help with speech corrupted by
  background noise.  The zero phase model tends to make speech with
  background noise sound "clicky".  With high levels of background
  noise the low level inter-formant parts of the spectrum will contain
  noise rather than speech harmonics, so modelling them as voiced
  (i.e. a continuous, non-random phase track) is inaccurate.

  Some codecs (like MBE) have a mixed voicing model that breaks the
  spectrum into voiced and unvoiced regions.  Several bits/frame
  (5-12) are required to transmit the frequency selective voicing
  information.  Mixed excitation also requires accurate voicing
  estimation (parameter estimators always break occasionally under
  exceptional conditions).

  In our case we use a post filter approach which requires no
  additional bits to be transmitted.  The decoder measures the average
  level of the background noise during unvoiced frames.  If a harmonic
  is less than this level it is made unvoiced by randomising it's
  phases.

  This idea is rather experimental.  Some potential problems that may
  happen:

  1/ If someone says "aaaaaaaahhhhhhhhh" will background estimator track
     up to speech level?  This would be a bad thing.

  2/ If background noise suddenly dissapears from the source speech does
     estimate drop quickly?  What is noise suddenly re-appears?

  3/ Background noise with a non-flat sepctrum.  Current algorithm just
     comsiders spectrum as a whole, but this could be broken up into
     bands, each with their own estimator.

  4/ Males and females with the same level of background noise.  Check
     performance the same.  Changing Wo affects width of each band, may
     affect bg energy estimates.

  5/ Not sure what happens during long periods of voiced speech
     e.g. "sshhhhhhh"

\*---------------------------------------------------------------------------*/

#define BG_THRESH 40.0	// only consider low levels signals for bg_est
#define BG_BETA    0.1	// averaging filter constant
#define BG_MARGIN  6.0	// harmonics this far above BG noise are
                        // randomised.  Helped make bg noise less
			            // spikey (impulsive) for mmt1, but speech was
                        // perhaps a little rougher.

void CCodec2::postfilter( MODEL *model, float *bg_est )
{
	int   m, uv;
	float e, thresh;

	/* determine average energy across spectrum */

	e = 1E-12;
	for(m=1; m<=model->L; m++)
		e += model->A[m]*model->A[m];

	assert(e > 0.0);
	e = 10.0*log10f(e/model->L);

	/* If beneath threhold, update bg estimate.  The idea
	   of the threshold is to prevent updating during high level
	   speech. */

	if ((e < BG_THRESH) && !model->voiced)
		*bg_est =  *bg_est*(1.0 - BG_BETA) + e*BG_BETA;

	/* now mess with phases during voiced frames to make any harmonics
	   less then our background estimate unvoiced.
	*/

	uv = 0;
	thresh = exp10f((*bg_est + BG_MARGIN)/20.0);
	if (model->voiced)
		for(m=1; m<=model->L; m++)
			if (model->A[m] < thresh)
			{
				model->phi[m] = (TWO_PI/CODEC2_RAND_MAX)*(float)codec2_rand();
				uv++;
			}
}

C2CONST CCodec2::c2const_create(int Fs, float framelength_s)
{
	C2CONST c2const;

	assert((Fs == 8000) || (Fs = 16000));
	c2const.Fs = Fs;
	c2const.n_samp = round(Fs*framelength_s);
	c2const.max_amp = floor(Fs*P_MAX_S/2);
	c2const.p_min = floor(Fs*P_MIN_S);
	c2const.p_max = floor(Fs*P_MAX_S);
	c2const.m_pitch = floor(Fs*M_PITCH_S);
	c2const.Wo_min = TWO_PI/c2const.p_max;
	c2const.Wo_max = TWO_PI/c2const.p_min;

	if (Fs == 8000)
	{
		c2const.nw = 279;
	}
	else
	{
		c2const.nw = 511;  /* actually a bit shorter in time but lets us maintain constant FFT size */
	}

	c2const.tw = Fs*TW_S;

	/*
	fprintf(stderr, "max_amp: %d m_pitch: %d\n", c2const.n_samp, c2const.m_pitch);
	fprintf(stderr, "p_min: %d p_max: %d\n", c2const.p_min, c2const.p_max);
	fprintf(stderr, "Wo_min: %f Wo_max: %f\n", c2const.Wo_min, c2const.Wo_max);
	fprintf(stderr, "nw: %d tw: %d\n", c2const.nw, c2const.tw);
	*/

	return c2const;
}

/*---------------------------------------------------------------------------*\

  FUNCTION....: make_analysis_window
  AUTHOR......: David Rowe
  DATE CREATED: 11/5/94

  Init function that generates the time domain analysis window and it's DFT.

\*---------------------------------------------------------------------------*/

void CCodec2::make_analysis_window(C2CONST *c2const, FFT_STATE *fft_fwd_cfg, float w[], float W[])
{
	float m;
	std::complex<float>  wshift[FFT_ENC];
	int   i,j;
	int   m_pitch = c2const->m_pitch;
	int   nw      = c2const->nw;

	/*
	   Generate Hamming window centered on M-sample pitch analysis window

	0            M/2           M-1
	|-------------|-------------|
	      |-------|-------|
	          nw samples

	   All our analysis/synthsis is centred on the M/2 sample.
	*/

	m = 0.0;
	for(i=0; i<m_pitch/2-nw/2; i++)
		w[i] = 0.0;
	for(i=m_pitch/2-nw/2,j=0; i<m_pitch/2+nw/2; i++,j++)
	{
		w[i] = 0.5 - 0.5*cosf(TWO_PI*j/(nw-1));
		m += w[i]*w[i];
	}
	for(i=m_pitch/2+nw/2; i<m_pitch; i++)
		w[i] = 0.0;

	/* Normalise - makes freq domain amplitude estimation straight
	   forward */

	m = 1.0/sqrtf(m*FFT_ENC);
	for(i=0; i<m_pitch; i++)
	{
		w[i] *= m;
	}

	/*
	   Generate DFT of analysis window, used for later processing.  Note
	   we modulo FFT_ENC shift the time domain window w[], this makes the
	   imaginary part of the DFT W[] equal to zero as the shifted w[] is
	   even about the n=0 time axis if nw is odd.  Having the imag part
	   of the DFT W[] makes computation easier.

	   0                      FFT_ENC-1
	   |-------------------------|

	    ----\               /----
	         \             /
	          \           /          <- shifted version of window w[n]
	           \         /
	            \       /
	             -------

	   |---------|     |---------|
	     nw/2              nw/2
	*/

	std::complex<float> temp[FFT_ENC];

	for(i=0; i<FFT_ENC; i++)
	{
		wshift[i] = std::complex<float>(0.0f, 0.0f);
	}
	for(i=0; i<nw/2; i++)
		wshift[i].real(w[i+m_pitch/2]);
	for(i=FFT_ENC-nw/2,j=m_pitch/2-nw/2; i<FFT_ENC; i++,j++)
		wshift[i].real(w[j]);

	kiss.fft(*fft_fwd_cfg, wshift, temp);

	/*
	    Re-arrange W[] to be symmetrical about FFT_ENC/2.  Makes later
	    analysis convenient.

	 Before:


	   0                 FFT_ENC-1
	   |----------|---------|
	   __                   _
	     \                 /
	      \_______________/

	 After:

	   0                 FFT_ENC-1
	   |----------|---------|
	             ___
	            /   \
	   ________/     \_______

	*/


	for(i=0; i<FFT_ENC/2; i++)
	{
		W[i] = temp[i + FFT_ENC / 2].real();
		W[i + FFT_ENC / 2] = temp[i].real();
	}

}

/*---------------------------------------------------------------------------*\

  FUNCTION....: dft_speech
  AUTHOR......: David Rowe
  DATE CREATED: 27/5/94

  Finds the DFT of the current speech input speech frame.

\*---------------------------------------------------------------------------*/

void CCodec2::dft_speech(C2CONST *c2const, FFT_STATE &fft_fwd_cfg, std::complex<float> Sw[], float Sn[], float w[])
{
    int  i;
    int  m_pitch = c2const->m_pitch;
    int   nw      = c2const->nw;

    for(i=0; i<FFT_ENC; i++) {
		Sw[i] = std::complex<float>(0.0f, 0.0f);
    }

    /* Centre analysis window on time axis, we need to arrange input
       to FFT this way to make FFT phases correct */

    /* move 2nd half to start of FFT input vector */

    for(i=0; i<nw/2; i++)
        Sw[i].real(Sn[i+m_pitch/2]*w[i+m_pitch/2]);

    /* move 1st half to end of FFT input vector */

    for(i=0; i<nw/2; i++)
        Sw[FFT_ENC-nw/2+i].real(Sn[i+m_pitch/2-nw/2]*w[i+m_pitch/2-nw/2]);

    nlp.codec2_fft_inplace(fft_fwd_cfg, Sw);
}

/*---------------------------------------------------------------------------*\

  FUNCTION....: two_stage_pitch_refinement
  AUTHOR......: David Rowe
  DATE CREATED: 27/5/94

  Refines the current pitch estimate using the harmonic sum pitch
  estimation technique.

\*---------------------------------------------------------------------------*/

void CCodec2::two_stage_pitch_refinement(C2CONST *c2const, MODEL *model, std::complex<float> Sw[])
{
	float pmin,pmax,pstep;	/* pitch refinment minimum, maximum and step */

	/* Coarse refinement */

	pmax = TWO_PI/model->Wo + 5;
	pmin = TWO_PI/model->Wo - 5;
	pstep = 1.0;
	hs_pitch_refinement(model, Sw, pmin, pmax, pstep);

	/* Fine refinement */

	pmax = TWO_PI/model->Wo + 1;
	pmin = TWO_PI/model->Wo - 1;
	pstep = 0.25;
	hs_pitch_refinement(model,Sw,pmin,pmax,pstep);

	/* Limit range */

	if (model->Wo < TWO_PI/c2const->p_max)
		model->Wo = TWO_PI/c2const->p_max;
	if (model->Wo > TWO_PI/c2const->p_min)
		model->Wo = TWO_PI/c2const->p_min;

	model->L = floorf(PI/model->Wo);

	/* trap occasional round off issues with floorf() */
	if (model->Wo*model->L >= 0.95*PI)
	{
		model->L--;
	}
	assert(model->Wo*model->L < PI);
}

/*---------------------------------------------------------------------------*\

 FUNCTION....: hs_pitch_refinement
 AUTHOR......: David Rowe
 DATE CREATED: 27/5/94

 Harmonic sum pitch refinement function.

 pmin   pitch search range minimum
 pmax	pitch search range maximum
 step   pitch search step size
 model	current pitch estimate in model.Wo

 model 	refined pitch estimate in model.Wo

\*---------------------------------------------------------------------------*/

void CCodec2::hs_pitch_refinement(MODEL *model, std::complex<float> Sw[], float pmin, float pmax, float pstep)
{
	int m;		/* loop variable */
	int b;		/* bin for current harmonic centre */
	float E;		/* energy for current pitch*/
	float Wo;		/* current "test" fundamental freq. */
	float Wom;		/* Wo that maximises E */
	float Em;		/* mamimum energy */
	float r, one_on_r;	/* number of rads/bin */
	float p;		/* current pitch */

	/* Initialisation */

	model->L = PI/model->Wo;	/* use initial pitch est. for L */
	Wom = model->Wo;
	Em = 0.0;
	r = TWO_PI/FFT_ENC;
	one_on_r = 1.0/r;

	/* Determine harmonic sum for a range of Wo values */

	for(p=pmin; p<=pmax; p+=pstep)
	{
		E = 0.0;
		Wo = TWO_PI/p;

		/* Sum harmonic magnitudes */
		for(m=1; m<=model->L; m++)
		{
			b = (int)(m*Wo*one_on_r + 0.5);
			E += Sw[b].real() * Sw[b].real() + Sw[b].imag() * Sw[b].imag();
		}
		/* Compare to see if this is a maximum */

		if (E > Em)
		{
			Em = E;
			Wom = Wo;
		}
	}

	model->Wo = Wom;
}

/*---------------------------------------------------------------------------*\

  FUNCTION....: estimate_amplitudes
  AUTHOR......: David Rowe
  DATE CREATED: 27/5/94

  Estimates the complex amplitudes of the harmonics.

\*---------------------------------------------------------------------------*/

void CCodec2::estimate_amplitudes(MODEL *model, std::complex<float> Sw[], int est_phase)
{
	int   i,m;		/* loop variables */
	int   am,bm;		/* bounds of current harmonic */
	float den;		/* denominator of amplitude expression */

	float r = TWO_PI/FFT_ENC;
	float one_on_r = 1.0/r;

	for(m=1; m<=model->L; m++)
	{
		/* Estimate ampltude of harmonic */

		den = 0.0;
		am = (int)((m - 0.5)*model->Wo*one_on_r + 0.5);
		bm = (int)((m + 0.5)*model->Wo*one_on_r + 0.5);

		for(i=am; i<bm; i++)
		{
			den += Sw[i].real() * Sw[i].real() + Sw[i].imag() * Sw[i].imag();
		}

		model->A[m] = sqrtf(den);

		if (est_phase)
		{
			int b = (int)(m*model->Wo/r + 0.5); /* DFT bin of centre of current harmonic */

			/* Estimate phase of harmonic, this is expensive in CPU for
			   embedded devicesso we make it an option */

			model->phi[m] = atan2f(Sw[b].imag(), Sw[b].real());
		}
	}
}

/*---------------------------------------------------------------------------*\

  est_voicing_mbe()

  Returns the error of the MBE cost function for a fiven F0.

  Note: I think a lot of the operations below can be simplified as
  W[].imag = 0 and has been normalised such that den always equals 1.

\*---------------------------------------------------------------------------*/

float CCodec2::est_voicing_mbe( C2CONST *c2const, MODEL *model, std::complex<float> Sw[], float  W[])
{
	int   l,al,bl,m;    /* loop variables */
	std::complex<float>  Am;             /* amplitude sample for this band */
	int   offset;         /* centers Hw[] about current harmonic */
	float den;            /* denominator of Am expression */
	float error;          /* accumulated error between original and synthesised */
	float Wo;
	float sig, snr;
	float elow, ehigh, eratio;
	float sixty;
	std::complex<float> Ew(0, 0);

	int l_1000hz = model->L*1000.0/(c2const->Fs/2);
	sig = 1E-4;
	for(l=1; l<=l_1000hz; l++)
	{
		sig += model->A[l]*model->A[l];
	}

	Wo = model->Wo;
	error = 1E-4;

	/* Just test across the harmonics in the first 1000 Hz */

	for(l=1; l<=l_1000hz; l++)
	{
		Am = std::complex<float>(0.0f, 0.0f);
		den = 0.0;
		al = ceilf((l - 0.5)*Wo*FFT_ENC/TWO_PI);
		bl = ceilf((l + 0.5)*Wo*FFT_ENC/TWO_PI);

		/* Estimate amplitude of harmonic assuming harmonic is totally voiced */

		offset = FFT_ENC/2 - l*Wo*FFT_ENC/TWO_PI + 0.5;
		for(m=al; m<bl; m++)
		{
			Am += W[offset+m] * Sw[m];
			den += W[offset+m]*W[offset+m];
		}

		Am /= den;

		/* Determine error between estimated harmonic and original */

		for(m=al; m<bl; m++)
		{
			Ew = Sw[m] - (W[offset+m] * Am);
			error += Ew.real() * Ew.real() + Ew.imag() * Ew.imag();
		}
	}

	snr = 10.0*log10f(sig/error);
	if (snr > V_THRESH)
		model->voiced = 1;
	else
		model->voiced = 0;

	/* post processing, helps clean up some voicing errors ------------------*/

	/*
	   Determine the ratio of low freqency to high frequency energy,
	   voiced speech tends to be dominated by low frequency energy,
	   unvoiced by high frequency. This measure can be used to
	   determine if we have made any gross errors.
	*/

	int l_2000hz = model->L*2000.0/(c2const->Fs/2);
	int l_4000hz = model->L*4000.0/(c2const->Fs/2);
	elow = ehigh = 1E-4;
	for(l=1; l<=l_2000hz; l++)
	{
		elow += model->A[l]*model->A[l];
	}
	for(l=l_2000hz; l<=l_4000hz; l++)
	{
		ehigh += model->A[l]*model->A[l];
	}
	eratio = 10.0*log10f(elow/ehigh);

	/* Look for Type 1 errors, strongly V speech that has been
	   accidentally declared UV */

	if (model->voiced == 0)
		if (eratio > 10.0)
			model->voiced = 1;

	/* Look for Type 2 errors, strongly UV speech that has been
	   accidentally declared V */

	if (model->voiced == 1)
	{
		if (eratio < -10.0)
			model->voiced = 0;

		/* A common source of Type 2 errors is the pitch estimator
		   gives a low (50Hz) estimate for UV speech, which gives a
		   good match with noise due to the close harmoonic spacing.
		   These errors are much more common than people with 50Hz3
		   pitch, so we have just a small eratio threshold. */

		sixty = 60.0*TWO_PI/c2const->Fs;
		if ((eratio < -4.0) && (model->Wo <= sixty))
			model->voiced = 0;
	}
	//printf(" v: %d snr: %f eratio: %3.2f %f\n",model->voiced,snr,eratio,dF0);

	return snr;
}

/*---------------------------------------------------------------------------*\

  FUNCTION....: make_synthesis_window
  AUTHOR......: David Rowe
  DATE CREATED: 11/5/94

  Init function that generates the trapezoidal (Parzen) sythesis window.

\*---------------------------------------------------------------------------*/

void CCodec2::make_synthesis_window(C2CONST *c2const, float Pn[])
{
	int   i;
	float win;
	int   n_samp = c2const->n_samp;
	int   tw     = c2const->tw;

	/* Generate Parzen window in time domain */

	win = 0.0;
	for(i=0; i<n_samp/2-tw; i++)
		Pn[i] = 0.0;
	win = 0.0;
	for(i=n_samp/2-tw; i<n_samp/2+tw; win+=1.0/(2*tw), i++ )
		Pn[i] = win;
	for(i=n_samp/2+tw; i<3*n_samp/2-tw; i++)
		Pn[i] = 1.0;
	win = 1.0;
	for(i=3*n_samp/2-tw; i<3*n_samp/2+tw; win-=1.0/(2*tw), i++)
		Pn[i] = win;
	for(i=3*n_samp/2+tw; i<2*n_samp; i++)
		Pn[i] = 0.0;
}

/*---------------------------------------------------------------------------*\

  FUNCTION....: synthesise
  AUTHOR......: David Rowe
  DATE CREATED: 20/2/95

  Synthesise a speech signal in the frequency domain from the
  sinusodal model parameters.  Uses overlap-add with a trapezoidal
  window to smoothly interpolate betwen frames.

\*---------------------------------------------------------------------------*/

void CCodec2::synthesise(
	int    n_samp,
	FFTR_STATE *fftr_inv_cfg,
	float  Sn_[],		/* time domain synthesised signal              */
	MODEL *model,		/* ptr to model parameters for this frame      */
	float  Pn[],		/* time domain Parzen window                   */
	int    shift          /* flag used to handle transition frames       */
)
{
	int   i,l,j,b;	        /* loop variables */
	std::complex<float>  Sw_[FFT_DEC/2+1];	/* DFT of synthesised signal */
	float sw_[FFT_DEC];	        /* synthesised signal */

	if (shift)
	{
		/* Update memories */
		for(i=0; i<n_samp-1; i++)
		{
			Sn_[i] = Sn_[i+n_samp];
		}
		Sn_[n_samp-1] = 0.0;
	}

	for(i=0; i<FFT_DEC/2+1; i++)
	{
		Sw_[i].real(0);
		Sw_[i].imag(0);
	}

	/* Now set up frequency domain synthesised speech */

	for(l=1; l<=model->L; l++)
	{
		b = (int)(l*model->Wo*FFT_DEC/TWO_PI + 0.5);
		if (b > ((FFT_DEC/2)-1))
		{
			b = (FFT_DEC/2)-1;
		}
		Sw_[b] = std::polar(model->A[l], model->phi[l]);
	}

	/* Perform inverse DFT */

	kiss.fftri(*fftr_inv_cfg, Sw_,sw_);

	/* Overlap add to previous samples */

	for(i=0; i<n_samp-1; i++)
	{
		Sn_[i] += sw_[FFT_DEC-n_samp+1+i]*Pn[i];
	}

	if (shift)
		for(i=n_samp-1,j=0; i<2*n_samp; i++,j++)
			Sn_[i] = sw_[j]*Pn[i];
	else
		for(i=n_samp-1,j=0; i<2*n_samp; i++,j++)
			Sn_[i] += sw_[j]*Pn[i];
}

int CCodec2::codec2_rand(void)
{
	static unsigned long next = 1;
	next = next * 1103515245 + 12345;
	return((unsigned)(next/65536) % 32768);
}

/*---------------------------------------------------------------------------*\

  FUNCTION....: interp_Wo()
  AUTHOR......: David Rowe
  DATE CREATED: 22 May 2012

  Interpolates centre 10ms sample of Wo and L samples given two
  samples 20ms apart. Assumes voicing is available for centre
  (interpolated) frame.

\*---------------------------------------------------------------------------*/

void CCodec2::interp_Wo(
	MODEL *interp,    /* interpolated model params                     */
	MODEL *prev,      /* previous frames model params                  */
	MODEL *next,      /* next frames model params                      */
	float  Wo_min
)
{
	interp_Wo2(interp, prev, next, 0.5, Wo_min);
}

/*---------------------------------------------------------------------------*\

  FUNCTION....: interp_Wo2()
  AUTHOR......: David Rowe
  DATE CREATED: 22 May 2012

  Weighted interpolation of two Wo samples.

\*---------------------------------------------------------------------------*/

void CCodec2::interp_Wo2(
	MODEL *interp,    /* interpolated model params                     */
	MODEL *prev,      /* previous frames model params                  */
	MODEL *next,      /* next frames model params                      */
	float  weight,
	float  Wo_min
)
{
	/* trap corner case where voicing est is probably wrong */

	if (interp->voiced && !prev->voiced && !next->voiced)
	{
		interp->voiced = 0;
	}

	/* Wo depends on voicing of this and adjacent frames */

	if (interp->voiced)
	{
		if (prev->voiced && next->voiced)
			interp->Wo = (1.0 - weight)*prev->Wo + weight*next->Wo;
		if (!prev->voiced && next->voiced)
			interp->Wo = next->Wo;
		if (prev->voiced && !next->voiced)
			interp->Wo = prev->Wo;
	}
	else
	{
		interp->Wo = Wo_min;
	}
	interp->L = PI/interp->Wo;
}

/*---------------------------------------------------------------------------*\

  FUNCTION....: interp_energy()
  AUTHOR......: David Rowe
  DATE CREATED: 22 May 2012

  Interpolates centre 10ms sample of energy given two samples 20ms
  apart.

\*---------------------------------------------------------------------------*/

float CCodec2::interp_energy(float prev_e, float next_e)
{
	//return powf(10.0, (log10f(prev_e) + log10f(next_e))/2.0);
	return sqrtf(prev_e * next_e); //looks better is math. identical and faster math
}

/*---------------------------------------------------------------------------*\

  FUNCTION....: interpolate_lsp_ver2()
  AUTHOR......: David Rowe
  DATE CREATED: 22 May 2012

  Weighted interpolation of LSPs.

\*---------------------------------------------------------------------------*/

void CCodec2::interpolate_lsp_ver2(float interp[], float prev[],  float next[], float weight, int order)
{
	int i;

	for(i=0; i<order; i++)
		interp[i] = (1.0 - weight)*prev[i] + weight*next[i];
}

/*---------------------------------------------------------------------------*\

  Introduction to Line Spectrum Pairs (LSPs)
  ------------------------------------------

  LSPs are used to encode the LPC filter coefficients {ak} for
  transmission over the channel.  LSPs have several properties (like
  less sensitivity to quantisation noise) that make them superior to
  direct quantisation of {ak}.

  A(z) is a polynomial of order lpcrdr with {ak} as the coefficients.

  A(z) is transformed to P(z) and Q(z) (using a substitution and some
  algebra), to obtain something like:

    A(z) = 0.5[P(z)(z+z^-1) + Q(z)(z-z^-1)]  (1)

  As you can imagine A(z) has complex zeros all over the z-plane. P(z)
  and Q(z) have the very neat property of only having zeros _on_ the
  unit circle.  So to find them we take a test point z=exp(jw) and
  evaluate P (exp(jw)) and Q(exp(jw)) using a grid of points between 0
  and pi.

  The zeros (roots) of P(z) also happen to alternate, which is why we
  swap coefficients as we find roots.  So the process of finding the
  LSP frequencies is basically finding the roots of 5th order
  polynomials.

  The root so P(z) and Q(z) occur in symmetrical pairs at +/-w, hence
  the name Line Spectrum Pairs (LSPs).

  To convert back to ak we just evaluate (1), "clocking" an impulse
  thru it lpcrdr times gives us the impulse response of A(z) which is
  {ak}.

\*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*\

  FUNCTION....: lsp_to_lpc()
  AUTHOR......: David Rowe
  DATE CREATED: 24/2/93

  This function converts LSP coefficients to LPC coefficients.  In the
  Speex code we worked out a way to simplify this significantly.

\*---------------------------------------------------------------------------*/

void CCodec2::lsp_to_lpc(float *lsp, float *ak, int order)
/*  float *freq         array of LSP frequencies in radians     	*/
/*  float *ak 		array of LPC coefficients 			*/
/*  int order     	order of LPC coefficients 			*/


{
	int i,j;
	float xout1,xout2,xin1,xin2;
	float *pw,*n1,*n2,*n3,*n4 = 0;
	float freq[order];
	float Wp[(order * 4) + 2];

	/* convert from radians to the x=cos(w) domain */

	for(i=0; i<order; i++)
		freq[i] = cosf(lsp[i]);

	pw = Wp;

	/* initialise contents of array */

	for(i=0; i<=4*(order/2)+1; i++)        	/* set contents of buffer to 0 */
	{
		*pw++ = 0.0;
	}

	/* Set pointers up */

	pw = Wp;
	xin1 = 1.0;
	xin2 = 1.0;

	/* reconstruct P(z) and Q(z) by cascading second order polynomials
	  in form 1 - 2xz(-1) +z(-2), where x is the LSP coefficient */

	for(j=0; j<=order; j++)
	{
		for(i=0; i<(order/2); i++)
		{
			n1 = pw+(i*4);
			n2 = n1 + 1;
			n3 = n2 + 1;
			n4 = n3 + 1;
			xout1 = xin1 - 2*(freq[2*i]) * *n1 + *n2;
			xout2 = xin2 - 2*(freq[2*i+1]) * *n3 + *n4;
			*n2 = *n1;
			*n4 = *n3;
			*n1 = xin1;
			*n3 = xin2;
			xin1 = xout1;
			xin2 = xout2;
		}
		xout1 = xin1 + *(n4+1);
		xout2 = xin2 - *(n4+2);
		ak[j] = (xout1 + xout2)*0.5;
		*(n4+1) = xin1;
		*(n4+2) = xin2;

		xin1 = 0.0;
		xin2 = 0.0;
	}
}
