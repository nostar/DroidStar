/*
	Copyright (C) 2019-2021 Doug McLain

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef VOCODER_PLUGIN_H
#define VOCODER_PLUGIN_H

#include <cinttypes>

class Vocoder
{
public:
	Vocoder() {}
	virtual ~Vocoder() {}
	virtual void decode_2400x1200(int16_t *pcm, uint8_t *codec) = 0;	// Provide pointer to 72 bits as codec, returns 160 S_16_BE PCM frames as pcm
	virtual void decode_2450x1150(int16_t *pcm, uint8_t *codec) = 0;	// Provide pointer to 72 bits as codec, returns 160 S_16_BE PCM frames as pcm
	virtual void decode_2450(int16_t *pcm, uint8_t *codec) = 0;			// Provide pointer to 49 bits as codec, returns 160 S_16_BE PCM frames as pcm
	virtual void encode_2400x1200(int16_t *pcm, uint8_t *codec) = 0;	// Provide pointer to 160 S_16_BE PCM frames as pcm, returns pointer to 72 bits as codec
	virtual void encode_2450x1150(int16_t *pcm, uint8_t *codec) = 0;	// Provide pointer to 160 S_16_BE PCM frames as pcm, returns pointer to 72 bits as codec
	virtual void encode_2450(int16_t *pcm, uint8_t *codec) = 0;			// Provide pointer to 160 S_16_BE PCM frames as pcm, returns pointer to 49 bits as codec
};

typedef Vocoder* create_t();
typedef void destry_t(Vocoder *);

#endif // VOCODER_PLUGIN_H
