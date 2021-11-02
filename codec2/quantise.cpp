/*---------------------------------------------------------------------------*\

  FILE........: quantise.c
  AUTHOR......: David Rowe
  DATE CREATED: 31/5/92

  Quantisation functions for the sinusoidal coder.

\*---------------------------------------------------------------------------*/

/*
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

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "defines.h"
#include "quantise.h"
#include "lpc.h"
#include "kiss_fft.h"

extern CKissFFT kiss;

#define LSP_DELTA1 0.01         /* grid spacing for LSP root searches */

/*---------------------------------------------------------------------------*\

                             FUNCTIONS

\*---------------------------------------------------------------------------*/

int CQuantize::lsp_bits(int i)
{
	return lsp_cb[i].log2m;
}

int CQuantize::lspd_bits(int i)
{
	return lsp_cbd[i].log2m;
}



/*---------------------------------------------------------------------------*\

  encode_lspds_scalar()

  Scalar/VQ LSP difference quantiser.

\*---------------------------------------------------------------------------*/

void CQuantize::encode_lspds_scalar(int indexes[], float lsp[], int order)
{
	int   i,k,m;
	float lsp_hz[order];
	float lsp__hz[order];
	float dlsp[order];
	float dlsp_[order];
	float wt[order];
	const float *cb;
	float se;

	for(i=0; i<order; i++)
	{
		wt[i] = 1.0;
	}

	/* convert from radians to Hz so we can use human readable
	   frequencies */

	for(i=0; i<order; i++)
		lsp_hz[i] = (4000.0/PI)*lsp[i];

	wt[0] = 1.0;
	for(i=0; i<order; i++)
	{

		/* find difference from previous qunatised lsp */

		if (i)
			dlsp[i] = lsp_hz[i] - lsp__hz[i-1];
		else
			dlsp[0] = lsp_hz[0];

		k = lsp_cbd[i].k;
		m = lsp_cbd[i].m;
		cb = lsp_cbd[i].cb;
		indexes[i] = quantise(cb, &dlsp[i], wt, k, m, &se);
		dlsp_[i] = cb[indexes[i]*k];


		if (i)
			lsp__hz[i] = lsp__hz[i-1] + dlsp_[i];
		else
			lsp__hz[0] = dlsp_[0];
	}

}


void CQuantize::decode_lspds_scalar( float lsp_[], int indexes[], int   order)
{
	int   i,k;
	float lsp__hz[order];
	float dlsp_[order];
	const float *cb;

	for(i=0; i<order; i++)
	{

		k = lsp_cbd[i].k;
		cb = lsp_cbd[i].cb;
		dlsp_[i] = cb[indexes[i]*k];

		if (i)
			lsp__hz[i] = lsp__hz[i-1] + dlsp_[i];
		else
			lsp__hz[0] = dlsp_[0];

		lsp_[i] = (PI/4000.0)*lsp__hz[i];
	}

}

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX_ENTRIES 16384

void CQuantize::compute_weights(const float *x, float *w, int ndim)
{
	int i;
	w[0] = MIN(x[0], x[1]-x[0]);
	for (i=1; i<ndim-1; i++)
		w[i] = MIN(x[i]-x[i-1], x[i+1]-x[i]);
	w[ndim-1] = MIN(x[ndim-1]-x[ndim-2], PI-x[ndim-1]);

	for (i=0; i<ndim; i++)
		w[i] = 1./(.01+w[i]);
}

int CQuantize::find_nearest(const float *codebook, int nb_entries, float *x, int ndim)
{
	int i, j;
	float min_dist = 1e15;
	int nearest = 0;

	for (i=0; i<nb_entries; i++)
	{
		float dist=0;
		for (j=0; j<ndim; j++)
			dist += (x[j]-codebook[i*ndim+j])*(x[j]-codebook[i*ndim+j]);
		if (dist<min_dist)
		{
			min_dist = dist;
			nearest = i;
		}
	}
	return nearest;
}

int CQuantize::check_lsp_order(float lsp[], int order)
{
	int   i;
	float tmp;
	int   swaps = 0;

	for(i=1; i<order; i++)
		if (lsp[i] < lsp[i-1])
		{
			//fprintf(stderr, "swap %d\n",i);
			swaps++;
			tmp = lsp[i-1];
			lsp[i-1] = lsp[i]-0.1;
			lsp[i] = tmp+0.1;
			i = 1; /* start check again, as swap may have caused out of order */
		}

	return swaps;
}


