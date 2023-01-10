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

#ifndef YSF_H
#define YSF_H

#include <cstdint>

const uint32_t YSF_FRAME_LENGTH_BYTES = 120U;

const uint8_t YSF_SYNC_BYTES[] = {0xD4U, 0x71U, 0xC9U, 0x63U, 0x4DU};
const uint32_t YSF_SYNC_LENGTH_BYTES = 5U;

const uint32_t YSF_FICH_LENGTH_BYTES = 25U;

const uint8_t YSF_SYNC_OK = 0x01U;

const uint32_t  YSF_CALLSIGN_LENGTH   = 10U;

const uint8_t YSF_FI_HEADER         = 0x00U;
const uint8_t YSF_FI_COMMUNICATIONS = 0x01U;
const uint8_t YSF_FI_TERMINATOR     = 0x02U;
const uint8_t YSF_FI_TEST           = 0x03U;

const uint8_t YSF_DT_VD_MODE1      = 0x00U;
const uint8_t YSF_DT_DATA_FR_MODE  = 0x01U;
const uint8_t YSF_DT_VD_MODE2      = 0x02U;
const uint8_t YSF_DT_VOICE_FR_MODE = 0x03U;

const uint8_t YSF_CM_GROUP1     = 0x00U;
const uint8_t YSF_CM_GROUP2     = 0x01U;
const uint8_t YSF_CM_INDIVIDUAL = 0x03U;

const uint8_t YSF_MR_DIRECT   = 0x00U;
const uint8_t YSF_MR_NOT_BUSY = 0x01U;
const uint8_t YSF_MR_BUSY     = 0x02U;

#include <string>
#include "mode.h"
#include "YSFFICH.h"

class YSF : public Mode
{
	Q_OBJECT
public:
	YSF();
	~YSF();
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
	void decode_header(uint8_t* data);
	void decode_dn(uint8_t* data);
	void decode_vw(uint8_t* data);
	void encode_header(bool eot = 0);
	void encode_vw();
	void encode_imbe(uint8_t* data, const uint8_t* imbe);
	void encode_dv2();
	void decode_vd2(uint8_t* data, uint8_t *dt);
	void decode_vd1(uint8_t* data, uint8_t *dt);
	void generate_vch_vd2(const uint8_t*);
	void ysf_scramble(uint8_t *buf, const int len);
	void writeDataFRModeData1(const uint8_t* dt, uint8_t* data);
	void writeDataFRModeData2(const uint8_t* dt, uint8_t* data);
	void writeVDMode2Data(uint8_t* data, const uint8_t* dt);
	void interleave(uint8_t *ambe);

	uint8_t m_fi;
	uint8_t packet_size;
	uint8_t gateway[12];
	uint8_t m_ysfFrame[200];
	uint8_t m_vch[13U];
	uint8_t m_ambe[55];
	//uint8_t m_imbe[55];
	CYSFFICH m_fich;
	uint8_t ambe_fr[4][24];
	uint32_t ambe_a;
	uint32_t ambe_b;
	uint32_t ambe_c;
	bool m_fcs;
	std::string m_fcsname;
	bool m_txfullrate;
	QQueue<uint8_t> m_rximbecodecq;
};

#endif
