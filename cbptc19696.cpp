/*
 *	 Copyright (C) 2012 by Ian Wraith
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

#include "cbptc19696.h"

#include "chamming.h"
//#include "cutils.h"

#include <cstdio>
#include <cassert>
#include <cstring>

CBPTC19696::CBPTC19696()
{
}

CBPTC19696::~CBPTC19696()
{
}

// The main decode function
void CBPTC19696::decode(const uint8_t* in, uint8_t* out)
{
    assert(in != NULL);
    assert(out != NULL);
    
    //  Get the raw binary
    decodeExtractBinary(in);
    
    // Deinterleave
    decodeDeInterleave();
    
    // Error check
    decodeErrorCheck();
    
    // Extract Data
    decodeExtractData(out);
}

// The main encode function
void CBPTC19696::encode(const uint8_t* in, uint8_t* out)
{
    assert(in != NULL);
    assert(out != NULL);
    
    // Extract Data
    encodeExtractData(in);
    
    // Error check
    encodeErrorCheck();
    
    // Deinterleave
    encodeInterleave();
    
    //  Get the raw binary
    encodeExtractBinary(out);
}

void CBPTC19696::byteToBitsBE(uint8_t byte, bool* bits)
{
	assert(bits != NULL);

	bits[0U] = (byte & 0x80U) == 0x80U;
	bits[1U] = (byte & 0x40U) == 0x40U;
	bits[2U] = (byte & 0x20U) == 0x20U;
	bits[3U] = (byte & 0x10U) == 0x10U;
	bits[4U] = (byte & 0x08U) == 0x08U;
	bits[5U] = (byte & 0x04U) == 0x04U;
	bits[6U] = (byte & 0x02U) == 0x02U;
	bits[7U] = (byte & 0x01U) == 0x01U;
}

void CBPTC19696::bitsToByteBE(bool* bits, uint8_t& byte)
{
	assert(bits != NULL);

	byte  = bits[0U] ? 0x80U : 0x00U;
	byte |= bits[1U] ? 0x40U : 0x00U;
	byte |= bits[2U] ? 0x20U : 0x00U;
	byte |= bits[3U] ? 0x10U : 0x00U;
	byte |= bits[4U] ? 0x08U : 0x00U;
	byte |= bits[5U] ? 0x04U : 0x00U;
	byte |= bits[6U] ? 0x02U : 0x00U;
	byte |= bits[7U] ? 0x01U : 0x00U;
}

void CBPTC19696::decodeExtractBinary(const uint8_t* in)
{
    // First block
	byteToBitsBE(in[0U],  m_rawData + 0U);
	byteToBitsBE(in[1U],  m_rawData + 8U);
	byteToBitsBE(in[2U],  m_rawData + 16U);
	byteToBitsBE(in[3U],  m_rawData + 24U);
	byteToBitsBE(in[4U],  m_rawData + 32U);
	byteToBitsBE(in[5U],  m_rawData + 40U);
	byteToBitsBE(in[6U],  m_rawData + 48U);
	byteToBitsBE(in[7U],  m_rawData + 56U);
	byteToBitsBE(in[8U],  m_rawData + 64U);
	byteToBitsBE(in[9U],  m_rawData + 72U);
	byteToBitsBE(in[10U], m_rawData + 80U);
	byteToBitsBE(in[11U], m_rawData + 88U);
	byteToBitsBE(in[12U], m_rawData + 96U);
    
    // Handle the two bits
    bool bits[8U];
	byteToBitsBE(in[20U], bits);
    m_rawData[98U] = bits[6U];
    m_rawData[99U] = bits[7U];
    
    // Second block
	byteToBitsBE(in[21U], m_rawData + 100U);
	byteToBitsBE(in[22U], m_rawData + 108U);
	byteToBitsBE(in[23U], m_rawData + 116U);
	byteToBitsBE(in[24U], m_rawData + 124U);
	byteToBitsBE(in[25U], m_rawData + 132U);
	byteToBitsBE(in[26U], m_rawData + 140U);
	byteToBitsBE(in[27U], m_rawData + 148U);
	byteToBitsBE(in[28U], m_rawData + 156U);
	byteToBitsBE(in[29U], m_rawData + 164U);
	byteToBitsBE(in[30U], m_rawData + 172U);
	byteToBitsBE(in[31U], m_rawData + 180U);
	byteToBitsBE(in[32U], m_rawData + 188U);
}

// Deinterleave the raw data
void CBPTC19696::decodeDeInterleave()
{
	for (uint32_t i = 0U; i < 196U; i++)
        m_deInterData[i] = false;
    
    // The first bit is R(3) which is not used so can be ignored
	for (uint32_t a = 0U; a < 196U; a++)	{
        // Calculate the interleave sequence
		uint32_t interleaveSequence = (a * 181U) % 196U;
        // Shuffle the data
        m_deInterData[a] = m_rawData[interleaveSequence];
    }
}

// Check each row with a Hamming (15,11,3) code and each column with a Hamming (13,9,3) code
void CBPTC19696::decodeErrorCheck()
{
    bool fixing;
	uint32_t count = 0U;
    do {
        fixing = false;
        
        // Run through each of the 15 columns
        bool col[13U];
		for (uint32_t c = 0U; c < 15U; c++) {
			uint32_t pos = c + 1U;
			for (uint32_t a = 0U; a < 13U; a++) {
                col[a] = m_deInterData[pos];
                pos = pos + 15U;
            }
            
            if (CHamming::decode1393(col)) {
				uint32_t pos = c + 1U;
				for (uint32_t a = 0U; a < 13U; a++) {
                    m_deInterData[pos] = col[a];
                    pos = pos + 15U;
                }
                
                fixing = true;
            }
        }
        
        // Run through each of the 9 rows containing data
		for (uint32_t r = 0U; r < 9U; r++) {
			uint32_t pos = (r * 15U) + 1U;
            if (CHamming::decode15113_2(m_deInterData + pos))
                fixing = true;
        }
        
        count++;
    } while (fixing && count < 5U);
}

// Extract the 96 bits of payload
void CBPTC19696::decodeExtractData(uint8_t* data)
{
    bool bData[96U];
	uint32_t pos = 0U;
	for(uint32_t a = 4U; a <= 11U; a++, pos++){
        bData[pos] = m_deInterData[a];
	}
	for(uint32_t a = 16U; a <= 26U; a++, pos++){
        bData[pos] = m_deInterData[a];
	}
	for(uint32_t a = 31U; a <= 41U; a++, pos++){
        bData[pos] = m_deInterData[a];
	}
	for(uint32_t a = 46U; a <= 56U; a++, pos++){
        bData[pos] = m_deInterData[a];
	}
	for(uint32_t a = 61U; a <= 71U; a++, pos++){
        bData[pos] = m_deInterData[a];
	}
	for(uint32_t a = 76U; a <= 86U; a++, pos++){
        bData[pos] = m_deInterData[a];
	}
	for(uint32_t a = 91U; a <= 101U; a++, pos++){
        bData[pos] = m_deInterData[a];
	}
	for(uint32_t a = 106U; a <= 116U; a++, pos++){
        bData[pos] = m_deInterData[a];
	}
	for(uint32_t a = 121U; a <= 131U; a++, pos++){
        bData[pos] = m_deInterData[a];
	}

	bitsToByteBE(bData + 0U,  data[0U]);
	bitsToByteBE(bData + 8U,  data[1U]);
	bitsToByteBE(bData + 16U, data[2U]);
	bitsToByteBE(bData + 24U, data[3U]);
	bitsToByteBE(bData + 32U, data[4U]);
	bitsToByteBE(bData + 40U, data[5U]);
	bitsToByteBE(bData + 48U, data[6U]);
	bitsToByteBE(bData + 56U, data[7U]);
	bitsToByteBE(bData + 64U, data[8U]);
	bitsToByteBE(bData + 72U, data[9U]);
	bitsToByteBE(bData + 80U, data[10U]);
	bitsToByteBE(bData + 88U, data[11U]);
}

// Extract the 96 bits of payload
void CBPTC19696::encodeExtractData(const uint8_t* in)
{
    bool bData[96U];
	byteToBitsBE(in[0U],  bData + 0U);
	byteToBitsBE(in[1U],  bData + 8U);
	byteToBitsBE(in[2U],  bData + 16U);
	byteToBitsBE(in[3U],  bData + 24U);
	byteToBitsBE(in[4U],  bData + 32U);
	byteToBitsBE(in[5U],  bData + 40U);
	byteToBitsBE(in[6U],  bData + 48U);
	byteToBitsBE(in[7U],  bData + 56U);
	byteToBitsBE(in[8U],  bData + 64U);
	byteToBitsBE(in[9U],  bData + 72U);
	byteToBitsBE(in[10U], bData + 80U);
	byteToBitsBE(in[11U], bData + 88U);
    
	for (uint32_t i = 0U; i < 196U; i++)
        m_deInterData[i] = false;
    
	uint32_t pos = 0U;
	for (uint32_t a = 4U; a <= 11U; a++, pos++)
        m_deInterData[a] = bData[pos];
    
	for (uint32_t a = 16U; a <= 26U; a++, pos++)
        m_deInterData[a] = bData[pos];
    
	for (uint32_t a = 31U; a <= 41U; a++, pos++)
        m_deInterData[a] = bData[pos];
    
	for (uint32_t a = 46U; a <= 56U; a++, pos++)
        m_deInterData[a] = bData[pos];
    
	for (uint32_t a = 61U; a <= 71U; a++, pos++)
        m_deInterData[a] = bData[pos];
    
	for (uint32_t a = 76U; a <= 86U; a++, pos++)
        m_deInterData[a] = bData[pos];
    
	for (uint32_t a = 91U; a <= 101U; a++, pos++)
        m_deInterData[a] = bData[pos];
    
	for (uint32_t a = 106U; a <= 116U; a++, pos++)
        m_deInterData[a] = bData[pos];
    
	for (uint32_t a = 121U; a <= 131U; a++, pos++)
        m_deInterData[a] = bData[pos];
}

// Check each row with a Hamming (15,11,3) code and each column with a Hamming (13,9,3) code
void CBPTC19696::encodeErrorCheck()
{
    
    // Run through each of the 9 rows containing data
	for (uint32_t r = 0U; r < 9U; r++) {
		uint32_t pos = (r * 15U) + 1U;
        CHamming::encode15113_2(m_deInterData + pos);
    }
    
    // Run through each of the 15 columns
    bool col[13U];
	for (uint32_t c = 0U; c < 15U; c++) {
		uint32_t pos = c + 1U;
		for (uint32_t a = 0U; a < 13U; a++) {
            col[a] = m_deInterData[pos];
            pos = pos + 15U;
        }
        
        CHamming::encode1393(col);
        
        pos = c + 1U;
		for (uint32_t a = 0U; a < 13U; a++) {
            m_deInterData[pos] = col[a];
            pos = pos + 15U;
        }
    }
}

// Interleave the raw data
void CBPTC19696::encodeInterleave()
{
	for (uint32_t i = 0U; i < 196U; i++)
        m_rawData[i] = false;
    
    // The first bit is R(3) which is not used so can be ignored
	for (uint32_t a = 0U; a < 196U; a++)	{
        // Calculate the interleave sequence
		uint32_t interleaveSequence = (a * 181U) % 196U;
        // Unshuffle the data
        m_rawData[interleaveSequence] = m_deInterData[a];
    }
}

void CBPTC19696::encodeExtractBinary(uint8_t* data)
{
    // First block
	bitsToByteBE(m_rawData + 0U,  data[0U]);
	bitsToByteBE(m_rawData + 8U,  data[1U]);
	bitsToByteBE(m_rawData + 16U, data[2U]);
	bitsToByteBE(m_rawData + 24U, data[3U]);
	bitsToByteBE(m_rawData + 32U, data[4U]);
	bitsToByteBE(m_rawData + 40U, data[5U]);
	bitsToByteBE(m_rawData + 48U, data[6U]);
	bitsToByteBE(m_rawData + 56U, data[7U]);
	bitsToByteBE(m_rawData + 64U, data[8U]);
	bitsToByteBE(m_rawData + 72U, data[9U]);
	bitsToByteBE(m_rawData + 80U, data[10U]);
	bitsToByteBE(m_rawData + 88U, data[11U]);
    
    // Handle the two bits
	uint8_t byte;
	bitsToByteBE(m_rawData + 96U, byte);
    data[12U] = (data[12U] & 0x3FU) | ((byte >> 0) & 0xC0U);
    data[20U] = (data[20U] & 0xFCU) | ((byte >> 4) & 0x03U);
    
    // Second block
	bitsToByteBE(m_rawData + 100U,  data[21U]);
	bitsToByteBE(m_rawData + 108U,  data[22U]);
	bitsToByteBE(m_rawData + 116U,  data[23U]);
	bitsToByteBE(m_rawData + 124U,  data[24U]);
	bitsToByteBE(m_rawData + 132U,  data[25U]);
	bitsToByteBE(m_rawData + 140U,  data[26U]);
	bitsToByteBE(m_rawData + 148U,  data[27U]);
	bitsToByteBE(m_rawData + 156U,  data[28U]);
	bitsToByteBE(m_rawData + 164U,  data[29U]);
	bitsToByteBE(m_rawData + 172U,  data[30U]);
	bitsToByteBE(m_rawData + 180U,  data[31U]);
	bitsToByteBE(m_rawData + 188U,  data[32U]);
}