/*---------------------------------------------------------------------------*\

   lpc_post_filter()

   Applies a post filter to the LPC synthesis filter power spectrum
   Pw, which supresses the inter-formant energy.

   The algorithm is from p267 (Section 8.6) of "Digital Speech",
   edited by A.M. Kondoz, 1994 published by Wiley and Sons.  Chapter 8
   of this text is on the MBE vocoder, and this is a freq domain
   adaptation of post filtering commonly used in CELP.

   I used the Octave simulation lpcpf.m to get an understanding of the
   algorithm.

   Requires two more FFTs which is significantly more MIPs.  However
   it should be possible to implement this more efficiently in the
   time domain.  Just not sure how to handle relative time delays
   between the synthesis stage and updating these coeffs.  A smaller
   FFT size might also be accetable to save CPU.

   TODO:
   [ ] sync var names between Octave and C version
   [ ] doc gain normalisation
   [ ] I think the first FFT is not rqd as we do the same
       thing in aks_to_M2().

\*---------------------------------------------------------------------------*/

void CQuantize::lpc_post_filter(FFTR_STATE *fftr_fwd_cfg, float Pw[], float ak[], int order, float beta, float gamma, int bass_boost, float E)
{
	int   i;
	float x[FFT_ENC];   /* input to FFTs                */
	std::complex<float>  Ww[FFT_ENC/2+1];  /* weighting spectrum           */
	float Rw[FFT_ENC/2+1];  /* R = WA                       */
	float e_before, e_after, gain;
	float Pfw;
	float max_Rw, min_Rw;
	float coeff;

	/* Determine weighting filter spectrum W(exp(jw)) ---------------*/

	for(i=0; i<FFT_ENC; i++)
	{
		x[i] = 0.0;
	}

	x[0]  = ak[0];
	coeff = gamma;
	for(i=1; i<=order; i++)
	{
		x[i] = ak[i] * coeff;
		coeff *= gamma;
	}
	kiss.fftr(*fftr_fwd_cfg, x, Ww);

	for(i=0; i<FFT_ENC/2; i++)
	{
		Ww[i].real(Ww[i].real() * Ww[i].real() + Ww[i].imag() * Ww[i].imag());
	}

	/* Determined combined filter R = WA ---------------------------*/

	max_Rw = 0.0;
	min_Rw = 1E32;
	for(i=0; i<FFT_ENC/2; i++)
	{
		Rw[i] = sqrtf(Ww[i].real() * Pw[i]);
		if (Rw[i] > max_Rw)
			max_Rw = Rw[i];
		if (Rw[i] < min_Rw)
			min_Rw = Rw[i];

	}

	/* create post filter mag spectrum and apply ------------------*/

	/* measure energy before post filtering */

	e_before = 1E-4;
	for(i=0; i<FFT_ENC/2; i++)
		e_before += Pw[i];

	/* apply post filter and measure energy  */


	e_after = 1E-4;
	for(i=0; i<FFT_ENC/2; i++)
	{
		Pfw = powf(Rw[i], beta);
		Pw[i] *= Pfw * Pfw;
		e_after += Pw[i];
	}
	gain = e_before/e_after;

	/* apply gain factor to normalise energy, and LPC Energy */

	gain *= E;
	for(i=0; i<FFT_ENC/2; i++)
	{
		Pw[i] *= gain;
	}

	if (bass_boost)
	{
		/* add 3dB to first 1 kHz to account for LP effect of PF */

		for(i=0; i<FFT_ENC/8; i++)
		{
			Pw[i] *= 1.4*1.4;
		}
	}
}


/*---------------------------------------------------------------------------*\

   aks_to_M2()

   Transforms the linear prediction coefficients to spectral amplitude
   samples.  This function determines A(m) from the average energy per
   band using an FFT.

\*---------------------------------------------------------------------------*/

