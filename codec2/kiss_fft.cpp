/*
Copyright (c) 2003-2010, Mark Borgerding

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the author nor the names of any contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <cstring>
#include <cassert>

#include "defines.h"
#include "kiss_fft.h"

void CKissFFT::kf_bfly2(std::complex<float> *Fout, const size_t fstride, FFT_STATE &st, int m)
{
	std::complex<float> *Fout2;
	std::complex<float> *tw1 = st.twiddles.data();
	std::complex<float> t;
	Fout2 = Fout + m;
	do
	{
		t = *Fout2 * *tw1;
		tw1 += fstride;
		*Fout2 = *Fout - t;
		*Fout += t;
		++Fout2;
		++Fout;
	}
	while (--m);
}

void CKissFFT::kf_bfly3(std::complex<float> * Fout, const size_t fstride, FFT_STATE &st, int m)
{
	const size_t m2 = 2 * m;
	std::complex<float> *tw1,*tw2;
	std::complex<float> scratch[5];
	std::complex<float> epi3;
	epi3 = st.twiddles[fstride*m];

	tw1 = tw2 = st.twiddles.data();

	do
	{
		scratch[1] = Fout[m] * *tw1;
		scratch[2] = Fout[m2] * *tw2;

		scratch[3] = scratch[1] + scratch[2];
		scratch[0] = scratch[1] - scratch[2];
		tw1 += fstride;
		tw2 += fstride*2;

		Fout[m] = *Fout - (0.5f * scratch[3]);

		scratch[0] *= epi3.imag();

		*Fout += scratch[3];

		Fout[m2].real(Fout[m].real() + scratch[0].imag());
		Fout[m2].imag(Fout[m].imag() - scratch[0].real());

		Fout[m].real(Fout[m].real() - scratch[0].imag());
		Fout[m].imag(Fout[m].imag() + scratch[0].real());

		++Fout;
	}
	while(--m);
}

void CKissFFT::kf_bfly4(std::complex<float> *Fout, const size_t fstride, FFT_STATE &st, int m)
{
	std::complex<float> *tw1,*tw2,*tw3;
	std::complex<float> scratch[6];
	int k = m;
	const int m2 = 2 * m;
	const int m3 = 3 * m;


	tw3 = tw2 = tw1 = st.twiddles.data();

	do
	{
		scratch[0] = Fout[m] * *tw1;
		scratch[1] = Fout[m2] * *tw2;
		scratch[2] = Fout[m3] * *tw3;

		scratch[5] = *Fout - scratch[1];
		*Fout += scratch[1];
		scratch[3] = scratch[0] + scratch[2];
		scratch[4] = scratch[0] - scratch[2];
		Fout[m2] = *Fout - scratch[3];
		tw1 += fstride;
		tw2 += fstride*2;
		tw3 += fstride*3;
		*Fout += scratch[3];

		if(st.inverse)
		{
			Fout[m].real(scratch[5].real() - scratch[4].imag());
			Fout[m].imag(scratch[5].imag() + scratch[4].real());
			Fout[m3].real(scratch[5].real() + scratch[4].imag());
			Fout[m3].imag(scratch[5].imag() - scratch[4].real());
		}
		else
		{
			Fout[m].real(scratch[5].real() + scratch[4].imag());
			Fout[m].imag(scratch[5].imag() - scratch[4].real());
			Fout[m3].real(scratch[5].real() - scratch[4].imag());
			Fout[m3].imag(scratch[5].imag() + scratch[4].real());
		}
		++Fout;
	}
	while(--k);
}

void CKissFFT::kf_bfly5(std::complex<float> * Fout, const size_t fstride, FFT_STATE &st, int m)
{
	std::complex<float> scratch[13];
	std::complex<float> *twiddles = st.twiddles.data();
	auto ya = twiddles[fstride*m];
	auto yb = twiddles[fstride*2*m];

	auto Fout0 = Fout;
	auto Fout1 = Fout0 + m;
	auto Fout2 = Fout0 + 2 * m;
	auto Fout3 = Fout0 + 3 * m;
	auto Fout4 = Fout0 + 4 * m;

	auto tw = st.twiddles.data();
	for (int u=0; u<m; ++u)
	{
		scratch[0] = *Fout0;

		scratch[1] = *Fout1 *  tw[u*fstride];
		scratch[2] = *Fout2 *  tw[2*u*fstride];
		scratch[3] = *Fout3 *  tw[3*u*fstride];
		scratch[4] = *Fout4 *  tw[4*u*fstride];

		 scratch[7] = scratch[1] + scratch[4];
		 scratch[10] = scratch[1] - scratch[4];
		 scratch[8] = scratch[2] + scratch[3];
		 scratch[9] = scratch[2] - scratch[3];

		*Fout0 += scratch[7] + scratch[8];

		scratch[5] = scratch[0] + (scratch[7] * ya.real()) + (scratch[8] * yb.real());

		scratch[6].real( (scratch[10].imag() * ya.imag()) + (scratch[9].imag() * yb.imag()));
		scratch[6].imag(-(scratch[10].real() * ya.imag()) - (scratch[9].real() * yb.imag()));

		*Fout1 = scratch[5] - scratch[6];
		*Fout4 = scratch[5] + scratch[6];

		scratch[11] = scratch[0] + (scratch[7] * yb.real()) + (scratch[8] * ya.real());
		scratch[12].real(-(scratch[10].imag() * yb.imag()) + (scratch[9].imag() * ya.imag()));
		scratch[12].imag( (scratch[10].real() * yb.imag()) - (scratch[9].real() * ya.imag()));

		*Fout2 = scratch[11] + scratch[12];
		*Fout3 = scratch[11] - scratch[12];

		++Fout0;
		++Fout1;
		++Fout2;
		++Fout3;
		++Fout4;
	}
}

/* perform the butterfly for one stage of a mixed radix FFT */
void CKissFFT::kf_bfly_generic(std::complex<float> *Fout, const size_t fstride, FFT_STATE &st, int m, int p)
{
	auto twiddles = st.twiddles.data();
	std::complex<float> t;
	int Norig = st.nfft;

	std::vector<std::complex<float>> scratch(p);

	for (int u=0; u<m; ++u)
	{
		int k = u;
		for (int q1=0 ; q1<p ; ++q1)
		{
			scratch[q1] = Fout[k];
			k += m;
		}

		k = u;
		for (int q1=0 ; q1<p ; ++q1)
		{
			int twidx = 0;
			Fout[k] = scratch[0];
			for (int q=1; q<p; ++q)
			{
				twidx += fstride * k;
				if (twidx >= Norig) twidx-=Norig;
				t = scratch[q] * twiddles[twidx];
				Fout[k] += t;
			}
			k += m;
		}
	}
	scratch.clear();
}

