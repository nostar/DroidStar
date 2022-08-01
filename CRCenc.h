/*
 *   Copyright (C) 2015,2016 by Jonathan Naylor G4KLX
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

#if !defined(CRC_H)
#define	CRC_H
#include <cstdint>

class CCRC
{
public:
	//static bool checkFiveBit(bool* in, uint32_t tcrc);
	static void bitsToByteBE(const bool* bits, uint8_t& byte);
	static void encodeFiveBit(const bool* in, uint32_t& tcrc);

	static void addCCITT161(uint8_t* in, uint32_t length);
	static void addCCITT162(uint8_t* in, uint32_t length);

	static bool checkCCITT161(const uint8_t* in, uint32_t length);
	static bool checkCCITT162(const uint8_t* in, uint32_t length);

	static unsigned char crc8(const uint8_t* in, uint32_t length);
	
	static unsigned char addCRC(const uint8_t* in, uint32_t length);
};

#endif