void CQuantize::aks_to_M2(
	FFTR_STATE * fftr_fwd_cfg,
	float         ak[],	     /* LPC's */
	int           order,
	MODEL        *model,	   /* sinusoidal model parameters for this frame */
	float         E,	       /* energy term */
	float        *snr,	       /* signal to noise ratio for this frame in dB */
	int           sim_pf,      /* true to simulate a post filter */
	int           pf,          /* true to enable actual LPC post filter */
	int           bass_boost,  /* enable LPC filter 0-1kHz 3dB boost */
	float         beta,
	float         gamma,       /* LPC post filter parameters */
	std::complex<float>          Aw[]         /* output power spectrum */
)
{
	int i,m;		/* loop variables */
	int am,bm;		/* limits of current band */
	float r;		/* no. rads/bin */
	float Em;		/* energy in band */
	float Am;		/* spectral amplitude sample */
	float signal, noise;

	r = TWO_PI/(FFT_ENC);

	/* Determine DFT of A(exp(jw)) --------------------------------------------*/
	{
		float a[FFT_ENC];  /* input to FFT for power spectrum */

		for(i=0; i<FFT_ENC; i++)
		{
			a[i] = 0.0;
		}

		for(i=0; i<=order; i++)
			a[i] = ak[i];
		kiss.fftr(*fftr_fwd_cfg, a, Aw);
	}

	/* Determine power spectrum P(w) = E/(A(exp(jw))^2 ------------------------*/

	float Pw[FFT_ENC/2];

	for(i=0; i<FFT_ENC/2; i++)
	{
		Pw[i] = 1.0/(Aw[i].real() * Aw[i].real() + Aw[i].imag() * Aw[i].imag() + 1E-6);
	}

	if (pf)
		lpc_post_filter(fftr_fwd_cfg, Pw, ak, order, beta, gamma, bass_boost, E);
	else
	{
		for(i=0; i<FFT_ENC/2; i++)
		{
			Pw[i] *= E;
		}
	}

	/* Determine magnitudes from P(w) ----------------------------------------*/

	/* when used just by decoder {A} might be all zeroes so init signal
	   and noise to prevent log(0) errors */

	signal = 1E-30;
	noise = 1E-32;

	for(m=1; m<=model->L; m++)
	{
		am = (int)((m - 0.5)*model->Wo/r + 0.5);
		bm = (int)((m + 0.5)*model->Wo/r + 0.5);

		// FIXME: With arm_rfft_fast_f32 we have to use this
		// otherwise sometimes a to high bm is calculated
		// which causes trouble later in the calculation
		// chain
		// it seems for some reason model->Wo is calculated somewhat too high
		if (bm>FFT_ENC/2)
		{
			bm = FFT_ENC/2;
		}
		Em = 0.0;

		for(i=am; i<bm; i++)
			Em += Pw[i];
		Am = sqrtf(Em);

		signal += model->A[m]*model->A[m];
		noise  += (model->A[m] - Am)*(model->A[m] - Am);

		/* This code significantly improves perf of LPC model, in
		   particular when combined with phase0.  The LPC spectrum tends
		   to track just under the peaks of the spectral envelope, and
		   just above nulls.  This algorithm does the reverse to
		   compensate - raising the amplitudes of spectral peaks, while
		   attenuating the null.  This enhances the formants, and
		   supresses the energy between formants. */

		if (sim_pf)
		{
			if (Am > model->A[m])
				Am *= 0.7;
			if (Am < model->A[m])
				Am *= 1.4;
		}
		model->A[m] = Am;
	}
	*snr = 10.0*log10f(signal/noise);
}

/*---------------------------------------------------------------------------*\

  FUNCTION....: encode_Wo()
  AUTHOR......: David Rowe
  DATE CREATED: 22/8/2010

  Encodes Wo using a WO_LEVELS quantiser.

\*---------------------------------------------------------------------------*/

int CQuantize::encode_Wo(C2CONST *c2const, float Wo, int bits)
{
	int   index, Wo_levels = 1<<bits;
	float Wo_min = c2const->Wo_min;
	float Wo_max = c2const->Wo_max;
	float norm;

	norm = (Wo - Wo_min)/(Wo_max - Wo_min);
	index = floorf(Wo_levels * norm + 0.5);
	if (index < 0 ) index = 0;
	if (index > (Wo_levels-1)) index = Wo_levels-1;

	return index;
}

/*---------------------------------------------------------------------------*\

  FUNCTION....: decode_Wo()
  AUTHOR......: David Rowe
  DATE CREATED: 22/8/2010

  Decodes Wo using a WO_LEVELS quantiser.

\*---------------------------------------------------------------------------*/

