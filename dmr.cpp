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

#include <iostream>
#include <cstring>
#include "dmr.h"
#include "cgolay2087.h"
#include "crs129.h"
#include "SHA256.h"
#include "CRCenc.h"
#include "MMDVMDefines.h"

//#define DEBUG

const uint32_t ENCODING_TABLE_1676[] =
	{0x0000U, 0x0273U, 0x04E5U, 0x0696U, 0x09C9U, 0x0BBAU, 0x0D2CU, 0x0F5FU, 0x11E2U, 0x1391U, 0x1507U, 0x1774U,
	 0x182BU, 0x1A58U, 0x1CCEU, 0x1EBDU, 0x21B7U, 0x23C4U, 0x2552U, 0x2721U, 0x287EU, 0x2A0DU, 0x2C9BU, 0x2EE8U,
	 0x3055U, 0x3226U, 0x34B0U, 0x36C3U, 0x399CU, 0x3BEFU, 0x3D79U, 0x3F0AU, 0x411EU, 0x436DU, 0x45FBU, 0x4788U,
	 0x48D7U, 0x4AA4U, 0x4C32U, 0x4E41U, 0x50FCU, 0x528FU, 0x5419U, 0x566AU, 0x5935U, 0x5B46U, 0x5DD0U, 0x5FA3U,
	 0x60A9U, 0x62DAU, 0x644CU, 0x663FU, 0x6960U, 0x6B13U, 0x6D85U, 0x6FF6U, 0x714BU, 0x7338U, 0x75AEU, 0x77DDU,
	 0x7882U, 0x7AF1U, 0x7C67U, 0x7E14U, 0x804FU, 0x823CU, 0x84AAU, 0x86D9U, 0x8986U, 0x8BF5U, 0x8D63U, 0x8F10U,
	 0x91ADU, 0x93DEU, 0x9548U, 0x973BU, 0x9864U, 0x9A17U, 0x9C81U, 0x9EF2U, 0xA1F8U, 0xA38BU, 0xA51DU, 0xA76EU,
	 0xA831U, 0xAA42U, 0xACD4U, 0xAEA7U, 0xB01AU, 0xB269U, 0xB4FFU, 0xB68CU, 0xB9D3U, 0xBBA0U, 0xBD36U, 0xBF45U,
	 0xC151U, 0xC322U, 0xC5B4U, 0xC7C7U, 0xC898U, 0xCAEBU, 0xCC7DU, 0xCE0EU, 0xD0B3U, 0xD2C0U, 0xD456U, 0xD625U,
	 0xD97AU, 0xDB09U, 0xDD9FU, 0xDFECU, 0xE0E6U, 0xE295U, 0xE403U, 0xE670U, 0xE92FU, 0xEB5CU, 0xEDCAU, 0xEFB9U,
	 0xF104U, 0xF377U, 0xF5E1U, 0xF792U, 0xF8CDU, 0xFABEU, 0xFC28U, 0xFE5BU};

DMR::DMR() :
	m_txslot(2),
	m_txcc(1)
{
	m_dmrcnt = 0;
	m_flco = FLCO_GROUP;
	m_attenuation = 5;
}

DMR::~DMR()
{
}

void DMR::set_dmr_params(uint8_t essid, QString password, QString lat, QString lon, QString location, QString desc, QString freq, QString url, QString swid, QString pkid, QString options)
{
	if (essid){
		m_essid = m_dmrid * 100 + (essid-1);
	}
	else{
		m_essid = m_dmrid;
	}

	m_password = password;
	m_lat = lat;
	m_lon = lon;
	m_location = location;
	m_desc = desc;
	m_freq = freq;
	m_url = url;
	m_swid = swid;
	m_pkid = pkid;
	m_options = options;
}