void CKissFFT::kf_work(std::complex<float> *Fout, const std::complex<float> *f, const size_t fstride, int in_stride, int *factors, FFT_STATE &st)
{
	auto Fout_beg = Fout;
	const int p = *factors++; /* the radix  */
	const int m = *factors++; /* stage's fft length/p */
	const std::complex<float> *Fout_end = Fout + p*m;

	if (m==1)
	{
		do
		{
			*Fout = *f;
			f += fstride*in_stride;
		}
		while( ++Fout != Fout_end );
	}
	else
	{
		do
		{
			// recursive call:
			// DFT of size m*p performed by doing
			// p instances of smaller DFTs of size m,
			// each one takes a decimated version of the input
			kf_work( Fout, f, fstride*p, in_stride, factors, st);
			f += fstride*in_stride;
		}
		while( (Fout += m) != Fout_end );
	}

	Fout=Fout_beg;

	// recombine the p smaller DFTs
	switch (p)
	{
	case 2:
		kf_bfly2(Fout,fstride,st,m);
		break;
	case 3:
		kf_bfly3(Fout,fstride,st,m);
		break;
	case 4:
		kf_bfly4(Fout,fstride,st,m);
		break;
	case 5:
		kf_bfly5(Fout,fstride,st,m);
		break;
	default:
		kf_bfly_generic(Fout,fstride,st,m,p);
		break;
	}
}

/*  facbuf is populated by p1,m1,p2,m2, ...
    where
    p[i] * m[i] = m[i-1]
    m0 = n                  */
void CKissFFT::kf_factor(int n,int * facbuf)
{
	int p=4;
	double floor_sqrt;
	floor_sqrt = floorf( sqrtf((double)n) );

	/*factor out powers of 4, powers of 2, then any remaining primes */
	do
	{
		while (n % p)
		{
			switch (p)
			{
			case 4:
				p = 2;
				break;
			case 2:
				p = 3;
				break;
			default:
				p += 2;
				break;
			}
			if (p > floor_sqrt)
				p = n;          /* no more factors, skip to end */
		}
		n /= p;
		*facbuf++ = p;
		*facbuf++ = n;
	}
	while (n > 1);
}

void CKissFFT::fft_alloc(FFT_STATE &state, const int nfft, bool inverse_fft)
{
	state.twiddles.resize(nfft);

	state.nfft = nfft;
	state.inverse = inverse_fft;

	for (int i=0; i<nfft; ++i)
	{
		const double pi=3.141592653589793238462643383279502884197169399375105820974944;
		double phase = -2.0 * pi * i / nfft;
		if (state.inverse)
			phase *= -1.0;
		state.twiddles[i] = std::polar(1.0f, float(phase));
	}

	kf_factor(nfft, state.factors);
}


