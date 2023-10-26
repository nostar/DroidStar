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

#ifndef DMR_H
#define DMR_H

#include "mode.h"
#include "DMRDefines.h"
#include "cbptc19696.h"

class DMR : public Mode
{
	Q_OBJECT
public:
	DMR();
	~DMR();
	void set_dmr_params(uint8_t essid, QString password, QString lat, QString lon, QString location, QString desc, QString freq, QString url, QString swid, QString pkid, QString options);
	uint8_t * get_eot();
private slots:
	void process_udp();
	void process_rx_data();
	void process_modem_data(QByteArray);
	void get_ambe();
	void send_ping();
	void send_disconnect();
	void transmit();
	void hostname_lookup(QHostInfo i);
	void dmr_tgid_changed(int id) { m_txdstid = id; }
    void dmrpc_state_changed(int p){m_flco = p ? FLCO_USER_USER : FLCO_GROUP; }
    void cc_changed(int cc) {m_txcc = cc;}
	void slot_changed(int s) {m_txslot = s + 1; }
	void send_frame();
private:
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
	uint8_t m_txslot;
	uint8_t m_txcc;
	uint8_t packet_size;
	uint8_t m_ambe[27];
	uint32_t m_defsrcid;
	uint8_t m_dmrFrame[55];
	uint8_t m_dataType;
	uint32_t m_dmrcnt;
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

#endif // DMR_H
