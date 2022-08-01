/*
 *   Copyright (C) 2009-2016 by Jonathan Naylor G4KLX
 *   Copyright (C) 2018 by Andy Uribe CA6JAU
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

#include "YSFConvolution.h"
#include "Golay24128.h"
#include "YSFFICH.h"
#include "CRCenc.h"

#include <cstdio>
#include <cassert>
#include <cstring>

const uint8_t BIT_MASK_TABLE[] = {0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U};

#define WRITE_BIT1(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT1(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

const uint32_t INTERLEAVE_TABLE[] = {
   0U, 40U,  80U, 120U, 160U,
   2U, 42U,  82U, 122U, 162U,
   4U, 44U,  84U, 124U, 164U,
   6U, 46U,  86U, 126U, 166U,
   8U, 48U,  88U, 128U, 168U,
  10U, 50U,  90U, 130U, 170U,
  12U, 52U,  92U, 132U, 172U,
  14U, 54U,  94U, 134U, 174U,
  16U, 56U,  96U, 136U, 176U,
  18U, 58U,  98U, 138U, 178U,
  20U, 60U, 100U, 140U, 180U,
  22U, 62U, 102U, 142U, 182U,
  24U, 64U, 104U, 144U, 184U,
  26U, 66U, 106U, 146U, 186U,
  28U, 68U, 108U, 148U, 188U,
  30U, 70U, 110U, 150U, 190U,
  32U, 72U, 112U, 152U, 192U,
  34U, 74U, 114U, 154U, 194U,
  36U, 76U, 116U, 156U, 196U,
  38U, 78U, 118U, 158U, 198U};

const uint32_t YSF_SYNC_LENGTH_BYTES = 5U;

CYSFFICH::CYSFFICH()
//m_fich(NULL)
{
	m_fich[0] = 0;
	m_fich[1] = 0;
	m_fich[2] = 0;
	m_fich[3] = 0;
	m_fich[4] = 0;
	m_fich[5] = 0;
}

CYSFFICH::~CYSFFICH()
{
	//delete[] m_fich;
}

bool CYSFFICH::decode(const uint8_t* bytes)
{
	assert(bytes != NULL);

	// Skip the sync bytes
	bytes += YSF_SYNC_LENGTH_BYTES;

	CYSFConvolution viterbi;
	viterbi.start();

	// Deinterleave the FICH and send bits to the Viterbi decoder
	for (uint32_t i = 0U; i < 100U; i++) {
		uint32_t n = INTERLEAVE_TABLE[i];
		uint8_t s0 = READ_BIT1(bytes, n) ? 1U : 0U;

		n++;
		uint8_t s1 = READ_BIT1(bytes, n) ? 1U : 0U;

		viterbi.decode(s0, s1);
	}

	uint8_t output[13U];
	viterbi.chainback(output, 96U);

	uint32_t b0 = CGolay24128::decode24128(output + 0U);
	uint32_t b1 = CGolay24128::decode24128(output + 3U);
	uint32_t b2 = CGolay24128::decode24128(output + 6U);
	uint32_t b3 = CGolay24128::decode24128(output + 9U);

	m_fich[0U] = (b0 >> 4) & 0xFFU;
	m_fich[1U] = ((b0 << 4) & 0xF0U) | ((b1 >> 8) & 0x0FU);
	m_fich[2U] = (b1 >> 0) & 0xFFU;
	m_fich[3U] = (b2 >> 4) & 0xFFU;
	m_fich[4U] = ((b2 << 4) & 0xF0U) | ((b3 >> 8) & 0x0FU);
	m_fich[5U] = (b3 >> 0) & 0xFFU;

	return CCRC::checkCCITT162(m_fich, 6U);
}

void CYSFFICH::encode(uint8_t* bytes)
{
	assert(bytes != NULL);

	// Skip the sync bytes
	bytes += YSF_SYNC_LENGTH_BYTES;

	CCRC::addCCITT162(m_fich, 6U);

	uint32_t b0 = ((m_fich[0U] << 4) & 0xFF0U) | ((m_fich[1U] >> 4) & 0x00FU);
	uint32_t b1 = ((m_fich[1U] << 8) & 0xF00U) | ((m_fich[2U] >> 0) & 0x0FFU);
	uint32_t b2 = ((m_fich[3U] << 4) & 0xFF0U) | ((m_fich[4U] >> 4) & 0x00FU);
	uint32_t b3 = ((m_fich[4U] << 8) & 0xF00U) | ((m_fich[5U] >> 0) & 0x0FFU);

	uint32_t c0 = CGolay24128::encode24128(b0);
	uint32_t c1 = CGolay24128::encode24128(b1);
	uint32_t c2 = CGolay24128::encode24128(b2);
	uint32_t c3 = CGolay24128::encode24128(b3);

	uint8_t conv[13U];
	conv[0U]  = (c0 >> 16) & 0xFFU;
	conv[1U]  = (c0 >> 8) & 0xFFU;
	conv[2U]  = (c0 >> 0) & 0xFFU;
	conv[3U]  = (c1 >> 16) & 0xFFU;
	conv[4U]  = (c1 >> 8) & 0xFFU;
	conv[5U]  = (c1 >> 0) & 0xFFU;
	conv[6U]  = (c2 >> 16) & 0xFFU;
	conv[7U]  = (c2 >> 8) & 0xFFU;
	conv[8U]  = (c2 >> 0) & 0xFFU;
	conv[9U]  = (c3 >> 16) & 0xFFU;
	conv[10U] = (c3 >> 8) & 0xFFU;
	conv[11U] = (c3 >> 0) & 0xFFU;
	conv[12U] = 0x00U;

	CYSFConvolution convolution;
	uint8_t convolved[25U];
	convolution.encode(conv, convolved, 100U);

	uint32_t j = 0U;
	for (uint32_t i = 0U; i < 100U; i++) {
		uint32_t n = INTERLEAVE_TABLE[i];

		bool s0 = READ_BIT1(convolved, j) != 0U;
		j++;

		bool s1 = READ_BIT1(convolved, j) != 0U;
		j++;

		WRITE_BIT1(bytes, n, s0);

		n++;
		WRITE_BIT1(bytes, n, s1);
	}
}

uint8_t CYSFFICH::getFI() const
{
	return (m_fich[0U] >> 6) & 0x03U;
}

uint8_t CYSFFICH::getCS() const
{
	return (m_fich[0U] >> 4) & 0x03U;
}

uint8_t CYSFFICH::getCM() const
{
	return (m_fich[0U] >> 2) & 0x03U;
}

uint8_t CYSFFICH::getBN() const
{
	return m_fich[0U] & 0x03U;
}

uint8_t CYSFFICH::getBT() const
{
	return (m_fich[1U] >> 6) & 0x03U;
}

uint8_t CYSFFICH::getFN() const
{
	return (m_fich[1U] >> 3) & 0x07U;
}

uint8_t CYSFFICH::getFT() const
{
	return m_fich[1U] & 0x07U;
}

uint8_t CYSFFICH::getDT() const
{
	return m_fich[2U] & 0x03U;
}

uint8_t CYSFFICH::getMR() const
{
	return (m_fich[2U] >> 3) & 0x03U;
}

bool CYSFFICH::getVoIP() const
{
	return (m_fich[2U] & 0x04U) == 0x04U;
}

bool CYSFFICH::getDev() const
{
	return (m_fich[2U] & 0x40U) == 0x40U;
}

bool CYSFFICH::getSQL() const
{
	return (m_fich[3U] & 0x80U) == 0x80U;
}

uint8_t CYSFFICH::getSQ() const
{
	return m_fich[3U] & 0x7FU;
}

void CYSFFICH::setFI(uint8_t fi)
{
	m_fich[0U] &= 0x3FU;
	m_fich[0U] |= (fi << 6) & 0xC0U;
}

void CYSFFICH::setCS(uint8_t cs)
{
	m_fich[0U] &= 0xCFU;
	m_fich[0U] |= (cs << 4) & 0x30U;
}

void CYSFFICH::setCM(uint8_t cm)
{
	m_fich[0U] &= 0xF3U;
	m_fich[0U] |= (cm << 2) & 0x0CU;
}

void CYSFFICH::setFN(uint8_t fn)
{
	m_fich[1U] &= 0xC7U;
	m_fich[1U] |= (fn << 3) & 0x38U;
}

void CYSFFICH::setFT(uint8_t ft)
{
	m_fich[1U] &= 0xF8U;
	m_fich[1U] |= ft & 0x07U;
}

void CYSFFICH::setMR(uint8_t mr)
{
	m_fich[2U] &= 0xC7U;
	m_fich[2U] |= (mr << 3) & 0x38U;
}

void CYSFFICH::setVoIP(bool on)
{
	if (on)
		m_fich[2U] |= 0x04U;
	else
		m_fich[2U] &= 0xFBU;
}

void CYSFFICH::setDev(bool on)
{
	if (on)
		m_fich[2U] |= 0x40U;
	else
		m_fich[2U] &= 0xBFU;
}

void CYSFFICH::setDT(uint8_t dt)
{
	m_fich[2U] &= 0xFCU;
	m_fich[2U] |= dt & 0x03U;
}

void CYSFFICH::setSQL(bool on)
{
	if (on)
		m_fich[3U] |= 0x80U;
	else
		m_fich[3U] &= 0x7FU;
}

void CYSFFICH::setSQ(uint8_t sq)
{
	m_fich[3U] &= 0x80U;
	m_fich[3U] |= sq & 0x7FU;
}

void CYSFFICH::setBN(uint8_t bn)
{
	m_fich[0U] &= 0xFCU;
	m_fich[0U] |= bn & 0x03U;
}

void CYSFFICH::setBT(uint8_t bt)
{
	m_fich[1U] &= 0x3FU;
	m_fich[1U] |= (bt << 6) & 0xC0U;
}

void CYSFFICH::load(const uint8_t* fich)
{
	assert(fich != NULL);

	::memcpy(m_fich, fich, 4U);
}

