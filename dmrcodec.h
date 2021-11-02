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

#ifndef DMRCODEC_H
#define DMRCODEC_H

#include "codec.h"
//#include <inttypes.h>
#include "DMRData.h"
#include "cbptc19696.h"

class DMRCodec : public Codec
{
	Q_OBJECT
public:
	DMRCodec(QString callsign, uint32_t dmrid, uint8_t essid, QString password, QString lat, QString lon, QString location, QString desc, QString freq, QString url, QString swid, QString pkid, QString options, uint32_t dstid, QString host, uint32_t port, bool ipv6, QString vocoder, QString modem, QString audioin, QString audioout);
	~DMRCodec();
	unsigned char * get_eot();
	void set_cc(uint32_t cc){m_colorcode = cc;}
	void set_slot(uint32_t s){m_slot = s;}
	void set_calltype(uint8_t c){m_flco = FLCO(c);}
private slots:
	void process_udp();
	void process_rx_data();
	void process_modem_data(QByteArray);
	void get_ambe();
	void send_ping();
	void send_disconnect();
	void transmit();
	void hostname_lookup(QHostInfo i);
	void dmr_tgid_changed(unsigned int id) { m_txdstid = id; }
	void dmr_cc_changed(int cc) {m_colorcode = cc + 1; }
	void dmr_slot_changed(int s) {m_slot = s + 1; }
	void dmrpc_state_changed(int pc){ m_pc = (pc ? true : false); }
	void send_frame();
private:
	uint32_t m_dmrid;
	uint32_t m_essid;
	QString m_password;
	QString m_lat;
	QString m_lon;
	QString m_location;
	QString m_desc;
	QString m_freq;
	QString m_url;
	QString m_swid;
	QString m_pkid;
	uint32_t m_txsrcid;
	uint32_t m_txdstid;
	uint32_t m_txstreamid;
	uint8_t packet_size;
	uint8_t m_ambe[27];
	uint32_t m_defsrcid;
	uint8_t m_dmrFrame[55];
	uint8_t m_dataType;
	uint32_t m_colorcode;
	uint32_t m_slot;
	uint32_t m_dmrcnt;
	bool m_pc;
	FLCO m_flco;
	CBPTC19696 m_bptc;
	bool m_raw[128U];
	bool m_data[72U];
	QString m_options;

	void byteToBitsBE(uint8_t byte, bool* bits);
	void bitsToByteBE(const bool* bits, uint8_t& byte);
	void build_frame();
	void encode_header(uint8_t);
	void encode_data();
	void encode16114(bool* d);
	void encode_qr1676(uint8_t* data);
	void get_slot_data(uint8_t* data);
	void lc_get_data(uint8_t*);
	void lc_get_data(bool* bits);
	void encode_embedded_data();
	uint8_t get_embedded_data(uint8_t* data, uint8_t n);
	void get_emb_data(uint8_t* data, uint8_t lcss);
	void full_lc_encode(uint8_t* data, uint8_t type);
	void addDMRDataSync(uint8_t* data, bool duplex);
	void addDMRAudioSync(uint8_t* data, bool duplex);
	void setup_connection();
};

#endif // DMRCODEC_H