float CQuantize::decode_Wo(C2CONST *c2const, int index, int bits)
{
	float Wo_min = c2const->Wo_min;
	float Wo_max = c2const->Wo_max;
	float step;
	float Wo;
	int   Wo_levels = 1<<bits;

	step = (Wo_max - Wo_min)/Wo_levels;
	Wo   = Wo_min + step*(index);

	return Wo;
}

/*---------------------------------------------------------------------------*\

  FUNCTION....: speech_to_uq_lsps()
  AUTHOR......: David Rowe
  DATE CREATED: 22/8/2010

  Analyse a windowed frame of time domain speech to determine LPCs
  which are the converted to LSPs for quantisation and transmission
  over the channel.

\*---------------------------------------------------------------------------*/

float CQuantize::speech_to_uq_lsps(float lsp[], float ak[], float Sn[], float w[], int m_pitch, int order)
{
	int   i, roots;
	float Wn[m_pitch];
	float R[order+1];
	float e, E;
	Clpc lpc;

	e = 0.0;
	for(i=0; i<m_pitch; i++)
	{
		Wn[i] = Sn[i]*w[i];
		e += Wn[i]*Wn[i];
	}

	/* trap 0 energy case as LPC analysis will fail */

	if (e == 0.0)
	{
		for(i=0; i<order; i++)
			lsp[i] = (PI/order)*(float)i;
		return 0.0;
	}

	lpc.autocorrelate(Wn, R, m_pitch, order);
	lpc.levinson_durbin(R, ak, order);

	E = 0.0;
	for(i=0; i<=order; i++)
		E += ak[i]*R[i];

	/* 15 Hz BW expansion as I can't hear the difference and it may help
	   help occasional fails in the LSP root finding.  Important to do this
	   after energy calculation to avoid -ve energy values.
	*/

	for(i=0; i<=order; i++)
		ak[i] *= powf(0.994,(float)i);

	roots = lpc_to_lsp(ak, order, lsp, 5, LSP_DELTA1);
	if (roots != order)
	{
		/* if root finding fails use some benign LSP values instead */
		for(i=0; i<order; i++)
			lsp[i] = (PI/order)*(float)i;
	}

	return E;
}

/*---------------------------------------------------------------------------*\

  FUNCTION....: encode_lsps_scalar()
  AUTHOR......: David Rowe
  DATE CREATED: 22/8/2010

  Thirty-six bit sclar LSP quantiser. From a vector of unquantised
  (floating point) LSPs finds the quantised LSP indexes.

\*---------------------------------------------------------------------------*/

void CQuantize::encode_lsps_scalar(int indexes[], float lsp[], int order)
{
	int    i,k,m;
	float  wt[1];
	float  lsp_hz[order];
	const float *cb;
	float se;

	/* convert from radians to Hz so we can use human readable
	   frequencies */

	for(i=0; i<order; i++)
		lsp_hz[i] = (4000.0/PI)*lsp[i];

	/* scalar quantisers */

	wt[0] = 1.0;
	for(i=0; i<order; i++)
	{
		k = lsp_cb[i].k;
		m = lsp_cb[i].m;
		cb = lsp_cb[i].cb;
		indexes[i] = quantise(cb, &lsp_hz[i], wt, k, m, &se);
	}
}

/*---------------------------------------------------------------------------*\

  FUNCTION....: decode_lsps_scalar()
  AUTHOR......: David Rowe
  DATE CREATED: 22/8/2010

  From a vector of quantised LSP indexes, returns the quantised
  (floating point) LSPs.

\*---------------------------------------------------------------------------*/

void CQuantize::decode_lsps_scalar(float lsp[], int indexes[], int order)
{
	int    i,k;
	float  lsp_hz[order];
	const float *cb;

	for(i=0; i<order; i++)
	{
		k = lsp_cb[i].k;
		cb = lsp_cb[i].cb;
		lsp_hz[i] = cb[indexes[i]*k];
	}

	/* convert back to radians */

	for(i=0; i<order; i++)
		lsp[i] = (PI/4000.0)*lsp_hz[i];
}

