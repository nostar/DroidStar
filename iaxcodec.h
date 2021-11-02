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

#ifndef IAXCODEC_H
#define IAXCODEC_H

#include <QObject>
#include <QtNetwork>
#include "audioengine.h"
#ifdef USE_FLITE
#include <flite/flite.h>
#endif

class IAXCodec : public QObject
{
	Q_OBJECT
public:
	IAXCodec(QString callsign, QString username, QString password, QString node, QString host, int port, QString audioin, QString audioout);
	~IAXCodec();
	uint8_t get_status(){ return m_status; }
	QString get_host() { return m_host; }
	int get_port() { return m_port; }
	int get_cnt() { return m_cnt; }
	void set_input_src(uint8_t s, QString t) { m_ttsid = s; m_ttstext = t; }
signals:
	void update();
	void update_output_level(unsigned short);
private slots:
	void deleteLater();
	void process_udp();
	void send_connect();
	void send_disconnect();
	void hostname_lookup(QHostInfo i);
	void send_registration(uint16_t dcall = 0);
	void send_call();
	void send_call_auth();
	void send_ack(uint16_t, uint16_t, uint8_t, uint8_t);
	void send_lag_response();
	void send_ping();
	void send_pong();
	void toggle_tx(bool);
	void start_tx();
	void stop_tx();
	void transmit();
	void process_rx_data();
	void send_voice_frame(int16_t *);
	void send_dtmf(QByteArray);
	void send_radio_key(bool);
	void input_src_changed(int id, QString t) { m_ttsid = id; m_ttstext = t; }
	void in_audio_vol_changed(qreal v){ m_audio->set_input_volume(v); }
	void out_audio_vol_changed(qreal v){ m_audio->set_output_volume(v); }
private:
	enum{
		DISCONNECTED,
		CLOSED,
		CONNECTING,
		DMR_AUTH,
		DMR_CONF,
		DMR_OPTS,
		CONNECTED_RW,
		CONNECTED_RO
	} m_status;
	QUdpSocket *m_udp = nullptr;
	QHostAddress m_address;
	QString m_callsign;
	QString m_username;
	QString m_password;
	QString m_node;
	QString m_context;
	QString m_host;
	int m_port;
	uint16_t m_scallno;
	uint16_t m_dcallno;
	uint16_t m_regscallno;
	uint16_t m_regdcallno;
	int m_id;
	QString m_audioin;
	QString m_audioout;
	qint64 m_timestamp;
	uint8_t m_regstat;
	QByteArray m_md5seed;
	QTimer *m_regtimer;
	QTimer *m_pingtimer;
	QTimer *m_rxtimer;
	QTimer *m_txtimer;
	AudioEngine *m_audio;
	uint8_t m_iseq;
	uint8_t m_oseq;
	QQueue<int16_t> m_audioq;
	bool m_tx;
	uint32_t m_rxjitter;
	uint32_t m_rxloss;
	uint32_t m_rxframes;
	uint16_t m_rxdelay;
	uint32_t m_rxdropped;
	uint32_t m_rxooo;
	uint8_t m_ttsid;
	QString m_ttstext;
	uint16_t m_ttscnt;
	int m_cnt;
	//qreal m_rxgain;
#ifdef USE_FLITE
	cst_voice *voice_slt;
	cst_voice *voice_kal;
	cst_voice *voice_awb;
	cst_wave *tts_audio;
#endif
};

#endif // IAXCODEC_H
