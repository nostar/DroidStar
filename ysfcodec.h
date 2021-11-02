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

#ifndef YSFCODEC_H
#define YSFCODEC_H

const unsigned int YSF_FRAME_LENGTH_BYTES = 120U;

const unsigned char YSF_SYNC_BYTES[] = {0xD4U, 0x71U, 0xC9U, 0x63U, 0x4DU};
const unsigned int YSF_SYNC_LENGTH_BYTES = 5U;

const unsigned int YSF_FICH_LENGTH_BYTES = 25U;

const unsigned char YSF_SYNC_OK = 0x01U;

const unsigned int  YSF_CALLSIGN_LENGTH   = 10U;

const unsigned char YSF_FI_HEADER         = 0x00U;
const unsigned char YSF_FI_COMMUNICATIONS = 0x01U;
const unsigned char YSF_FI_TERMINATOR     = 0x02U;
const unsigned char YSF_FI_TEST           = 0x03U;

const unsigned char YSF_DT_VD_MODE1      = 0x00U;
const unsigned char YSF_DT_DATA_FR_MODE  = 0x01U;
const unsigned char YSF_DT_VD_MODE2      = 0x02U;
const unsigned char YSF_DT_VOICE_FR_MODE = 0x03U;

const unsigned char YSF_CM_GROUP1     = 0x00U;
const unsigned char YSF_CM_GROUP2     = 0x01U;
const unsigned char YSF_CM_INDIVIDUAL = 0x03U;

const unsigned char YSF_MR_DIRECT   = 0x00U;
const unsigned char YSF_MR_NOT_BUSY = 0x01U;
const unsigned char YSF_MR_BUSY     = 0x02U;

#include <string>
#include "codec.h"
#include "YSFFICH.h"

class YSFCodec : public Codec
{
	Q_OBJECT
public:
	YSFCodec(QString callsign, QString hostname, QString host, int port, bool ipv6, QString vocoder, QString modem, QString audioin, QString audioout);
	~YSFCodec();
	void set_fcs_mode(bool y, std::string f = "        "){ m_fcs = y; m_fcsname = f; }
private slots:
	void process_udp();
	void process_rx_data();
	void get_ambe();
	void send_ping();
	void send_disconnect();
	void transmit();
	void hostname_lookup(QHostInfo i);
	void send_frame();
	void rate_changed(int r) { m_txfullrate = r;}
	void process_modem_data(QByteArray);
private:
	void decode_dn(uint8_t* data);
	void decode_vw(uint8_t* data);
	void encode_header(bool eot = 0);
	void encode_vw();
	void encode_imbe(unsigned char* data, const unsigned char* imbe);
	void encode_dv2();
	void decode_vd2(uint8_t* data, uint8_t *dt);
	void decode_vd1(uint8_t* data, uint8_t *dt);
	void generate_vch_vd2(const unsigned char*);
	void ysf_scramble(unsigned char *buf, const int len);
	void writeDataFRModeData1(const unsigned char* dt, unsigned char* data);
	void writeDataFRModeData2(const unsigned char* dt, unsigned char* data);
	void writeVDMode2Data(unsigned char* data, const unsigned char* dt);
	void interleave(uint8_t *ambe);

	uint8_t m_fi;
	uint8_t packet_size;
	unsigned char gateway[12];
	unsigned char m_ysfFrame[200];
	unsigned char m_vch[13U];
	unsigned char m_ambe[55];
	//unsigned char m_imbe[55];
	CYSFFICH fich;
	unsigned char ambe_fr[4][24];
	unsigned int ambe_a;
	unsigned int ambe_b;
	unsigned int ambe_c;
	bool m_fcs;
	std::string m_fcsname;
	bool m_txfullrate;
};

#endif
