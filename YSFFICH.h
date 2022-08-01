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

#if !defined(YSFFICH_H)
#define  YSFFICH_H
#include <cstdint>

class CYSFFICH {
public:
	CYSFFICH();
	~CYSFFICH();

	bool decode(const uint8_t* bytes);

	void encode(uint8_t* bytes);

	uint8_t getFI() const;
	uint8_t getCS() const;
	uint8_t getCM() const;
	uint8_t getBN() const;
	uint8_t getBT() const;
	uint8_t getFN() const;
	uint8_t getFT() const;
	uint8_t getDT() const;
	uint8_t getMR() const;
	bool getVoIP() const;
	bool getDev() const;
	bool getSQL() const;
	uint8_t getSQ() const;

	void setFI(uint8_t fi);
	void setCS(uint8_t cs);
	void setCM(uint8_t cm);
	void setFN(uint8_t fn);
	void setFT(uint8_t ft);
	void setBN(uint8_t bn);
	void setBT(uint8_t bt);
	void setDT(uint8_t dt);
	void setMR(uint8_t mr);
	void setVoIP(bool set);
	void setDev(bool set);
	void setSQL(bool set);
	void setSQ(uint8_t sq);

	void load(const uint8_t* fich);

private:
	uint8_t m_fich[6U];
	//m_fich  = new uint8_t[6U];
};

#endif
