/*
 *   Copyright (C) 2015 by Jonathan Naylor G4KLX
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#if !defined(BPTC19696_H)
#define	BPTC19696_H
#include <cstdint>

class CBPTC19696
{
public:
    CBPTC19696();
    ~CBPTC19696();
    
	void decode(const uint8_t* in, uint8_t* out);
    
	void encode(const uint8_t* in, uint8_t* out);
    
private:
    bool m_rawData[196];
    bool m_deInterData[196];
    
	void decodeExtractBinary(const uint8_t* in);
    void decodeErrorCheck();
    void decodeDeInterleave();
	void decodeExtractData(uint8_t* data);
    
	void encodeExtractData(const uint8_t* in);
    void encodeInterleave();
    void encodeErrorCheck();
	void encodeExtractBinary(uint8_t* data);
	void byteToBitsBE(uint8_t byte, bool* bits);
	void bitsToByteBE(bool* bits, uint8_t& byte);
};

#endif
