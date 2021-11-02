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

class CYSFFICH {
public:
	CYSFFICH();
	~CYSFFICH();

	bool decode(const unsigned char* bytes);

	void encode(unsigned char* bytes);

	unsigned char getFI() const;
	unsigned char getCS() const;
	unsigned char getCM() const;
	unsigned char getBN() const;
	unsigned char getBT() const;
	unsigned char getFN() const;
	unsigned char getFT() const;
	unsigned char getDT() const;
	unsigned char getMR() const;
	bool getVoIP() const;
	bool getDev() const;
	bool getSQL() const;
	unsigned char getSQ() const;

	void setFI(unsigned char fi);
	void setCS(unsigned char cs);
	void setCM(unsigned char cm);
	void setFN(unsigned char fn);
	void setFT(unsigned char ft);
	void setBN(unsigned char bn);
	void setBT(unsigned char bt);
	void setDT(unsigned char dt);
	void setMR(unsigned char mr);
	void setVoIP(bool set);
	void setDev(bool set);
	void setSQL(bool set);
	void setSQ(unsigned char sq);

	void load(const unsigned char* fich);

private:
	unsigned char m_fich[6U];
	//m_fich  = new unsigned char[6U];
};

#endif
