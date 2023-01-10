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

#ifndef P25_H
#define P25_H

#include "mode.h"

class P25 : public Mode
{
	Q_OBJECT
public:
	P25();
	~P25();
	uint8_t * get_frame(uint8_t *ambe);
private:
	int m_p25cnt;
	uint8_t imbe[11U];
	int m_dstid;
	uint32_t m_txdstid;
private slots:
	void process_udp();
	void process_rx_data();
	void send_ping();
	void send_disconnect();
	void transmit();
	void hostname_lookup(QHostInfo i);
	void dmr_tgid_changed(uint32_t id) { m_txdstid = id; }
};

#endif // P25_H
