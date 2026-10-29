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
#define _USE_MATH_DEFINES
#include <cstring>
#include <cmath>

#include "mbevocoder.h"
#include "vocoder_tables.h"

static inline uint32_t golay_24_encode(uint32_t code_word_in)
{
   static const uint32_t encoding[12] = {
      040006165,
      020003073,
      010007550,
      04003664,
      02001732,
      01006631,
      0403315,
      0201547,
      0106706,
      045227,
      024476,
      014353
   };

	uint32_t code_word_out = 0;
	for(uint16_t i = 0; i < 12; i++) {
		uint32_t temp_word = code_word_in & (1 << (11 - i));
		if(temp_word >= 1) {
			code_word_out = code_word_out ^ encoding[i];
		}
	}
	return(code_word_out);
}

static inline uint32_t golay_23_encode(uint32_t code_word_in)
{
	return golay_24_encode(code_word_in) >> 1;
}

static inline void store_reg(int reg, uint8_t val[], int len)
{
	for (int i=0; i<len; i++){
		val[i] = (reg >> (len-1-i)) & 1;
	}
}

static inline int load_reg(const uint8_t val[], int len)
{
	int acc = 0;
	for (int i=0; i<len; i++){
		acc = (acc << 1) + (val[i] & 1);
	}
	return acc;
}

    MBEVocoder::MBEVocoder()
	{
		m_mbelibParms = new mbelibParms();
		m_audio_out_temp_buf_p = m_audio_out_temp_buf;

		memset(m_audio_out_buf, 0, sizeof(short) * 2 * 48000);
		m_audio_out_buf_p = m_audio_out_buf;
		m_audio_out_nb_samples = 0;
		m_audio_out_buf_size = 48000; // given in number of unique samples

		initMbeParms();
		memset(ambe_d, 0, 49);
		m_dstar_gain_s = -99.0f;
        d = ambe2_decoder_new();
        e = ambe2_encoder_new();
	}
	
    MBEVocoder::~MBEVocoder()
	{
		delete m_mbelibParms;
	}
	
    void MBEVocoder::decode_2400x1200(int16_t *pcm, uint8_t *ambe)
	{
		int samples = 0;
		process_2400x1200(ambe);
		int16_t *p = getAudio(samples);
		memcpy(pcm, p, samples * sizeof(int16_t));
		resetAudio();
	}

    void MBEVocoder::decode_2450x1150(int16_t *pcm, uint8_t *ambe)
	{
        ambe2_decode_packed(d, ambe, pcm);
        return;
		int samples = 0;
		process_2450x1150(ambe);
		int16_t *p = getAudio(samples);
		memcpy(pcm, p, samples * sizeof(int16_t));
		resetAudio();
	}

    void MBEVocoder::decode_2450(int16_t *pcm, uint8_t *ambe)
	{
        uint8_t ambe_data[56];
        //char dvsi_data[7];
        //memset(dvsi_data, 0, 7);

        for(int i = 0; i < 6; ++i){
            for(int j = 0; j < 8; j++){
                ambe_data[j+(8*i)] = (1 & (ambe[i] >> (7 - j)));
            }
        }

        ambe_data[48] = (1 & (ambe[6] >> 7));
        //int samples = 0;
        ambe2_decode(d, ambe_data, pcm);
        //process_2450(ambe);
        //int16_t *p = getAudio(samples);
        //memcpy(pcm, p, samples * sizeof(int16_t));
        //resetAudio();
	}
	
    void MBEVocoder::encode_2400x1200(int16_t *pcm, uint8_t *ambe)
	{
		int b[9];
		int16_t frame_vector[8];	// result ignored
		uint8_t ambe_frame[72];
		
		uint8_t pbuf[48];
		uint8_t tbuf[48];
		int tbufp = 0;
		memset(ambe, 0, 9);
		
		vocoder.imbe_encode(frame_vector, pcm);
		encode_ambe(vocoder.param(), b, m_mbelibParms->m_cur_mp, m_mbelibParms->m_prev_mp, true, DSTAR_GAIN_ADJUST, &m_dstar_gain_s);
		b[8] >>= 1;

		for (int i=0; i < 9; i++) {
			store_reg(b[i], &tbuf[tbufp], b_lengths[i]);
			tbufp += b_lengths[i];
		}
		
		for (int i=0; i < 48; i++)
			pbuf[i] = tbuf[m_list[i]];
			
		int u0 = load_reg(pbuf+0, 12);
		int u1 = load_reg(pbuf+12, 12);

		int m1 = PRNG_TABLE[u0];
		int c0 = golay_24_encode(u0);
		int c1 = golay_24_encode(u1) ^ m1;

		uint8_t pre_buf[72];
		store_reg(c0, pre_buf, 24);
		store_reg(c1, pre_buf+24, 24);
		memcpy(pre_buf+48, pbuf+24, 24);

		for (int i=0; i < 72; i++)
			ambe_frame[d_list[i]] = pre_buf[i];

		for(int i = 0; i < 9; ++i){
			for(int j = 0; j < 8; ++j){
				ambe[i] |= (ambe_frame[(i*8)+j] << j);
			}
		}
	}
	
    void MBEVocoder::encode_2450x1150(int16_t *pcm, uint8_t *ambe)
	{
		unsigned int aOrig = 0U;
		unsigned int bOrig = 0U;
		unsigned int cOrig = 0U;
		unsigned int MASK = 0x000800U;
		uint8_t tmp[9];
		
		memset(tmp, 0, 9);
		memset(ambe, 0, 9);
		encode_2450(pcm, tmp);
		
		for (unsigned int i = 0U; i < 12U; i++, MASK >>= 1) {
			unsigned int n1 = i;
			unsigned int n2 = i + 12U;
			if (READ_BIT(tmp, n1))
				aOrig |= MASK;
			if (READ_BIT(tmp, n2))
				bOrig |= MASK;
		}

		MASK = 0x1000000U;
		for (unsigned int i = 0U; i < 25U; i++, MASK >>= 1) {
			unsigned int n = i + 24U;
			if (READ_BIT(tmp, n))
				cOrig |= MASK;
		}

		unsigned int a = golay_24_encode(aOrig);

		// The PRNG
		unsigned int p = PRNG_TABLE[aOrig] >> 1;

		unsigned int b = golay_23_encode(bOrig);
		b ^= p;

		MASK = 0x800000U;
		for (unsigned int i = 0U; i < 24U; i++, MASK >>= 1) {
			unsigned int aPos = A_TABLE[i];
			WRITE_BIT(ambe, aPos, a & MASK);
		}

		MASK = 0x400000U;
		for (unsigned int i = 0U; i < 23U; i++, MASK >>= 1) {
			unsigned int bPos = B_TABLE[i];
			WRITE_BIT(ambe, bPos, b & MASK);
		}

		MASK = 0x1000000U;
		for (unsigned int i = 0U; i < 25U; i++, MASK >>= 1) {
			unsigned int cPos = C_TABLE[i];
			WRITE_BIT(ambe, cPos, cOrig & MASK);
		}
	}
	
    void MBEVocoder::encode_2450(int16_t *pcm, uint8_t *ambe)
	{
        //int b[9];
        //int16_t frame_vector[8];	// result ignored
        uint8_t ambe_bits[56];
        memset(ambe, 0, 9);

        ambe2_encode(e, pcm, ambe_bits);
        //ambe2_pack(ambe_bits, ambe);
        //return;
        /*
        vocoder.imbe_encode(frame_vector, pcm);
        encode_ambe(vocoder.param(), b, m_mbelibParms->m_cur_mp, m_mbelibParms->m_prev_mp, false, 1.0);
        memset(ambe_frame, 0, 56);

        ambe_frame[0] = (b[0] >> 6) & 1;
        ambe_frame[1] = (b[0] >> 5) & 1;
        ambe_frame[2] = (b[0] >> 4) & 1;
        ambe_frame[3] = (b[0] >> 3) & 1;
        ambe_frame[4] = (b[1] >> 4) & 1;
        ambe_frame[5] = (b[1] >> 3) & 1;
        ambe_frame[6] = (b[1] >> 2) & 1;
        ambe_frame[7] = (b[1] >> 1) & 1;
        ambe_frame[8] = (b[2] >> 4) & 1;
        ambe_frame[9] = (b[2] >> 3) & 1;
        ambe_frame[10] = (b[2] >> 2) & 1;
        ambe_frame[11] = (b[2] >> 1) & 1;
        ambe_frame[12] = (b[3] >> 8) & 1;
        ambe_frame[13] = (b[3] >> 7) & 1;
        ambe_frame[14] = (b[3] >> 6) & 1;
        ambe_frame[15] = (b[3] >> 5) & 1;
        ambe_frame[16] = (b[3] >> 4) & 1;
        ambe_frame[17] = (b[3] >> 3) & 1;
        ambe_frame[18] = (b[3] >> 2) & 1;
        ambe_frame[19] = (b[3] >> 1) & 1;
        ambe_frame[20] = (b[4] >> 6) & 1;
        ambe_frame[21] = (b[4] >> 5) & 1;
        ambe_frame[22] = (b[4] >> 4) & 1;
        ambe_frame[23] = (b[4] >> 3) & 1;
        ambe_frame[24] = (b[5] >> 4) & 1;
        ambe_frame[25] = (b[5] >> 3) & 1;
        ambe_frame[26] = (b[5] >> 2) & 1;
        ambe_frame[27] = (b[5] >> 1) & 1;
        ambe_frame[28] = (b[6] >> 3) & 1;
        ambe_frame[29] = (b[6] >> 2) & 1;
        ambe_frame[30] = (b[6] >> 1) & 1;
        ambe_frame[31] = (b[7] >> 3) & 1;
        ambe_frame[32] = (b[7] >> 2) & 1;
        ambe_frame[33] = (b[7] >> 1) & 1;
        ambe_frame[34] = (b[8] >> 2) & 1;
        ambe_frame[35] = b[1] & 1;
        ambe_frame[36] = b[2] & 1;
        ambe_frame[37] = (b[0] >> 2) & 1;
        ambe_frame[38] = (b[0] >> 1) & 1;
        ambe_frame[39] = b[0] & 1;
        ambe_frame[40] = b[3] & 1;
        ambe_frame[41] = (b[4] >> 2) & 1;
        ambe_frame[42] = (b[4] >> 1) & 1;
        ambe_frame[43] = b[4] & 1;
        ambe_frame[44] = b[5] & 1;
        ambe_frame[45] = b[6] & 1;
        ambe_frame[46] = b[7] & 1;
        ambe_frame[47] = (b[8] >> 1) & 1;
        ambe_frame[48] = b[8] & 1;
*/
        for(int i = 0; i < 7; ++i){
            for(int j = 0; j < 8; ++j){
                ambe[i] |= (ambe_bits[(i*8)+j] << (7-j));
            }
        }
	}

    void MBEVocoder::initMbeParms()
	{
		mbe_initMbeParms(m_mbelibParms->m_cur_mp, m_mbelibParms->m_prev_mp, m_mbelibParms->m_prev_mp_enhanced);
		//m_errs = 0;
		m_errs2 = 0;
		m_err_str[0] = 0;
	}
	
    void MBEVocoder::process_2400x1200(unsigned char *d)
	{
		char ambe_fr[4][24];
    
		memset(ambe_fr, 0, 96);
		w = dW;
		x = dX;
	
		for(int i = 0; i < 9; ++i){
			for(int j = 0; j < 8; ++j){
				ambe_fr[*w][*x] = (1 & (d[i] >> j));
				w++;
				x++;
			}
		}

		mbe_processAmbe3600x2400Framef(m_audio_out_temp_buf, &m_errs2, m_err_str, ambe_fr, ambe_d,m_mbelibParms-> m_cur_mp, m_mbelibParms->m_prev_mp, m_mbelibParms->m_prev_mp_enhanced, 3);
		static const float kOut = powf(10.0f, DSTAR_OUT_GAIN_DB / 20.0f);
		for (int i = 0; i < 160; i++) m_audio_out_temp_buf[i] *= kOut;
		processAudio();
	}

    void MBEVocoder::process_2450x1150(unsigned char *d)
	{
		char ambe_fr[4][24];

		memset(ambe_fr, 0, 96);
		w = rW;
		x = rX;
		y = rY;
		z = rZ;

		for(int i = 0; i < 9; ++i){
			for(int j = 0; j < 8; j+=2){
				ambe_fr[*y][*z] = (1 & (d[i] >> (7 - (j+1))));
				ambe_fr[*w][*x] = (1 & (d[i] >> (7 - j)));
				w++;
				x++;
				y++;
				z++;
			}
		}

		mbe_processAmbe3600x2450Framef(m_audio_out_temp_buf, &m_errs2, m_err_str, ambe_fr, ambe_d,m_mbelibParms-> m_cur_mp, m_mbelibParms->m_prev_mp, m_mbelibParms->m_prev_mp_enhanced, 3);
		processAudio();
	}

    void MBEVocoder::process_2450(unsigned char *d)
	{
		char ambe_data[49];
		char dvsi_data[7];
		memset(dvsi_data, 0, 7);

		for(int i = 0; i < 6; ++i){
			for(int j = 0; j < 8; j++){
				ambe_data[j+(8*i)] = (1 & (d[i] >> (7 - j)));
			}
		}
		ambe_data[48] = (1 & (d[6] >> 7));
		processData(ambe_data);
	}
	
    void MBEVocoder::processData(char ambe_data[49])
	{
		mbe_processAmbe2450Dataf(m_audio_out_temp_buf, &m_errs2, m_err_str, ambe_data, m_mbelibParms->m_cur_mp,m_mbelibParms->m_prev_mp, m_mbelibParms->m_prev_mp_enhanced, 3);
		processAudio();
	}
	
    short * MBEVocoder::getAudio(int& nbSamples)
	{
		nbSamples = m_audio_out_nb_samples;
		return m_audio_out_buf;
	}

    void MBEVocoder::resetAudio()
	{
		m_audio_out_nb_samples = 0;
		m_audio_out_buf_p = m_audio_out_buf;
	}
	
    void MBEVocoder::processAudio()
	{
		m_audio_out_temp_buf_p = m_audio_out_temp_buf;

		if (m_audio_out_nb_samples + 160 >= m_audio_out_buf_size){
            resetAudio();
		}

		for (int i = 0; i < 160; i++){
			if (*m_audio_out_temp_buf_p > static_cast<float>(32760)){
				*m_audio_out_temp_buf_p = static_cast<float>(32760);
			}
			else if (*m_audio_out_temp_buf_p < static_cast<float>(-32760)){
				*m_audio_out_temp_buf_p = static_cast<float>(-32760);
			}

			*m_audio_out_buf_p = static_cast<short>(*m_audio_out_temp_buf_p);
			m_audio_out_buf_p++;

			m_audio_out_nb_samples++;
			m_audio_out_temp_buf_p++;
		}
	}
