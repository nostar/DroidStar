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

#ifndef MODE_H
#define MODE_H

#include <QObject>
#include <QtNetwork>
#ifdef USE_FLITE
#include <flite/flite.h>
#endif
#include <imbe_vocoder_api.h>
#ifdef VOCODER_PLUGIN
#include "vocoder_plugin.h"
#else
#include <vocoder_plugin_api.h>
#endif
#include "audioengine.h"
#if !defined(Q_OS_IOS)
#include "serialambe.h"
#include "serialmodem.h"
#endif

class Mode : public QObject
{
	Q_OBJECT
public:
	Mode();
	~Mode();
	static Mode* create_mode(QString);
	void init(QString callsign, uint32_t dmrid, char module, QString refname, QString host, int port, bool ipv6, QString vocoder, QString modem, QString audioin, QString audioout);
	void set_modem_flags(bool rxInvert, bool txInvert, bool pttInvert, bool useCOSAsLockout, bool duplex)
	{
		m_rxInvert = rxInvert;
		m_txInvert = txInvert;
		m_pttInvert = pttInvert;
		m_useCOSAsLockout = useCOSAsLockout;
		m_duplex = duplex;
	}
	void set_modem_params(uint32_t baud, uint32_t rxfreq, uint32_t txfreq, uint32_t txDelay, float rxLevel, float rfLevel, uint32_t ysfTXHang, float cwIdTXLevel, float dstarTXLevel, float dmrTXLevel, float ysfTXLevel, float p25TXLevel, float nxdnTXLevel, float pocsagTXLevel, float m17TXLevel)
	{
		m_baud = baud;
		m_rxfreq = rxfreq;
		m_txfreq = txfreq;
		m_txDelay = txDelay;
		m_rxLevel = rxLevel;
		m_rfLevel = rfLevel;
		m_ysfTXHang = ysfTXHang;
		m_cwIdTXLevel = cwIdTXLevel;
		m_dstarTXLevel = dstarTXLevel;
		m_dmrTXLevel = dmrTXLevel;
		m_ysfTXLevel = ysfTXLevel;
		m_p25TXLevel = p25TXLevel;
		m_nxdnTXLevel = nxdnTXLevel;
		m_pocsagTXLevel = pocsagTXLevel;
		m_m17TXLevel = m17TXLevel;
	}
	virtual void set_dmr_params(uint8_t, QString, QString, QString, QString, QString, QString, QString, QString, QString, QString) {}
	virtual void set_iax_params(QString, QString, QString, QString, int) {}
	bool get_hwrx() { return m_hwrx; }
	bool get_hwtx() { return m_hwtx; }
	void set_hostname(std::string);
	void set_callsign(std::string);
	struct MODEINFO {
		qint64 ts;
		int status;
		int stream_state;
		QString callsign;
		QString gw;
		QString gw2;
		QString src;
		QString dst;
		QString usertxt;
		QString netmsg;
		uint32_t gwid;
		uint32_t srcid;
		uint32_t dstid;
		uint8_t slot;
		uint8_t cc;
		QString ambedesc;
		QString ambeprodid;
		QString ambeverstr;
		QString mmdvmdesc;
		QString mmdvm;
		QString host;
		int port;
		bool path;
		char type;
		uint16_t frame_number;
		uint8_t frame_total;
		int count;
		uint32_t streamid;
		bool mode;
		bool sw_vocoder_loaded;
		bool hw_vocoder_loaded;
	} m_modeinfo;
	enum{
		DISCONNECTED,
		CLOSED,
		CONNECTING,
		DMR_AUTH,
		DMR_CONF,
		DMR_OPTS,
		CONNECTED_RW,
		CONNECTED_RO
	};
	enum{
		STREAM_NEW,
		STREAMING,
		STREAM_END,
		STREAM_LOST,
		STREAM_IDLE,
		TRANSMITTING,
		TRANSMITTING_MODEM,
		STREAM_UNKNOWN
	};
signals:
	void update(Mode::MODEINFO);
	void update_output_level(unsigned short);
protected slots:
	virtual void send_disconnect(){}
	virtual void hostname_lookup(QHostInfo){}
	virtual void mmdvm_direct_connect(){}