void CKissFFT::fft_stride(FFT_STATE &st, const std::complex<float> *fin, std::complex<float> *fout, int in_stride)
{
	if (fin == fout)
	{
		//NOTE: this is not really an in-place FFT algorithm.
		//It just performs an out-of-place FFT into a temp buffer
		std::vector<std::complex<float>> tmpbuf(st.nfft);
		kf_work(tmpbuf.data(), fin, true, in_stride, st.factors, st);
		memcpy(fout, tmpbuf.data(), sizeof(std::complex<float>)*st.nfft);
		tmpbuf.clear();
	}
	else
	{
		kf_work(fout, fin, 1, in_stride, st.factors, st);
	}
}

void CKissFFT::fft(FFT_STATE &cfg, const std::complex<float> *fin, std::complex<float> *fout)
{
	fft_stride(cfg, fin, fout, 1);
}

int CKissFFT::fft_next_fast_size(int n)
{
	while(1)
	{
		int m = n;
		while ( (m % 2) == 0 ) m /= 2;
		while ( (m % 3) == 0 ) m /= 3;
		while ( (m % 5) == 0 ) m /= 5;
		if (m <= 1)
			break; /* n is completely factorable by twos, threes, and fives */
		n++;
	}
	return n;
}

void CKissFFT::fftr_alloc(FFTR_STATE &st, int nfft, const bool inverse_fft)
{
	nfft >>= 1;

	fft_alloc(st.substate, nfft, inverse_fft);
	st.tmpbuf.resize(nfft);
	st.super_twiddles.resize(nfft);

	for (int i=0; i<nfft/2; ++i)
	{
		double phase = -3.141592653589793238462643383279502884197169399375105820974944 * (double(i+1) / nfft + .5);
		if (inverse_fft)
			phase *= -1.0;
		st.super_twiddles[i] = std::polar(1.0f, float(phase));
	}
}

void CKissFFT::fftr(FFTR_STATE &st, const float *timedata, std::complex<float> *freqdata)
{
	assert(st.substate.inverse == false);

	auto ncfft = st.substate.nfft;

	/*perform the parallel fft of two real signals packed in real,imag*/
	fft( st.substate, (const std::complex<float>*)timedata, st.tmpbuf.data());
	/* The real part of the DC element of the frequency spectrum in st->tmpbuf
	 * contains the sum of the even-numbered elements of the input time sequence
	 * The imag part is the sum of the odd-numbered elements
	 *
	 * The sum of tdc.r and tdc.i is the sum of the input time sequence.
	 *      yielding DC of input time sequence
	 * The difference of tdc.r - tdc.i is the sum of the input (dot product) [1,-1,1,-1...
	 *      yielding Nyquist bin of input time sequence
	 */

	auto tdc = st.tmpbuf[0];
	freqdata[0].real(tdc.real() + tdc.imag());
	freqdata[ncfft].real(tdc.real() - tdc.imag());
	freqdata[ncfft].imag(0.f);
	freqdata[0].imag(0.f);

	for (int  k=1; k <= ncfft/2; ++k)
	{
		auto fpk = st.tmpbuf[k];
		auto fpnk = std::conj(st.tmpbuf[ncfft-k]);

		auto f1k = fpk + fpnk;
		auto f2k = fpk - fpnk;
		auto tw = f2k * st.super_twiddles[k-1];

		freqdata[k] = 0.5f * (f1k + tw);
		freqdata[ncfft-k].real(0.5f * (f1k.real() - tw.real()));
		freqdata[ncfft-k].imag(0.5f * (tw.imag() - f1k.imag()));
	}
}

void CKissFFT::fftri(FFTR_STATE &st, const std::complex<float> *freqdata, float *timedata)
{
	assert(st.substate.inverse == true);

	auto ncfft = st.substate.nfft;

	st.tmpbuf[0].real(freqdata[0].real() + freqdata[ncfft].real());
	st.tmpbuf[0].imag(freqdata[0].real() - freqdata[ncfft].real());

	for (int k=1; k <= ncfft/2; ++k)
	{
		auto fk = freqdata[k];
		auto fnkc = std::conj(freqdata[ncfft - k]);

		auto fek = fk + fnkc;
		auto tmp = fk - fnkc;
		auto fok = tmp * st.super_twiddles[k-1];
		st.tmpbuf[k] = fek + fok;
		st.tmpbuf[ncfft - k] = std::conj(fek - fok);
	}
	fft (st.substate, st.tmpbuf.data(), (std::complex<float> *)timedata);
}
