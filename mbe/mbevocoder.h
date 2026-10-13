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
 
#ifndef MBEVOCODER_H
#define MBEVOCODER_H

#include <cinttypes>
#include <imbe_vocoder_api.h>

extern "C" {
struct mbe_parameters
{
  float w0;
  int L;
  int K;
  int Vl[57];
  float Ml[57];
  float log2Ml[57];
  float PHIl[57];
  float PSIl[57];
  float gamma;
  int un;
  int repeat;
};

typedef struct mbe_parameters mbe_parms;

struct mbelibParms
{
    mbe_parms *m_cur_mp;
    mbe_parms *m_prev_mp;
    mbe_parms *m_prev_mp_enhanced;

    mbelibParms()
    {
        m_cur_mp = (mbe_parms *) malloc(sizeof(mbe_parms));
        m_prev_mp = (mbe_parms *) malloc(sizeof(mbe_parms));
        m_prev_mp_enhanced = (mbe_parms *) malloc(sizeof(mbe_parms));
    }

    ~mbelibParms()
    {
        free(m_prev_mp_enhanced);
        free(m_prev_mp);
        free(m_cur_mp);
    }
};

void mbe_checkGolayBlock (long int *block);
int mbe_golay2312 (char *in, char *out);
void encode_ambe(const IMBE_PARAM *, int b[], mbe_parms*cur_mp, mbe_parms*prev_mp, bool dstar, float gain_adjust, float *gain_state = nullptr);

/*
 * Prototypes from ambe3600x2400.cpp
 */
int mbe_eccAmbe3600x2400C0 (char ambe_fr[4][24]);
int mbe_eccAmbe3600x2400Data (char ambe_fr[4][24], char *ambe_d);
int mbe_decodeAmbe2400Parms (char *ambe_d, mbe_parms * cur_mp, mbe_parms * prev_mp);
void mbe_demodulateAmbe3600x2400Data (char ambe_fr[4][24]);
void mbe_processAmbe2400Dataf (float *aout_buf, int *errs2, char *err_str, char ambe_d[49], mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced, int uvquality);
void mbe_processAmbe2400Data (short *aout_buf, int *errs2, char *err_str, char ambe_d[49], mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced, int uvquality);
void mbe_processAmbe3600x2400Framef (float *aout_buf, int *errs2, char *err_str, char ambe_fr[4][24], char ambe_d[49], mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced, int uvquality);
void mbe_processAmbe3600x2400Frame (short *aout_buf, int *errs2, char *err_str, char ambe_fr[4][24], char ambe_d[49], mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced, int uvquality);

/*
 * Prototypes from ambe3600x2450.cpp
 */
int mbe_eccAmbe3600x2450C0 (char ambe_fr[4][24]);
int mbe_eccAmbe3600x2450Data (char ambe_fr[4][24], char *ambe_d);
int mbe_decodeAmbe2450Parms (char *ambe_d, mbe_parms * cur_mp, mbe_parms * prev_mp);
void mbe_demodulateAmbe3600x2450Data (char ambe_fr[4][24]);
void mbe_processAmbe2450Dataf (float *aout_buf, int *errs2, char *err_str, char ambe_d[49], mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced, int uvquality);
void mbe_processAmbe2450Data (short *aout_buf, int *errs2, char *err_str, char ambe_d[49], mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced, int uvquality);
void mbe_processAmbe3600x2450Framef (float *aout_buf, int *errs2, char *err_str, char ambe_fr[4][24], char ambe_d[49], mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced, int uvquality);
void mbe_processAmbe3600x2450Frame (short *aout_buf, int *errs2, char *err_str, char ambe_fr[4][24], char ambe_d[49], mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced, int uvquality);

/*
 * Prototypes from mbelib.c
 */
void mbe_printVersion (char *str);
void mbe_moveMbeParms (mbe_parms * cur_mp, mbe_parms * prev_mp);
void mbe_useLastMbeParms (mbe_parms * cur_mp, mbe_parms * prev_mp);
void mbe_initMbeParms (mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced);
void mbe_spectralAmpEnhance (mbe_parms * cur_mp);
void mbe_synthesizeSilencef (float *aout_buf);
void mbe_synthesizeSilence (short *aout_buf);
void mbe_synthesizeSpeechf (float *aout_buf, mbe_parms * cur_mp, mbe_parms * prev_mp, int uvquality);
void mbe_synthesizeSpeech (short *aout_buf, mbe_parms * cur_mp, mbe_parms * prev_mp, int uvquality);
void mbe_floattoshort (float *float_buf, short *aout_buf);
}

class MBEVocoder
{
public:
    MBEVocoder();
    ~MBEVocoder();
	void decode_2400x1200(int16_t *pcm, uint8_t *codec);
	void decode_2450x1150(int16_t *pcm, uint8_t *codec);
	void decode_2450(int16_t *pcm, uint8_t *codec);
	void encode_2400x1200(int16_t *pcm, uint8_t *codec);
	void encode_2450x1150(int16_t *pcm, uint8_t *codec);
	void encode_2450(int16_t *pcm, uint8_t *codec);
	
private:
	imbe_vocoder vocoder;
	mbelibParms *m_mbelibParms;
    int m_errs2;
    char m_err_str[64];

    float m_audio_out_temp_buf[160];   //!< output of decoder
    float *m_audio_out_temp_buf_p;
    //float m_aout_max_buf[200];
    //float *m_aout_max_buf_p;
    //int m_aout_max_buf_idx;
    short m_audio_out_buf[2*48000];    //!< final result - 1s of L+R S16LE samples
    short *m_audio_out_buf_p;
    int   m_audio_out_nb_samples;
    int   m_audio_out_buf_size;
	const int *w, *x, *y, *z;
	char ambe_d[49];
	float m_dstar_gain_s;
	
	void initMbeParms();
	void process_2400x1200(unsigned char *d);
	void process_2450x1150(unsigned char *d);
	void process_2450(unsigned char *d);
	void processData(char ambe_data[49]);
	short *getAudio(int& nbSamples);
	void resetAudio();
	void processAudio();
};
#endif // MBEVOCODER_H