/*---------------------------------------------------------------------------*\

  FUNCTION....: bw_expand_lsps()
  AUTHOR......: David Rowe
  DATE CREATED: 22/8/2010

  Applies Bandwidth Expansion (BW) to a vector of LSPs.  Prevents any
  two LSPs getting too close together after quantisation.  We know
  from experiment that LSP quantisation errors < 12.5Hz (25Hz step
  size) are inaudible so we use that as the minimum LSP separation.

\*---------------------------------------------------------------------------*/

void CQuantize::bw_expand_lsps(float lsp[], int order, float min_sep_low, float min_sep_high)
{
	int i;

	for(i=1; i<4; i++)
	{

		if ((lsp[i] - lsp[i-1]) < min_sep_low*(PI/4000.0))
			lsp[i] = lsp[i-1] + min_sep_low*(PI/4000.0);

	}

	/* As quantiser gaps increased, larger BW expansion was required
	   to prevent twinkly noises.  This may need more experiment for
	   different quanstisers.
	*/

	for(i=4; i<order; i++)
	{
		if (lsp[i] - lsp[i-1] < min_sep_high*(PI/4000.0))
			lsp[i] = lsp[i-1] + min_sep_high*(PI/4000.0);
	}
}

/*---------------------------------------------------------------------------*\

  FUNCTION....: apply_lpc_correction()
  AUTHOR......: David Rowe
  DATE CREATED: 22/8/2010

  Apply first harmonic LPC correction at decoder.  This helps improve
  low pitch males after LPC modelling, like hts1a and morig.

\*---------------------------------------------------------------------------*/

void CQuantize::apply_lpc_correction(MODEL *model)
{
	if (model->Wo < (PI*150.0/4000))
	{
		model->A[1] *= 0.032;
	}
}

/*---------------------------------------------------------------------------*\

  FUNCTION....: encode_energy()
  AUTHOR......: David Rowe
  DATE CREATED: 22/8/2010

  Encodes LPC energy using an E_LEVELS quantiser.

\*---------------------------------------------------------------------------*/

int CQuantize::encode_energy(float e, int bits)
{
	int   index, e_levels = 1<<bits;
	float e_min = E_MIN_DB;
	float e_max = E_MAX_DB;
	float norm;

	e = 10.0*log10f(e);
	norm = (e - e_min)/(e_max - e_min);
	index = floorf(e_levels * norm + 0.5);
	if (index < 0 ) index = 0;
	if (index > (e_levels-1)) index = e_levels-1;

	return index;
}

/*---------------------------------------------------------------------------*\

  FUNCTION....: decode_energy()
  AUTHOR......: David Rowe
  DATE CREATED: 22/8/2010

  Decodes energy using a E_LEVELS quantiser.

\*---------------------------------------------------------------------------*/

float CQuantize::decode_energy(int index, int bits)
{
	float e_min = E_MIN_DB;
	float e_max = E_MAX_DB;
	float step;
	float e;
	int   e_levels = 1<<bits;

	step = (e_max - e_min)/e_levels;
	e    = e_min + step*(index);
	e    = exp10f(e/10.0);

	return e;
}

/*---------------------------------------------------------------------------*\

  FUNCTION....: lpc_to_lsp()
  AUTHOR......: David Rowe
  DATE CREATED: 24/2/93

  This function converts LPC coefficients to LSP coefficients.

\*---------------------------------------------------------------------------*/

