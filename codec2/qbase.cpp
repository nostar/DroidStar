#include <assert.h>
#include <math.h>

#include "qbase.h"

/*---------------------------------------------------------------------------*\

  quantise

  Quantises vec by choosing the nearest vector in codebook cb, and
  returns the vector index.  The squared error of the quantised vector
  is added to se.

\*---------------------------------------------------------------------------*/

long CQbase::quantise(const float *cb, float vec[], float w[], int k, int m, float *se)
/* float   cb[][K];	current VQ codebook       */
/* float   vec[];	vector to quantise        */
/* float   w[];     weighting vector          */
/* int	   k;		dimension of vectors      */
/* int     m;		size of codebook          */
/* float   *se;		accumulated squared error */
{
	float   e;			/* current error		*/
	long	   besti;	/* best index so far	*/
	float   beste;		/* best error so far	*/
	long	   j;
	int     i;
	float   diff;

	besti = 0;
	beste = 1E32;
	for(j=0; j<m; j++)
	{
		e = 0.0;
		for(i=0; i<k; i++)
		{
			diff = cb[j*k+i]-vec[i];
			e += (diff*w[i] * diff*w[i]);
		}
		if (e < beste)
		{
			beste = e;
			besti = j;
		}
	}

	*se += beste;

	return(besti);
}

/*---------------------------------------------------------------------------*\

  FUNCTION....: encode_WoE()
  AUTHOR......: Jean-Marc Valin & David Rowe
  DATE CREATED: 11 May 2012

  Joint Wo and LPC energy vector quantiser developed my Jean-Marc
  Valin.  Returns index, and updated states xq[].

\*---------------------------------------------------------------------------*/

int CQbase::encode_WoE(MODEL *model, float e, float xq[])
{
	int          i, n1;
	float        x[2];
	float        err[2];
	float        w[2];
	const float *codebook1 = ge_cb[0].cb;
	int          nb_entries = ge_cb[0].m;
	int          ndim = ge_cb[0].k;

	assert((1<<WO_E_BITS) == nb_entries);

	if (e < 0.0) e = 0;  /* occasional small negative energies due LPC round off I guess */

	x[0] = log10f((model->Wo/PI)*4000.0/50.0)/log10f(2);
	x[1] = 10.0*log10f(1e-4 + e);

	compute_weights2(x, xq, w);
	for (i=0; i<ndim; i++)
		err[i] = x[i]-ge_coeff[i]*xq[i];
	n1 = find_nearest_weighted(codebook1, nb_entries, err, w, ndim);

	for (i=0; i<ndim; i++)
	{
		xq[i] = ge_coeff[i]*xq[i] + codebook1[ndim*n1+i];
		err[i] -= codebook1[ndim*n1+i];
	}

	//printf("enc: %f %f (%f)(%f) \n", xq[0], xq[1], e, 10.0*log10(1e-4 + e));
	return n1;
}


/*---------------------------------------------------------------------------*\

  FUNCTION....: decode_WoE()
  AUTHOR......: Jean-Marc Valin & David Rowe
  DATE CREATED: 11 May 2012

  Joint Wo and LPC energy vector quantiser developed my Jean-Marc
  Valin.  Given index and states xq[], returns Wo & E, and updates
  states xq[].

\*---------------------------------------------------------------------------*/

void CQbase::decode_WoE(C2CONST *c2const, MODEL *model, float *e, float xq[], int n1)
{
	int          i;
	const float *codebook1 = ge_cb[0].cb;
	int          ndim = ge_cb[0].k;
	float Wo_min = c2const->Wo_min;
	float Wo_max = c2const->Wo_max;

	for (i=0; i<ndim; i++)
	{
		xq[i] = ge_coeff[i]*xq[i] + codebook1[ndim*n1+i];
	}

	//printf("dec: %f %f\n", xq[0], xq[1]);
	model->Wo = powf(2.0, xq[0])*(PI*50.0)/4000.0;

	/* bit errors can make us go out of range leading to all sorts of
	   probs like seg faults */

	if (model->Wo > Wo_max) model->Wo = Wo_max;
	if (model->Wo < Wo_min) model->Wo = Wo_min;

	model->L  = PI/model->Wo; /* if we quantise Wo re-compute L */

	*e = exp10f(xq[1]/10.0);
}

void CQbase::compute_weights2(const float *x, const float *xp, float *w)
{
	w[0] = 30;
	w[1] = 1;
	if (x[1]<0)
	{
		w[0] *= .6;
		w[1] *= .3;
	}
	if (x[1]<-10)
	{
		w[0] *= .3;
		w[1] *= .3;
	}
	/* Higher weight if pitch is stable */
	if (fabsf(x[0]-xp[0])<.2)
	{
		w[0] *= 2;
		w[1] *= 1.5;
	}
	else if (fabsf(x[0]-xp[0])>.5)   /* Lower if not stable */
	{
		w[0] *= .5;
	}

	/* Lower weight for low energy */
	if (x[1] < xp[1]-10)
	{
		w[1] *= .5;
	}
	if (x[1] < xp[1]-20)
	{
		w[1] *= .5;
	}

	//w[0] = 30;
	//w[1] = 1;

	/* Square the weights because it's applied on the squared error */
	w[0] *= w[0];
	w[1] *= w[1];

}

int CQbase::find_nearest_weighted(const float *codebook, int nb_entries, float *x, const float *w, int ndim)
{
	int i, j;
	float min_dist = 1e15;
	int nearest = 0;

	for (i=0; i<nb_entries; i++)
	{
		float dist=0;
		for (j=0; j<ndim; j++)
			dist += w[j]*(x[j]-codebook[i*ndim+j])*(x[j]-codebook[i*ndim+j]);
		if (dist<min_dist)
		{
			min_dist = dist;
			nearest = i;
		}
	}
	return nearest;
}

/*---------------------------------------------------------------------------*\

  FUNCTION....: encode_log_Wo()
  AUTHOR......: David Rowe
  DATE CREATED: 22/8/2010

  Encodes Wo in the log domain using a WO_LEVELS quantiser.

\*---------------------------------------------------------------------------*/

int CQbase::encode_log_Wo(C2CONST *c2const, float Wo, int bits)
{
	int   index, Wo_levels = 1<<bits;
	float Wo_min = c2const->Wo_min;
	float Wo_max = c2const->Wo_max;
	float norm;

	norm = (log10f(Wo) - log10f(Wo_min))/(log10f(Wo_max) - log10f(Wo_min));
	index = floorf(Wo_levels * norm + 0.5);
	if (index < 0 ) index = 0;
	if (index > (Wo_levels-1)) index = Wo_levels-1;

	return index;
}

/*---------------------------------------------------------------------------*\

  FUNCTION....: decode_log_Wo()
  AUTHOR......: David Rowe
  DATE CREATED: 22/8/2010

  Decodes Wo using a WO_LEVELS quantiser in the log domain.

\*---------------------------------------------------------------------------*/

float CQbase::decode_log_Wo(C2CONST *c2const, int index, int bits)
{
	float Wo_min = c2const->Wo_min;
	float Wo_max = c2const->Wo_max;
	float step;
	float Wo;
	int   Wo_levels = 1<<bits;

	step = (log10f(Wo_max) - log10f(Wo_min))/Wo_levels;
	Wo   = log10f(Wo_min) + step*(index);

	return exp10f(Wo);
}