void DMR::process_udp()
{
	QByteArray buf;
	QByteArray in;
	QByteArray out;
	QHostAddress sender;
	quint16 senderPort;
	CSHA256 sha256;
	char buffer[400U];

	buf.resize(m_udp->pendingDatagramSize());
	m_udp->readDatagram(buf.data(), buf.size(), &sender, &senderPort);
#ifdef DEBUG
	fprintf(stderr, "RECV: ");
	for(int i = 0; i < buf.size(); ++i){
		fprintf(stderr, "%02x ", (uint8_t)buf.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
	if((m_modeinfo.status != CONNECTED_RW) && (::memcmp(buf.data() + 3, "NAK", 3U) == 0)){
		m_modeinfo.status = DISCONNECTED;
	}
	if((m_modeinfo.status != CONNECTED_RW) && (::memcmp(buf.data(), "MSTCL", 5U) == 0)){
		m_modeinfo.status = CLOSED;
	}
	if((m_modeinfo.status != CONNECTED_RW) && (::memcmp(buf.data(), "RPTACK", 6U) == 0)){
		switch(m_modeinfo.status){
		case CONNECTING:
			m_modeinfo.status = DMR_AUTH;
			in.append(buf[6]);
			in.append(buf[7]);
			in.append(buf[8]);
			in.append(buf[9]);
			in.append(m_password.toUtf8());

			out.clear();
			out.resize(40);
			out[0] = 'R';
			out[1] = 'P';
			out[2] = 'T';
			out[3] = 'K';
			out[4] = (m_essid >> 24) & 0xff;
			out[5] = (m_essid >> 16) & 0xff;
			out[6] = (m_essid >> 8) & 0xff;
			out[7] = (m_essid >> 0) & 0xff;
			sha256.buffer((uint8_t *)in.data(), (uint32_t)(m_password.size() + sizeof(uint32_t)), (uint8_t *)out.data() + 8U);
			break;
		case DMR_AUTH:
			out.clear();
			buffer[0] = 'R';
			buffer[1] = 'P';
			buffer[2] = 'T';
			buffer[3] = 'C';
			buffer[4] = (m_essid >> 24) & 0xff;
			buffer[5] = (m_essid >> 16) & 0xff;
			buffer[6] = (m_essid >> 8) & 0xff;
			buffer[7] = (m_essid >> 0) & 0xff;

			m_modeinfo.status = DMR_CONF;
			char latitude[20U];
			char longitude[20U];

			sprintf(latitude, "%08f", m_lat.toFloat());
			sprintf(longitude, "%09f", m_lon.toFloat());

			char *p;
			if((p = strchr(latitude, ',')) != NULL){
				*p = '.';
			}
			if((p = strchr(longitude, ',')) != NULL){
				*p = '.';
			}
			::sprintf(buffer + 8U, "%-8.8s%09u%09u%02u%02u%8.8s%9.9s%03d%-20.20s%-19.19s%c%-124.124s%-40.40s%-40.40s", m_modeinfo.callsign.toStdString().c_str(),
					  m_freq.toUInt(), m_freq.toUInt(), 1, 1, latitude, longitude, 0, m_location.toStdString().c_str(), m_desc.toStdString().c_str(), '4',
					  m_url.toStdString().c_str(), m_swid.toStdString().c_str(), m_pkid.toStdString().c_str());
			out.append(buffer, 302);
			break;
		case DMR_CONF:
			setup_connection();
			if(m_options.size()){
				out.clear();
				out.append('R');
				out.append('P');
				out.append('T');
				out.append('O');
				out.append((m_essid >> 24) & 0xff);
				out.append((m_essid >> 16) & 0xff);
				out.append((m_essid >> 8) & 0xff);
				out.append((m_essid >> 0) & 0xff);
				out.append(m_options.toUtf8());
			}
			break;
		case DMR_OPTS:
			//setup_connection();
			break;
		default:
			break;
		}
		m_udp->writeDatagram(out, m_address, m_modeinfo.port);
	}
	if((buf.size() == 11) && (::memcmp(buf.data(), "MSTPONG", 7U) == 0)){
		m_modeinfo.count++;
	}
	if((buf.size() != 55) && ( (m_modeinfo.stream_state == STREAM_LOST) || (m_modeinfo.stream_state == STREAM_END) )){
		m_modeinfo.stream_state = STREAM_IDLE;
	}
	if((buf.size() == 55) &&
		(::memcmp(buf.data(), "DMRD", 4U) == 0) &&
		((uint8_t)buf.data()[15] & 0x20) &&
		(m_modeinfo.status == CONNECTED_RW))
	{
		m_rxwatchdog = 0;
		uint8_t t;
		if((uint8_t)buf.data()[15] & 0x02){
			qDebug() << "DMR RX EOT";
			m_modeinfo.stream_state = STREAM_END;
			m_modeinfo.ts = QDateTime::currentMSecsSinceEpoch();
			m_modeinfo.streamid = 0;
			t = 0x42;
		}
		else if((uint8_t)buf.data()[15] & 0x01){
			m_audio->start_playback();
			if(!m_rxtimer->isActive()){
				m_rxtimer->start(m_rxtimerint);
			}
			m_modeinfo.stream_state = STREAM_NEW;
			m_modeinfo.ts = QDateTime::currentMSecsSinceEpoch();
			m_modeinfo.srcid = (uint32_t)((buf.data()[5] << 16) | ((buf.data()[6] << 8) & 0xff00) | (buf.data()[7] & 0xff));
			m_modeinfo.dstid = (uint32_t)((buf.data()[8] << 16) | ((buf.data()[9] << 8) & 0xff00) | (buf.data()[10] & 0xff));
			m_modeinfo.gwid = (uint32_t)((buf.data()[11] << 24) | ((buf.data()[12] << 16) & 0xff0000) | ((buf.data()[13] << 8) & 0xff00) | (buf.data()[14] & 0xff));
			m_modeinfo.streamid = (uint32_t)((buf.data()[16] << 24) | ((buf.data()[17] << 16) & 0xff0000) | ((buf.data()[18] << 8) & 0xff00) | (buf.data()[19] & 0xff));
			m_modeinfo.frame_number = (uint8_t)buf.data()[4];
			m_modeinfo.slot = (buf.data()[15] & 0x80) ? 2 : 1;
			t = 0x41;
			qDebug() << "New DMR stream from " << m_modeinfo.srcid << " to " << m_modeinfo.dstid;
		}
		if(m_modem){
			m_rxmodemq.append(MMDVM_FRAME_START);
			m_rxmodemq.append(0x25);
			m_rxmodemq.append(MMDVM_DMR_DATA2);
			m_rxmodemq.append(t);

			for(int i = 0; i < 33; ++i){
				m_rxmodemq.append(buf.data()[20+i]);
			};
		}
	}
	if((buf.size() == 55) &&
		(::memcmp(buf.data(), "DMRD", 4U) == 0) &&
		!((uint8_t)buf.data()[15] & 0x20) &&
		(m_modeinfo.status == CONNECTED_RW))
	{
		if(!m_tx && ( (m_modeinfo.stream_state == STREAM_LOST) || (m_modeinfo.stream_state == STREAM_END) || (m_modeinfo.stream_state == STREAM_IDLE) )){
			m_audio->start_playback();
			if(!m_rxtimer->isActive()){
				m_rxtimer->start(m_rxtimerint);
			}
			m_modeinfo.stream_state = STREAM_NEW;
		}
		else{
			m_modeinfo.stream_state = STREAMING;
		}
		m_rxwatchdog = 0;

		uint8_t dmrframe[33];
		uint8_t dmr3ambe[27];
		uint8_t dmrsync[7];
		// get the 33 bytes ambe
		memcpy(dmrframe, &(buf.data()[20]), 33);
		// extract the 3 ambe frames
		memcpy(dmr3ambe, dmrframe, 14);
		dmr3ambe[13] &= 0xF0;
		dmr3ambe[13] |= (dmrframe[19] & 0x0F);
		memcpy(&dmr3ambe[14], &dmrframe[20], 13);
		// extract sync
		dmrsync[0] = dmrframe[13] & 0x0F;
		::memcpy(&dmrsync[1], &dmrframe[14], 5);
		dmrsync[6] = dmrframe[19] & 0xF0;
		m_modeinfo.srcid =		(uint32_t)((buf.data()[5] << 16) | ((buf.data()[6] << 8) & 0xff00) | (buf.data()[7] & 0xff));
		m_modeinfo.dstid =		(uint32_t)((buf.data()[8] << 16) | ((buf.data()[9] << 8) & 0xff00) | (buf.data()[10] & 0xff));
		m_modeinfo.gwid =		(uint32_t)((buf.data()[11] << 24) | ((buf.data()[12] << 16) & 0xff0000) | ((buf.data()[13] << 8) & 0xff00) | (buf.data()[14] & 0xff));
		m_modeinfo.streamid =	(uint32_t)((buf.data()[16] << 24) | ((buf.data()[17] << 16) & 0xff0000) | ((buf.data()[18] << 8) & 0xff00) | (buf.data()[19] & 0xff));
		m_modeinfo.frame_number = (uint8_t)buf.data()[4];

		if(m_modem){
			uint8_t t = ((uint8_t)buf.data()[15] & 0x0f);
			if(!t) t = 0x20;

			m_rxmodemq.append(MMDVM_FRAME_START);
			m_rxmodemq.append(0x25);
			m_rxmodemq.append(MMDVM_DMR_DATA2);
			m_rxmodemq.append(t);

			for(int i = 0; i < 33; ++i){
				m_rxmodemq.append(buf.data()[20+i]);
			}
		}

		for(int i = 0; i < 3; ++i){
			for(int j = 0; j < 9; ++j){
				m_rxcodecq.append(dmr3ambe[j + (9*i)]);
			}
		}
		//uint32_t id = (uint32_t)((buf.data()[5] << 16) | ((buf.data()[6] << 8) & 0xff00) | (buf.data()[7] & 0xff));
	}
	emit update(m_modeinfo);
#ifdef DEBUG
	if(out.size() > 0){
		fprintf(stderr, "SEND: ");
		for(int i = 0; i < out.size(); ++i){
			fprintf(stderr, "%02x ", (uint8_t)out.data()[i]);
		}
		fprintf(stderr, "\n");
		fflush(stderr);
	}
#endif
}

void DMR::setup_connection()
{
	m_modeinfo.status = CONNECTED_RW;
	//m_mbeenc->set_gain_adjust(2.5);
	m_modeinfo.sw_vocoder_loaded = load_vocoder_plugin();
	m_txtimer = new QTimer();
	connect(m_txtimer, SIGNAL(timeout()), this, SLOT(transmit()));
	m_rxtimer = new QTimer();
	connect(m_rxtimer, SIGNAL(timeout()), this, SLOT(process_rx_data()));
	m_ping_timer = new QTimer();
	connect(m_ping_timer, SIGNAL(timeout()), this, SLOT(send_ping()));
	m_ping_timer->start(5000);
	if(m_vocoder != ""){
		m_hwrx = true;
		m_hwtx = true;
		m_modeinfo.hw_vocoder_loaded = true;
#if !defined(Q_OS_IOS)
		m_ambedev = new SerialAMBE("DMR");
		m_ambedev->connect_to_serial(m_vocoder);
		connect(m_ambedev, SIGNAL(connected(bool)), this, SLOT(ambe_connect_status(bool)));
		connect(m_ambedev, SIGNAL(data_ready()), this, SLOT(get_ambe()));
#endif
	}
	else{
		m_hwrx = false;
		m_hwtx = false;
	}
	if(m_modemport != ""){
#if !defined(Q_OS_IOS)
		m_modem = new SerialModem("DMR");
		m_modem->set_modem_flags(m_rxInvert, m_txInvert, m_pttInvert, m_useCOSAsLockout, m_duplex);
		m_modem->set_modem_params(m_baud, m_rxfreq, m_txfreq, m_txDelay, m_rxLevel, m_rfLevel, m_ysfTXHang, m_cwIdTXLevel, m_dstarTXLevel, m_dmrTXLevel, m_ysfTXLevel, m_p25TXLevel, m_nxdnTXLevel, m_pocsagTXLevel, m_m17TXLevel);
		m_modem->connect_to_serial(m_modemport);
		connect(m_modem, SIGNAL(connected(bool)), this, SLOT(mmdvm_connect_status(bool)));
		connect(m_modem, SIGNAL(modem_data_ready(QByteArray)), this, SLOT(process_modem_data(QByteArray)));
#endif
	}
	m_audio = new AudioEngine(m_audioin, m_audioout);
	m_audio->init();
}

void DMR::hostname_lookup(QHostInfo i)
{
	if (!i.addresses().isEmpty()) {
		QByteArray out;
		out.append('R');
		out.append('P');
		out.append('T');
		out.append('L');
		out.append((m_essid >> 24) & 0xff);
		out.append((m_essid >> 16) & 0xff);
		out.append((m_essid >> 8) & 0xff);
		out.append((m_essid >> 0) & 0xff);
		m_address = i.addresses().first();
		m_udp = new QUdpSocket(this);
		connect(m_udp, SIGNAL(readyRead()), this, SLOT(process_udp()));
		m_udp->writeDatagram(out, m_address, m_modeinfo.port);
#ifdef DEBUG
		fprintf(stderr, "CONN: ");
		for(int i = 0; i < out.size(); ++i){
			fprintf(stderr, "%02x ", (uint8_t)out.data()[i]);
		}
		fprintf(stderr, "\n");
		fflush(stderr);
#endif
	}
}

void DMR::send_ping()
{
	QByteArray out;
	char tag[] = { 'R','P','T','P','I','N','G' };
	out.append(tag, 7);
	out.append((m_essid >> 24) & 0xff);
	out.append((m_essid >> 16) & 0xff);
	out.append((m_essid >> 8) & 0xff);
	out.append((m_essid >> 0) & 0xff);
	m_udp->writeDatagram(out, m_address, m_modeinfo.port);
#ifdef DEBUG
	fprintf(stderr, "PING: ");
	for(int i = 0; i < out.size(); ++i){
		fprintf(stderr, "%02x ", (uint8_t)out.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
}

void DMR::send_disconnect()
{
	QByteArray out;
	out.append('R');
	out.append('P');
	out.append('T');
	out.append('C');
	out.append('L');
	out.append((m_essid >> 24) & 0xff);
	out.append((m_essid >> 16) & 0xff);
	out.append((m_essid >> 8) & 0xff);
	out.append((m_essid >> 0) & 0xff);
	m_udp->writeDatagram(out, m_address, m_modeinfo.port);
#ifdef DEBUG
	fprintf(stderr, "SEND: ");
	for(int i = 0; i < out.size(); ++i){
		fprintf(stderr, "%02x ", (uint8_t)out.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
}

void DMR::process_modem_data(QByteArray d)
{
	QByteArray txdata;
	uint8_t lcData[12U];

	uint8_t *p_frame = (uint8_t *)(d.data());
	m_dataType = p_frame[3U] & 0x0f;

	if ((p_frame[3U] & DMR_SYNC_DATA) == DMR_SYNC_DATA){
		if((m_dataType == DT_VOICE_LC_HEADER) && (m_modeinfo.stream_state == STREAM_IDLE)){
			m_modeinfo.stream_state = TRANSMITTING_MODEM;
		}
		else if(m_dataType == DT_TERMINATOR_WITH_LC){
			m_modeinfo.stream_state = STREAM_IDLE;
		}

		m_dmrcnt = 0;
		m_bptc.decode(p_frame + 4, lcData);
		m_txdstid = lcData[3U] << 16 | lcData[4U] << 8 | lcData[5U];
		m_txsrcid = lcData[6U] << 16 | lcData[7U] << 8 | lcData[8U];
		m_flco = FLCO(lcData[0U] & 0x3FU);
		build_frame();
		::memcpy(m_dmrFrame + 20U, p_frame + 4, 33U);
		txdata.append((char *)m_dmrFrame, 55);
		m_udp->writeDatagram(txdata, m_address, m_modeinfo.port);
	}
	else {
		m_dataType = (m_dmrcnt % 6U) ? DT_VOICE : DT_VOICE_SYNC;
		build_frame();
		::memcpy(m_dmrFrame + 20U, p_frame + 4, 33U);
		txdata.append((char *)m_dmrFrame, 55);
		m_udp->writeDatagram(txdata, m_address, m_modeinfo.port);
		++m_dmrcnt;
	}
#ifdef DEBUG
	fprintf(stderr, "SEND:%d: ", txdata.size());
	for(int i = 0; i < txdata.size(); ++i){
		fprintf(stderr, "%02x ", (uint8_t)txdata.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
}

void DMR::transmit()
{
	uint8_t ambe[72];
	int16_t pcm[160];

#ifdef USE_FLITE
	if(m_ttsid > 0){
		for(int i = 0; i < 160; ++i){
			if(m_ttscnt >= tts_audio->num_samples/2){
				pcm[i] = 0;
			}
			else{
				pcm[i] = tts_audio->samples[m_ttscnt*2] / 2;
				m_ttscnt++;
			}
		}
	}
#endif
	if(m_ttsid == 0){
		if(m_audio->read(pcm, 160)){
		}
		else{
			return;
		}
	}

	if(m_hwtx){
#if !defined(Q_OS_IOS)
		m_ambedev->encode(pcm);
#endif
	}
	else{
		if(m_modeinfo.sw_vocoder_loaded){
			m_mbevocoder->encode_2450x1150(pcm, ambe);
		}
		for(int i = 0; i < 9; ++i){
			m_txcodecq.append(ambe[i]);
		}
	}

	if(m_tx && (m_txcodecq.size() >= 27)){
		for(int i = 0; i < 27; ++i){
			m_ambe[i] = m_txcodecq.dequeue();
		}
		send_frame();
	}
	else if(m_tx == false){
		send_frame();
	}
}

void DMR::send_frame()
{
	QByteArray txdata;

	m_txsrcid = m_dmrid;
	if(m_tx){
		m_modeinfo.stream_state = TRANSMITTING;
		m_modeinfo.slot = m_txslot;

		if(!m_dmrcnt){
			encode_header(DT_VOICE_LC_HEADER);
			m_txstreamid = static_cast<uint32_t>(::rand());
		}
		else{
			::memcpy(m_dmrFrame + 20U, m_ambe, 13U);
			m_dmrFrame[33U] = m_ambe[13U] & 0xF0U;
			m_dmrFrame[39U] = m_ambe[13U] & 0x0FU;
			::memcpy(m_dmrFrame + 40U, &m_ambe[14U], 13U);
			encode_data();
		}

		build_frame();
		txdata.append((char *)m_dmrFrame, 55);
		m_udp->writeDatagram(txdata, m_address, m_modeinfo.port);
		++m_dmrcnt;
/*
		if(!m_dmrcnt){
			for (int i = 0U; i < 3; i++) {
				m_dmrFrame[4U] = m_dmrcnt;
				txdata.append((char *)m_dmrFrame, 55);
				m_udp->writeDatagram(txdata, m_address, m_modeinfo.port);
				m_dmrcnt++;
			}

		}
		else{
			++m_dmrcnt;
			txdata.append((char *)m_dmrFrame, 55);
			m_udp->writeDatagram(txdata, m_address, m_modeinfo.port);
		}
*/
	}
	else{
		//fprintf(stderr, "DMR TX stopped\n");
		get_eot();
		build_frame();
		m_ttscnt = 0;
		txdata.append((char *)m_dmrFrame, 55);
		m_udp->writeDatagram(txdata, m_address, m_modeinfo.port);
		m_txtimer->stop();

		if(m_ttsid == 0){
			m_audio->stop_capture();
		}

		m_modeinfo.stream_state = STREAM_IDLE;
	}
	emit update_output_level(m_audio->level() * 8);
	emit update(m_modeinfo);
#ifdef DEBUG
	fprintf(stderr, "SEND:%d: ", txdata.size());
	for(int i = 0; i < txdata.size(); ++i){
		fprintf(stderr, "%02x ", (uint8_t)txdata.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
}

uint8_t * DMR::get_eot()
{
	encode_header(DT_TERMINATOR_WITH_LC);
	m_dmrcnt = 0;
	return m_dmrFrame;
}

void DMR::build_frame()
{
	//qDebug() << "DMR: slot:cc == " << m_txslot << ":" << m_txcc;
	m_dmrFrame[0U]  = 'D';
	m_dmrFrame[1U]  = 'M';
	m_dmrFrame[2U]  = 'R';
	m_dmrFrame[3U]  = 'D';

	m_dmrFrame[5U]  = m_txsrcid >> 16;
	m_dmrFrame[6U]  = m_txsrcid >> 8;
	m_dmrFrame[7U]  = m_txsrcid >> 0;
	m_dmrFrame[8U]  = m_txdstid >> 16;
	m_dmrFrame[9U]  = m_txdstid >> 8;
	m_dmrFrame[10U] = m_txdstid >> 0;
	m_dmrFrame[11U]  = m_essid >> 24;
	m_dmrFrame[12U]  = m_essid >> 16;
	m_dmrFrame[13U]  = m_essid >> 8;
	m_dmrFrame[14U]  = m_essid >> 0;

	m_dmrFrame[15U] = (m_txslot == 1U) ? 0x00U : 0x80U;
	m_dmrFrame[15U] |= (m_flco == FLCO_GROUP) ? 0x00U : 0x40U;

	if (m_dataType == DT_VOICE_SYNC) {
		m_dmrFrame[15U] |= 0x10U;
	} else if (m_dataType == DT_VOICE) {
		m_dmrFrame[15U] |= ((m_dmrcnt - 1) % 6U);
	} else {
		m_dmrFrame[15U] |= (0x20U | m_dataType);
	}

	m_dmrFrame[4U] = m_dmrcnt;
	::memcpy(m_dmrFrame + 16U, &m_txstreamid, 4U);

	m_dmrFrame[53U] = 0; //data.getBER();
	m_dmrFrame[54U] = 0; //data.getRSSI();

	m_modeinfo.srcid = m_txsrcid;
	m_modeinfo.dstid = m_txdstid;
	m_modeinfo.gwid = m_essid;
	m_modeinfo.frame_number = m_dmrcnt;
}

void DMR::encode_header(uint8_t t)
{
	addDMRDataSync(m_dmrFrame+20, 0);
	m_dataType = t;
	full_lc_encode(m_dmrFrame+20, t);
}

void DMR::encode_data()
{
	uint32_t n_dmr = (m_dmrcnt - 1) % 6U;

	if (!n_dmr) {
		m_dataType = DT_VOICE_SYNC;
		addDMRAudioSync(m_dmrFrame+20, 0);
		encode_embedded_data();
	}
	else {
		m_dataType = DT_VOICE;
		uint8_t lcss = get_embedded_data(m_dmrFrame+20, n_dmr);
		get_emb_data(m_dmrFrame+20, lcss);
	}
}

void DMR::encode16114(bool* d)
{
	d[11] = d[0] ^ d[1] ^ d[2] ^ d[3] ^ d[5] ^ d[7] ^ d[8];
	d[12] = d[1] ^ d[2] ^ d[3] ^ d[4] ^ d[6] ^ d[8] ^ d[9];
	d[13] = d[2] ^ d[3] ^ d[4] ^ d[5] ^ d[7] ^ d[9] ^ d[10];
	d[14] = d[0] ^ d[1] ^ d[2] ^ d[4] ^ d[6] ^ d[7] ^ d[10];
	d[15] = d[0] ^ d[2] ^ d[5] ^ d[6] ^ d[8] ^ d[9] ^ d[10];
}

void DMR::encode_qr1676(uint8_t* data)
{
	uint32_t value = (data[0U] >> 1) & 0x7FU;
	uint32_t cksum = ENCODING_TABLE_1676[value];

	data[0U] = cksum >> 8;
	data[1U] = cksum & 0xFFU;
}

void DMR::get_emb_data(uint8_t* data, uint8_t lcss)
{
	uint8_t DMREMB[2U];
	DMREMB[0U]  = (m_modeinfo.cc << 4) & 0xF0U;
	//DMREMB[0U] |= m_PI ? 0x08U : 0x00U;
	DMREMB[0U] |= (lcss << 1) & 0x06U;
	DMREMB[1U]  = 0x00U;

	encode_qr1676(DMREMB);

	data[13U] = (data[13U] & 0xF0U) | ((DMREMB[0U] >> 4U) & 0x0FU);
	data[14U] = (data[14U] & 0x0FU) | ((DMREMB[0U] << 4U) & 0xF0U);
	data[18U] = (data[18U] & 0xF0U) | ((DMREMB[1U] >> 4U) & 0x0FU);
	data[19U] = (data[19U] & 0x0FU) | ((DMREMB[1U] << 4U) & 0xF0U);
}

uint8_t DMR::get_embedded_data(uint8_t* data, uint8_t n)
{
	if (n >= 1U && n < 5U) {
		n--;

		bool bits[40U];
		::memset(bits, 0x00U, 40U * sizeof(bool));
		::memcpy(bits + 4U, m_raw + n * 32U, 32U * sizeof(bool));

		uint8_t bytes[5U];
		bitsToByteBE(bits + 0U,  bytes[0U]);
		bitsToByteBE(bits + 8U,  bytes[1U]);
		bitsToByteBE(bits + 16U, bytes[2U]);
		bitsToByteBE(bits + 24U, bytes[3U]);
		bitsToByteBE(bits + 32U, bytes[4U]);

		data[14U] = (data[14U] & 0xF0U) | (bytes[0U] & 0x0FU);
		data[15U] = bytes[1U];
		data[16U] = bytes[2U];
		data[17U] = bytes[3U];
		data[18U] = (data[18U] & 0x0FU) | (bytes[4U] & 0xF0U);

		switch (n) {
		case 0U:
			return 1U;
		case 3U:
			return 2U;
		default:
			return 3U;
		}
	} else {
		data[14U] &= 0xF0U;
		data[15U]  = 0x00U;
		data[16U]  = 0x00U;
		data[17U]  = 0x00U;
		data[18U] &= 0x0FU;

		return 0U;
	}
}

void DMR::encode_embedded_data()
{
	uint32_t crc;
	lc_get_data(m_data);
	CCRC::encodeFiveBit(m_data, crc);

	bool data[128U];
	::memset(data, 0x00U, 128U * sizeof(bool));

	data[106U] = (crc & 0x01U) == 0x01U;
	data[90U]  = (crc & 0x02U) == 0x02U;
	data[74U]  = (crc & 0x04U) == 0x04U;
	data[58U]  = (crc & 0x08U) == 0x08U;
	data[42U]  = (crc & 0x10U) == 0x10U;

	uint32_t b = 0U;
	for (uint32_t a = 0U; a < 11U; a++, b++)
		data[a] = m_data[b];
	for (uint32_t a = 16U; a < 27U; a++, b++)
		data[a] = m_data[b];
	for (uint32_t a = 32U; a < 42U; a++, b++)
		data[a] = m_data[b];
	for (uint32_t a = 48U; a < 58U; a++, b++)
		data[a] = m_data[b];
	for (uint32_t a = 64U; a < 74U; a++, b++)
		data[a] = m_data[b];
	for (uint32_t a = 80U; a < 90U; a++, b++)
		data[a] = m_data[b];
	for (uint32_t a = 96U; a < 106U; a++, b++)
		data[a] = m_data[b];

	// Hamming (16,11,4) check each row except the last one
	for (uint32_t a = 0U; a < 112U; a += 16U)
		encode16114(data + a);

	// Add the parity bits for each column
	for (uint32_t a = 0U; a < 16U; a++)
		data[a + 112U] = data[a + 0U] ^ data[a + 16U] ^ data[a + 32U] ^ data[a + 48U] ^ data[a + 64U] ^ data[a + 80U] ^ data[a + 96U];

	// The data is packed downwards in columns
	b = 0U;
	for (uint32_t a = 0U; a < 128U; a++) {
		m_raw[a] = data[b];
		b += 16U;
		if (b > 127U)
			b -= 127U;
	}
}

void DMR::bitsToByteBE(const bool* bits, uint8_t& byte)
{
	byte  = bits[0U] ? 0x80U : 0x00U;
	byte |= bits[1U] ? 0x40U : 0x00U;
	byte |= bits[2U] ? 0x20U : 0x00U;
	byte |= bits[3U] ? 0x10U : 0x00U;
	byte |= bits[4U] ? 0x08U : 0x00U;
	byte |= bits[5U] ? 0x04U : 0x00U;
	byte |= bits[6U] ? 0x02U : 0x00U;
	byte |= bits[7U] ? 0x01U : 0x00U;
}

void DMR::byteToBitsBE(uint8_t byte, bool* bits)
{
	bits[0U] = (byte & 0x80U) == 0x80U;
	bits[1U] = (byte & 0x40U) == 0x40U;
	bits[2U] = (byte & 0x20U) == 0x20U;
	bits[3U] = (byte & 0x10U) == 0x10U;
	bits[4U] = (byte & 0x08U) == 0x08U;
	bits[5U] = (byte & 0x04U) == 0x04U;
	bits[6U] = (byte & 0x02U) == 0x02U;
	bits[7U] = (byte & 0x01U) == 0x01U;
}

void DMR::lc_get_data(bool* bits)
{
	uint8_t bytes[9U];
	memset(bytes, 0, 9);
	lc_get_data(bytes);

	byteToBitsBE(bytes[0U], bits + 0U);
	byteToBitsBE(bytes[1U], bits + 8U);
	byteToBitsBE(bytes[2U], bits + 16U);
	byteToBitsBE(bytes[3U], bits + 24U);
	byteToBitsBE(bytes[4U], bits + 32U);
	byteToBitsBE(bytes[5U], bits + 40U);
	byteToBitsBE(bytes[6U], bits + 48U);
	byteToBitsBE(bytes[7U], bits + 56U);
	byteToBitsBE(bytes[8U], bits + 64U);
}

void DMR::lc_get_data(uint8_t *bytes)
{
	bool pf, r;
	uint8_t fid, options;
	bytes[0U] = (uint8_t)m_flco;

	pf = (bytes[0U] & 0x80U) == 0x80U;
	r  = (bytes[0U] & 0x40U) == 0x40U;
	//m_flco = FLCO(bytes[0U] & 0x3FU);
	fid = bytes[1U];
	options = bytes[2U];

	//bytes[0U] = (uint8_t)m_flco;

	if (pf)
		bytes[0U] |= 0x80U;

	if (r)
		bytes[0U] |= 0x40U;

	bytes[1U] = fid;
	bytes[2U] = options;
	bytes[3U] = m_txdstid >> 16;
	bytes[4U] = m_txdstid >> 8;
	bytes[5U] = m_txdstid >> 0;
	bytes[6U] = m_dmrid >> 16;
	bytes[7U] = m_dmrid >> 8;
	bytes[8U] = m_dmrid >> 0;
}

void DMR::full_lc_encode(uint8_t* data, uint8_t type)  // for header
{
	uint8_t lcData[12U];
	uint8_t parity[4U];
	::memset(lcData, 0, sizeof(lcData));
	lc_get_data(lcData);

	CRS129::encode(lcData, 9U, parity);

	switch (type) {
		case DT_VOICE_LC_HEADER:
			lcData[9U]  = parity[2U] ^ VOICE_LC_HEADER_CRC_MASK[0U];
			lcData[10U] = parity[1U] ^ VOICE_LC_HEADER_CRC_MASK[1U];
			lcData[11U] = parity[0U] ^ VOICE_LC_HEADER_CRC_MASK[2U];
			break;

		case DT_TERMINATOR_WITH_LC:
			lcData[9U]  = parity[2U] ^ TERMINATOR_WITH_LC_CRC_MASK[0U];
			lcData[10U] = parity[1U] ^ TERMINATOR_WITH_LC_CRC_MASK[1U];
			lcData[11U] = parity[0U] ^ TERMINATOR_WITH_LC_CRC_MASK[2U];
			break;

		default:
			return;
	}
	get_slot_data(data);
	m_bptc.encode(lcData, data);
}

void DMR::get_slot_data(uint8_t* data)
{
	uint8_t DMRSlotType[3U];
	DMRSlotType[0U]  = (m_modeinfo.cc << 4) & 0xF0U;
	DMRSlotType[0U] |= (m_dataType  << 0) & 0x0FU;
	DMRSlotType[1U]  = 0x00U;
	DMRSlotType[2U]  = 0x00U;

	CGolay2087::encode(DMRSlotType);

	data[12U] = (data[12U] & 0xC0U) | ((DMRSlotType[0U] >> 2) & 0x3FU);
	data[13U] = (data[13U] & 0x0FU) | ((DMRSlotType[0U] << 6) & 0xC0U) | ((DMRSlotType[1U] >> 2) & 0x30U);
	data[19U] = (data[19U] & 0xF0U) | ((DMRSlotType[1U] >> 2) & 0x0FU);
	data[20U] = (data[20U] & 0x03U) | ((DMRSlotType[1U] << 6) & 0xC0U) | ((DMRSlotType[2U] >> 2) & 0x3CU);
}


void DMR::addDMRDataSync(uint8_t* data, bool duplex)
{
	if (duplex) {
		for (uint32_t i = 0U; i < 7U; i++)
			data[i + 13U] = (data[i + 13U] & ~SYNC_MASK[i]) | BS_SOURCED_DATA_SYNC[i];
	} else {
		for (uint32_t i = 0U; i < 7U; i++)
			data[i + 13U] = (data[i + 13U] & ~SYNC_MASK[i]) | MS_SOURCED_DATA_SYNC[i];
	}
}

void DMR::addDMRAudioSync(uint8_t* data, bool duplex)
{
	if (duplex) {
		for (uint32_t i = 0U; i < 7U; i++)
			data[i + 13U] = (data[i + 13U] & ~SYNC_MASK[i]) | BS_SOURCED_AUDIO_SYNC[i];
	} else {
		for (uint32_t i = 0U; i < 7U; i++)
			data[i + 13U] = (data[i + 13U] & ~SYNC_MASK[i]) | MS_SOURCED_AUDIO_SYNC[i];
	}
}

void DMR::get_ambe()
{
#if !defined(Q_OS_IOS)
	uint8_t ambe[9];

	if(m_ambedev->get_ambe(ambe)){
		for(int i = 0; i < 9; ++i){
			m_txcodecq.append(ambe[i]);
		}
	}
#endif
}

void DMR::process_rx_data()
{
	int16_t pcm[160];
	uint8_t ambe[9];
	static uint8_t cnt = 0;

	if(m_rxwatchdog++ > 100){
		qDebug() << "DMR RX stream timeout ";
		m_rxwatchdog = 0;
		m_modeinfo.stream_state = STREAM_LOST;
		m_modeinfo.ts = QDateTime::currentMSecsSinceEpoch();
		emit update(m_modeinfo);
		m_modeinfo.streamid = 0;
	}

	if((m_rxmodemq.size() > 2) && (++cnt >= 3)){
		QByteArray out;
		int s = m_rxmodemq[1];
		if((m_rxmodemq[0] == MMDVM_FRAME_START) && (m_rxmodemq.size() >= s)){
			for(int i = 0; i < s; ++i){
				out.append(m_rxmodemq.dequeue());
			}
#if !defined(Q_OS_IOS)
			m_modem->write(out);
#endif
		}
		cnt = 0;
	}

	if((!m_tx) && (m_rxcodecq.size() > 8) ){
		for(int i = 0; i < 9; ++i){
			ambe[i] = m_rxcodecq.dequeue();
		}
		if(m_hwrx){
#if !defined(Q_OS_IOS)
			m_ambedev->decode(ambe);

			if(m_ambedev->get_audio(pcm)){
				m_audio->write(pcm, 160);
				emit update_output_level(m_audio->level());
			}
#endif
		}
		else{
			if(m_modeinfo.sw_vocoder_loaded){
				m_mbevocoder->decode_2450x1150(pcm, ambe);
			}
			else{
				memset(pcm, 0, 160 * sizeof(int16_t));
			}
			m_audio->write(pcm, 160);
			emit update_output_level(m_audio->level());
		}
	}
	else if ( ((m_modeinfo.stream_state == STREAM_END) || (m_modeinfo.stream_state == STREAM_LOST)) && (m_rxmodemq.size() < 50) ){
		m_rxtimer->stop();
		m_audio->stop_playback();
		m_rxwatchdog = 0;
		m_modeinfo.streamid = 0;
		m_rxcodecq.clear();
		qDebug() << "DMR playback stopped";
		m_modeinfo.stream_state = STREAM_IDLE;
		return;
	}
}