int CQuantize::lpc_to_lsp(float *a, int order, float *freq, int nb, float delta)
/*  float *a 		     	lpc coefficients			*/
/*  int order			order of LPC coefficients (10) 		*/
/*  float *freq 	      	LSP frequencies in radians      	*/
/*  int nb			number of sub-intervals (4) 		*/
/*  float delta			grid spacing interval (0.02) 		*/
{
	float psuml,psumr,psumm,temp_xr,xl,xr,xm = 0;
	float temp_psumr;
	int i,j,m,flag,k;
	float *px;                	/* ptrs of respective P'(z) & Q'(z)	*/
	float *qx;
	float *p;
	float *q;
	float *pt;                	/* ptr used for cheb_poly_eval()
				   whether P' or Q' 			*/
	int roots=0;              	/* number of roots found 	        */
	float Q[order + 1];
	float P[order + 1];

	flag = 1;
	m = order/2;            	/* order of P'(z) & Q'(z) polynimials 	*/

	/* Allocate memory space for polynomials */

	/* determine P'(z)'s and Q'(z)'s coefficients where
	  P'(z) = P(z)/(1 + z^(-1)) and Q'(z) = Q(z)/(1-z^(-1)) */

	px = P;                      /* initilaise ptrs */
	qx = Q;
	p = px;
	q = qx;
	*px++ = 1.0;
	*qx++ = 1.0;
	for(i=1; i<=m; i++)
	{
		*px++ = a[i]+a[order+1-i]-*p++;
		*qx++ = a[i]-a[order+1-i]+*q++;
	}
	px = P;
	qx = Q;
	for(i=0; i<m; i++)
	{
		*px = 2**px;
		*qx = 2**qx;
		px++;
		qx++;
	}
	px = P;             	/* re-initialise ptrs 			*/
	qx = Q;

	/* Search for a zero in P'(z) polynomial first and then alternate to Q'(z).
	Keep alternating between the two polynomials as each zero is found 	*/

	xr = 0;             	/* initialise xr to zero 		*/
	xl = 1.0;               	/* start at point xl = 1 		*/


	for(j=0; j<order; j++)
	{
		if(j%2)            	/* determines whether P' or Q' is eval. */
			pt = qx;
		else
			pt = px;

		psuml = cheb_poly_eva(pt,xl,order);	/* evals poly. at xl 	*/
		flag = 1;
		while(flag && (xr >= -1.0))
		{
			xr = xl - delta ;                  	/* interval spacing 	*/
			psumr = cheb_poly_eva(pt,xr,order);/* poly(xl-delta_x) 	*/
			temp_psumr = psumr;
			temp_xr = xr;

			/* if no sign change increment xr and re-evaluate
			   poly(xr). Repeat til sign change.  if a sign change has
			   occurred the interval is bisected and then checked again
			   for a sign change which determines in which interval the
			   zero lies in.  If there is no sign change between poly(xm)
			   and poly(xl) set interval between xm and xr else set
			   interval between xl and xr and repeat till root is located
			   within the specified limits  */

			if(((psumr*psuml)<0.0) || (psumr == 0.0))
			{
				roots++;

				psumm=psuml;
				for(k=0; k<=nb; k++)
				{
					xm = (xl+xr)/2;        	/* bisect the interval 	*/
					psumm=cheb_poly_eva(pt,xm,order);
					if(psumm*psuml>0.)
					{
						psuml=psumm;
						xl=xm;
					}
					else
					{
						psumr=psumm;
						xr=xm;
					}
				}

				/* once zero is found, reset initial interval to xr 	*/
				freq[j] = (xm);
				xl = xm;
				flag = 0;       		/* reset flag for next search 	*/
			}
			else
			{
				psuml=temp_psumr;
				xl=temp_xr;
			}
		}
	}

	/* convert from x domain to radians */

	for(i=0; i<order; i++)
	{
		freq[i] = acosf(freq[i]);
	}

	return(roots);
}

/*---------------------------------------------------------------------------*\

  FUNCTION....: cheb_poly_eva()
  AUTHOR......: David Rowe
  DATE CREATED: 24/2/93

  This function evalutes a series of chebyshev polynomials

  FIXME: performing memory allocation at run time is very inefficient,
  replace with stack variables of MAX_P size.

\*---------------------------------------------------------------------------*/

float CQuantize::cheb_poly_eva(float *coef,float x,int order)
/*  float coef[]  	coefficients of the polynomial to be evaluated 	*/
/*  float x   		the point where polynomial is to be evaluated 	*/
/*  int order 		order of the polynomial 			*/
{
	int i;
	float *t,*u,*v,sum;
	float T[(order / 2) + 1];

	/* Initialise pointers */

	t = T;                          	/* T[i-2] 			*/
	*t++ = 1.0;
	u = t--;                        	/* T[i-1] 			*/
	*u++ = x;
	v = u--;                        	/* T[i] 			*/

	/* Evaluate chebyshev series formulation using iterative approach 	*/

	for(i=2; i<=order/2; i++)
		*v++ = (2*x)*(*u++) - *t++;  	/* T[i] = 2*x*T[i-1] - T[i-2]	*/

	sum=0.0;                        	/* initialise sum to zero 	*/
	t = T;                          	/* reset pointer 		*/

	/* Evaluate polynomial and return value also free memory space */

	for(i=0; i<=order/2; i++)
		sum+=coef[(order/2)-i]**t++;

	return sum;
}
