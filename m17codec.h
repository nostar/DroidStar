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

#ifndef M17CODEC_H
#define M17CODEC_H

#include <string>
#include "codec.h"
#include "codec2/codec2.h"

class M17Codec : public Codec
{
	Q_OBJECT
public:
	M17Codec(QString callsign, char module, QString hostname, QString host, int port, bool ipv6, QString modem, QString audioin, QString audioout);
	~M17Codec();
	static void encode_callsign(uint8_t *);
	static void decode_callsign(uint8_t *);
	void decode_c2(int16_t *, uint8_t *);
	void encode_c2(int16_t *, uint8_t *);
	void set_mode(bool m){ m_c2->codec2_set_mode(m);}
	bool get_mode(){ return m_c2->codec2_get_mode(); }
	CCodec2 *m_c2;
private slots:
	void process_udp();
	void send_ping();
	void send_disconnect();
	void toggle_tx(bool);
	void start_tx();
	void transmit();
	void hostname_lookup(QHostInfo i);
	void rate_changed(int r) { m_txrate = r; }
	void process_rx_data();
private:
	int m_txrate;
};

#endif // M17CODEC_H
