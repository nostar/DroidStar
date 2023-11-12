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
 
#ifndef VOCODER_PLUGIN_H
#define VOCODER_PLUGIN_H

#include <cinttypes>
#include "imbe_vocoder/imbe_vocoder_api.h"
#include "mbelib_parms.h"

class VocoderPlugin
{
public:
	VocoderPlugin();
	~VocoderPlugin();
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
	
	void initMbeParms();
	void process_2400x1200(unsigned char *d);
	void process_2450x1150(unsigned char *d);
	void process_2450(unsigned char *d);
	void processData(char ambe_data[49]);
	short *getAudio(int& nbSamples);
	void resetAudio();
	void processAudio();
};

#endif // VOCODER_PLUGIN_H
