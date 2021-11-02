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

#ifndef P25CODEC_H
#define P25CODEC_H

#include "codec.h"

class P25Codec : public Codec
{
	Q_OBJECT
public:
	P25Codec(QString callsign, int dmrid, int hostname, QString host, int port, bool ipv6, QString modem, QString audioin, QString audioout);
	~P25Codec();
	unsigned char * get_frame(unsigned char *ambe);
private:
	int m_p25cnt;
	unsigned char imbe[11U];
	int m_hostname;
	uint32_t m_dmrid;
	uint32_t m_txdstid;
private slots:
	void process_udp();
	void process_rx_data();
	void send_ping();
	void send_disconnect();
	void transmit();
	void hostname_lookup(QHostInfo i);
	void dmr_tgid_changed(unsigned int id) { m_txdstid = id; }
	void input_src_changed(int id, QString t) { m_ttsid = id; m_ttstext = t; }
};

#endif // P25CODEC_H
