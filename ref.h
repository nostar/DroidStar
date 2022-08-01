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

#ifndef REF_H
#define REF_H

#include "mode.h"

class REF : public Mode
{
	Q_OBJECT
public:
	REF();
	~REF();
	uint8_t * get_frame(uint8_t *ambe);
private:
	uint8_t packet_size;
private slots:
	void toggle_tx(bool);
	void start_tx();
	void process_udp();
	void process_modem_data(QByteArray);
	void process_rx_data();
	void get_ambe();
	void send_ping();
	void send_disconnect();
	void transmit();
	void format_callsign(QString &s);
	void hostname_lookup(QHostInfo i);
	void module_changed(int m) { m_module = 0x41 + m; m_modeinfo.streamid = 0; }
	void send_frame(uint8_t *);
};

#endif // REF_H