	void ambe_connect_status(bool);
	void mmdvm_connect_status(bool);
	void send_connect();
	void input_src_changed(int id, QString t) { m_ttsid = id; m_ttstext = t; }
	void start_tx();
	void stop_tx();
	void toggle_tx(bool);
	void deleteLater();
	void in_audio_vol_changed(qreal);
	void out_audio_vol_changed(qreal);
	bool load_vocoder_plugin();
	void swrx_state_changed(int s) {m_hwrx = !s; }
	void swtx_state_changed(int s) {m_hwtx = !s; }
	void agc_state_changed(int s);
	void mycall_changed(QString mc) { m_txmycall = mc; }
	void urcall_changed(QString uc) { m_txurcall = uc; }
	void rptr1_changed(QString r1) { m_txrptr1 = r1; }
	void rptr2_changed(QString r2) { m_txrptr2 = r2; }
	void usrtxt_changed(QString t) { m_txusrtxt = t; }
	void module_changed(char m) { m_module = m; m_modeinfo.streamid = 0; }
protected:
	QUdpSocket *m_udp = nullptr;
	QHostAddress m_address;
	char m_module;
	uint32_t m_dmrid;
	QString m_refname;
	bool m_tx;
	uint16_t m_txcnt;
	uint16_t m_ttscnt;
	uint8_t m_ttsid;
	QString m_ttstext;
	QString m_txmycall;
	QString m_txurcall;
	QString m_txrptr1;
	QString m_txrptr2;
	QString m_txusrtxt;
#ifdef USE_FLITE
	cst_voice *voice_slt;
	cst_voice *voice_kal;
	cst_voice *voice_awb;
	cst_wave *tts_audio;
#endif
	QTimer *m_ping_timer;
	QTimer *m_txtimer;
	QTimer *m_rxtimer;
	AudioEngine *m_audio;
	QString m_audioin;
	QString m_audioout;
	uint32_t m_rxwatchdog;
	uint8_t m_attenuation;
	uint8_t m_rxtimerint;
	uint8_t m_txtimerint;
	QQueue<uint8_t> m_rxcodecq;
	QQueue<uint8_t> m_txcodecq;
	QQueue<uint8_t> m_rxmodemq;
	imbe_vocoder vocoder;
#ifdef VOCODER_PLUGIN
	Vocoder *m_mbevocoder;
#else
	VocoderPlugin *m_mbevocoder;
#endif
	QString m_vocoder;
	QString m_modemport;
#if defined(Q_OS_IOS)
	void *m_modem;
	void *m_ambedev;
#else
	SerialModem *m_modem;
	SerialAMBE *m_ambedev;
#endif
	bool m_hwrx;
	bool m_hwtx;
	bool m_ipv6;

	uint32_t m_baud;
	uint32_t m_rxfreq;
	uint32_t m_txfreq;
	uint32_t m_dmrColorCode;
	bool m_ysfLoDev;
	uint32_t m_ysfTXHang;
	uint32_t m_p25TXHang;
	uint32_t m_nxdnTXHang;
	bool m_duplex;
	bool m_rxInvert;
	bool m_txInvert;
	bool m_pttInvert;
	uint32_t m_txDelay;
	uint32_t m_dmrDelay;
	float m_rxLevel;
	float m_rfLevel;
	float m_cwIdTXLevel;
	float m_dstarTXLevel;
	float m_dmrTXLevel;
	float m_ysfTXLevel;
	float m_p25TXLevel;
	float m_nxdnTXLevel;
	float m_pocsagTXLevel;
	float m_fmTXLevel;
	float m_m17TXLevel;
	bool m_debug;
	bool m_useCOSAsLockout;
	bool m_dstarEnabled;
	bool m_dmrEnabled;
	bool m_ysfEnabled;
	bool m_p25Enabled;
	bool m_nxdnEnabled;
	bool m_pocsagEnabled;
	bool m_fmEnabled;
	int m_rxDCOffset;
	int m_txDCOffset;
};

#endif // MODE_H
