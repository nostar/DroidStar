#ifndef KISS_FFT_H
#define KISS_FFT_H

#include <complex>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "defines.h"

/* for real ffts, we need an even size */
#define kiss_fftr_next_fast_size_real(n) (kiss_fft_next_fast_size( ((n)+1) >> 1) << 1 )

class CKissFFT
{
public:
	void fft_alloc(FFT_STATE &state, const int nfft, const bool inverse_fft);
	void fft(FFT_STATE &cfg, const std::complex<float> *fin, std::complex<float> *fout);
	void fft_stride(FFT_STATE &cfg, const std::complex<float> *fin, std::complex<float> *fout, int fin_stride);
	int fft_next_fast_size(int n);
	void fftr_alloc(FFTR_STATE &state, int nfft, const bool inverse_fft);
	void fftr(FFTR_STATE &cfg,const float *timedata,std::complex<float> *freqdata);
	void fftri(FFTR_STATE &cfg,const std::complex<float> *freqdata,float *timedata);
private:
	void kf_bfly2(std::complex<float> *Fout, const size_t fstride, FFT_STATE &st, int m);
	void kf_bfly3(std::complex<float> *Fout, const size_t fstride, FFT_STATE &st, int m);
	void kf_bfly4(std::complex<float> *Fout, const size_t fstride, FFT_STATE &st, int m);
	void kf_bfly5(std::complex<float> *Fout, const size_t fstride, FFT_STATE &st, int m);
	void kf_bfly_generic(std::complex<float> *Fout, const size_t fstride, FFT_STATE &st, int m, int p);
	void kf_work(std::complex<float> *Fout, const std::complex<float> *f, const size_t fstride, int in_stride, int *factors, FFT_STATE &st);
	void kf_factor(int n, int *facbuf);
};
#endif
