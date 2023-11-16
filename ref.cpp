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
#include "ref.h"
#include "CRCenc.h"


const uint8_t MMDVM_DSTAR_HEADER = 0x10U;
const uint8_t MMDVM_DSTAR_DATA   = 0x11U;
const uint8_t MMDVM_DSTAR_LOST   = 0x12U;
const uint8_t MMDVM_DSTAR_EOT    = 0x13U;

REF::REF()
{
    m_mode = "REF";
	m_attenuation = 5;
}

REF::~REF()
{
}

void REF::process_udp()
{
	QByteArray buf;
	QByteArray out;
	QHostAddress sender;
	quint16 senderPort;
	static bool sd_sync = 0;
    static int sd_txt_seq = 0;
    static int sd_gps_cnt = 0;
    static int sd_hdr_cnt = 0;
    static int sd_debug_cnt = 0;
	static char user_data[21];
    static QByteArray gps_data;
    static uint8_t debug_data[64];

	const uint8_t header[5] = {0x80,0x44,0x53,0x56,0x54};

	buf.resize(m_udp->pendingDatagramSize());
	m_udp->readDatagram(buf.data(), buf.size(), &sender, &senderPort);

    if(m_debug){
        QDebug debug = qDebug();
        debug.noquote();
        QString s = "RECV:";
        for(int i = 0; i < buf.size(); ++i){
            s += " " + QString("%1").arg((uint8_t)buf.data()[i], 2, 16, QChar('0'));
        }
        debug << s;
    }

	if ((buf.size() == 5) && (buf.data()[0] == 5)){
		int x = (::rand() % (999999 - 7245 + 1)) + 7245;
		QString serial = "HS" + QString("%1").arg(x, 6, 10, QChar('0'));
		out.append(0x1c);
		out.append(0xc0);
		out.append(0x04);
		out.append('\x00');
		out.append(m_modeinfo.callsign.toUpper().toLocal8Bit().data(), 6);
		out.append(10,'\x00');
		out.append(serial.toUtf8());
		m_udp->writeDatagram(out, m_address, 20001);
	}
	if(buf.size() == 3){ //2 way keep alive ping
		m_modeinfo.count++;
		if( (m_modeinfo.stream_state == STREAM_LOST) || (m_modeinfo.stream_state == STREAM_END) ){
			m_modeinfo.stream_state = STREAM_IDLE;
		}
		emit update(m_modeinfo);
	}

    if(m_debug && out.size() > 0){
        QDebug debug = qDebug();
        debug.noquote();
        QString s = "RECV:";
        for(int i = 0; i < out.size(); ++i){
            s += " " + QString("%1").arg((uint8_t)out.data()[i], 2, 16, QChar('0'));
        }
        debug << s;
    }

	if((m_modeinfo.status == CONNECTING) && (buf.size() == 0x08)){
		if((memcmp(&buf.data()[4], "OKRW", 4) == 0) || (memcmp(&buf.data()[4], "OKRO", 4) == 0) || (memcmp(&buf.data()[4], "BUSY", 4) == 0)){
			m_modeinfo.sw_vocoder_loaded = load_vocoder_plugin();
			m_rxtimer = new QTimer();
			connect(m_rxtimer, SIGNAL(timeout()), this, SLOT(process_rx_data()));
			m_txtimer = new QTimer();
			connect(m_txtimer, SIGNAL(timeout()), this, SLOT(transmit()));
			m_ping_timer = new QTimer();
			connect(m_ping_timer, SIGNAL(timeout()), this, SLOT(send_ping()));
			m_ping_timer->start(1000);
			m_audio = new AudioEngine(m_audioin, m_audioout);
			m_audio->init();

			if(buf.data()[7] == 0x57){ //OKRW
				m_modeinfo.status = CONNECTED_RW;
				//memset(m_rptr2, ' ', 8);
				//memcpy(rptr2, hostname.toLocal8Bit(), hostname.size());
				//rptr2[7] = module;
				//rptr2[8] = 0;
				//m_ping_timer->start(1000);
			}
			else if(buf.data()[7] == 0x4f){ //OKRO -- Go get registered!
				m_modeinfo.status = CONNECTED_RW;
			}
		}
		else if((buf.data()[4] == 0x46) && (buf.data()[5] == 0x41) && (buf.data()[6] == 0x49) && (buf.data()[7] == 0x4c)){ // FAIL response
			m_modeinfo.status = DISCONNECTED;
		}
		else{ //Unknown response
			m_modeinfo.status = DISCONNECTED;
		}
		emit update(m_modeinfo);
	}
	if(m_modeinfo.status != CONNECTED_RW) return;

	if((buf.size() == 0x3a) && (!memcmp(buf.data()+1, header, 5)) ){
		char temp[9];
		memcpy(temp, buf.data() + 20, 8); temp[8] = '\0';
		QString rptr2 = QString(temp);
		memcpy(temp, buf.data() + 28, 8); temp[8] = '\0';
		QString rptr1 = QString(temp);
		memcpy(temp, buf.data() + 36, 8); temp[8] = '\0';
		QString urcall = QString(temp);
		memcpy(temp, buf.data() + 44, 8); temp[8] = '\0';
		QString mycall = QString(temp);
		QString h = m_refname + " " + m_module;

		if( (rptr2.simplified() == h.simplified()) || (rptr1.simplified() == h.simplified()) ){
			m_rxwatchdog = 0;
			const uint16_t streamid = (buf.data()[14] << 8) | (buf.data()[15] & 0xff);
			m_modeinfo.src = mycall;
			m_modeinfo.dst = urcall;
			m_modeinfo.gw = rptr1;
			m_modeinfo.gw2 = rptr2;

			if(!m_tx && !m_rxtimer->isActive() && (m_modeinfo.streamid == 0)){
				m_audio->start_playback();
				m_rxtimer->start(m_rxtimerint);
				m_rxcodecq.clear();
				m_modeinfo.stream_state = STREAM_NEW;
				m_modeinfo.streamid = streamid;

				if(m_modem){
					uint8_t out[44];
					out[0] = 0xe0;
					out[1] = 44;
					out[2] = MMDVM_DSTAR_HEADER;
					out[3] = 0x40;
					out[4] = 0;
					out[5] = 0;
					memcpy(out + 6, rptr2.toLocal8Bit().data(), 8);
					memcpy(out + 14, rptr1.toLocal8Bit().data(), 8);
					memcpy(out + 22, urcall.toLocal8Bit().data(), 8);
					memcpy(out + 30, mycall.toLocal8Bit().data(), 8);
					memcpy(out + 38, buf.data() + 52, 4);
					CCRC::addCCITT161((uint8_t *)out + 3, 41);
					for(int i = 0; i < 44; ++i){
						m_rxmodemq.append(out[i]);
					}
					//m_modem->write(out);
				}

				qDebug() << "New stream from " << m_modeinfo.src << " to " << m_modeinfo.dst << " id == " << QString::number(m_modeinfo.streamid, 16);
				emit update(m_modeinfo);
                sd_gps_cnt = 0;
                gps_data.clear();
			}
		}
		else{
			//streamid = 0;
		}
	}
	if((buf.size() == 0x1d) && (!memcmp(buf.data()+1, header, 5)) ){ //29
		const uint16_t streamid = (buf.data()[14] << 8) | (buf.data()[15] & 0xff);
		if(streamid != m_modeinfo.streamid){
			return;
		}
		m_rxwatchdog = 0;
		m_modeinfo.stream_state = STREAMING;
		m_modeinfo.frame_number = (uint8_t)buf.data()[16];

		if(m_modem){
			m_rxmodemq.append(0xe0);
			m_rxmodemq.append(15);
			m_rxmodemq.append(MMDVM_DSTAR_DATA);
			for(int i = 0; i < 12; ++i){
				m_rxmodemq.append(buf.data()[17+i]);
			}
		}
		if((buf.data()[16] == 0) && (buf.data()[26] == 0x55) && (buf.data()[27] == 0x2d) && (buf.data()[28] == 0x16)){
			sd_sync = 1;
            sd_txt_seq = 1;
            //sd_debug_cnt = 0;
            //for(int i = 0; i < 63; i++){
            //    fprintf(stderr, "%02x ", debug_data[i]);
            //}
           // fprintf(stderr, "\n");
		}
        //debug_data[sd_debug_cnt++] = buf.data()[26] ^ 0x70;
        //debug_data[sd_debug_cnt++] = buf.data()[27] ^ 0x4f;
        //debug_data[sd_debug_cnt++] = buf.data()[28] ^ 0x93;

        char c = buf.data()[26] ^ 0x70;

        if( sd_sync && !sd_gps_cnt && ((c & 0xf0) == 0x50)){
            sd_hdr_cnt = (c & 0x0f);
        }
        else if(sd_hdr_cnt){
            c = 0;
            sd_hdr_cnt = 0;
        }
        if( sd_sync && !sd_gps_cnt && ((c & 0xf0) == 0x30)){
            sd_gps_cnt = (c & 0x0f);
            c = buf.data()[27] ^ 0x4f;
            if( (c == 0x0a) || (c == 0x0d) ){
                gps_data.append('\0');
                sd_gps_cnt = 0;
                QTextStream(stderr) << "GPS: " << QString(gps_data) << Qt::endl;
                if(gps_data[0] == '$') emit update_log("GPS: " + QString(gps_data));
                gps_data.clear();
            }
            else{
                sd_gps_cnt--;
                gps_data.append(c);
                c = buf.data()[28] ^ 0x93;
                if( (c == 0x0a) || (c == 0x0d) ){
                    gps_data.append('\0');
                    sd_gps_cnt = 0;
                    QTextStream(stderr) << "GPS: " << QString(gps_data) << Qt::endl;
                    if(gps_data[0] == '$') emit update_log("GPS: " + QString(gps_data));
                    gps_data.clear();
                }
                else{
                    sd_gps_cnt--;
                    gps_data.append(c);
                }
            }
        }
        else if(sd_gps_cnt && (buf.data()[16] != 0)){
            if( (c == 0x0a) || (c == 0x0d) ){
                gps_data.append('\0');
                sd_gps_cnt = 0;
                QTextStream(stderr) << "GPS: " << QString(gps_data) << Qt::endl;
                if(gps_data[0] == '$') emit update_log("GPS: " + QString(gps_data));
                gps_data.clear();
            }
            else{
                gps_data.append(c);
                c = buf.data()[27] ^ 0x4f;
                if( (c == 0x0a) || (c == 0x0d) ){
                    gps_data.append('\0');
                    sd_gps_cnt = 0;
                    QTextStream(stderr) << "GPS: " << QString(gps_data) << Qt::endl;
                    if(gps_data[0] == '$') emit update_log("GPS: " + QString(gps_data));
                    gps_data.clear();
                }
                else{
                    gps_data.append(c);
                    c = buf.data()[28] ^ 0x93;
                    if( (c == 0x0a) || (c == 0x0d) ){
                        gps_data.append('\0');
                        sd_gps_cnt = 0;
                        QTextStream(stderr) << "GPS: " << QString(gps_data) << Qt::endl;
                        if(gps_data[0] == '$') emit update_log("GPS: " + QString(gps_data));
                        gps_data.clear();
                    }
                    else{
                        gps_data.append(c);
                    }
                }
            }
            sd_gps_cnt = 0;
        }
        if(sd_sync && (sd_txt_seq == 1) && (buf.data()[16] == 1) && (buf.data()[26] == 0x30)){
		   user_data[0] = buf.data()[27] ^ 0x4f;
		   user_data[1] = buf.data()[28] ^ 0x93;
           ++sd_txt_seq;
		}
        if(sd_sync && (sd_txt_seq == 2) && (buf.data()[16] == 2)){
		   user_data[2] = buf.data()[26] ^ 0x70;
		   user_data[3] = buf.data()[27] ^ 0x4f;
		   user_data[4] = buf.data()[28] ^ 0x93;
           ++sd_txt_seq;
        }
        if(sd_sync && (sd_txt_seq == 3) && ((buf.data()[16] == 3) || (buf.data()[16] == 5)) && (buf.data()[26] == 0x31)){
		   user_data[5] = buf.data()[27] ^ 0x4f;
		   user_data[6] = buf.data()[28] ^ 0x93;
           ++sd_txt_seq;
		}
        if(sd_sync && (sd_txt_seq == 4) && ((buf.data()[16] == 4) || (buf.data()[16] == 6))){
		   user_data[7] = buf.data()[26] ^ 0x70;
		   user_data[8] = buf.data()[27] ^ 0x4f;
		   user_data[9] = buf.data()[28] ^ 0x93;
           ++sd_txt_seq;
		}
        if(sd_sync && (sd_txt_seq == 5) && ((buf.data()[16] == 5) || (buf.data()[16] == 9)) && (buf.data()[26] == 0x32)){
		   user_data[10] = buf.data()[27] ^ 0x4f;
		   user_data[11] = buf.data()[28] ^ 0x93;
           ++sd_txt_seq;
		}
        if(sd_sync && (sd_txt_seq == 6) && ((buf.data()[16] == 6) || (buf.data()[16] == 10))){
		   user_data[12] = buf.data()[26] ^ 0x70;
		   user_data[13] = buf.data()[27] ^ 0x4f;
		   user_data[14] = buf.data()[28] ^ 0x93;
           ++sd_txt_seq;
		}
        if(sd_sync && (sd_txt_seq == 7) && ((buf.data()[16] == 7) || (buf.data()[16] == 13)) && (buf.data()[26] == 0x33)){
		   user_data[15] = buf.data()[27] ^ 0x4f;
		   user_data[16] = buf.data()[28] ^ 0x93;
           ++sd_txt_seq;
		}
        if(sd_sync && (sd_txt_seq == 8) && ((buf.data()[16] == 8) || (buf.data()[16] == 14))){
		   user_data[17] = buf.data()[26] ^ 0x70;
		   user_data[18] = buf.data()[27] ^ 0x4f;
		   user_data[19] = buf.data()[28] ^ 0x93;
           user_data[20] = '\0';
           sd_txt_seq = 0;
		   m_modeinfo.usertxt = QString(user_data);
		}
		for(int i = 0; i < 9; ++i){
			m_rxcodecq.append(buf.data()[17+i]);
		}
		emit update(m_modeinfo);
	}
	if(buf.size() == 0x20){ //32
		const uint16_t streamid = (buf.data()[14] << 8) | (buf.data()[15] & 0xff);
		if(streamid == m_modeinfo.streamid){
			if(m_modem){
				m_rxmodemq.append(0xe0);
				m_rxmodemq.append(3);
				m_rxmodemq.append(MMDVM_DSTAR_EOT);
			}
			m_modeinfo.usertxt.clear();
			qDebug() << "REF RX stream ended ";
			m_rxwatchdog = 0;
			m_modeinfo.stream_state = STREAM_END;
			m_modeinfo.ts = QDateTime::currentMSecsSinceEpoch();
			emit update(m_modeinfo);
			m_modeinfo.streamid = 0;
            sd_sync = 0;
            sd_gps_cnt = 0;
		}
	}
	//emit update(m_modeinfo);
}

void REF::hostname_lookup(QHostInfo i)
{
	if (!i.addresses().isEmpty()) {
		QByteArray out;
		out.append(0x05);
		out.append('\x00');
		out.append(0x18);
		out.append('\x00');
		out.append(0x01);
		m_address = i.addresses().first();
		m_udp = new QUdpSocket(this);
		connect(m_udp, SIGNAL(readyRead()), this, SLOT(process_udp()));
		m_udp->writeDatagram(out, m_address, m_modeinfo.port);

        if(m_debug){
            QDebug debug = qDebug();
            debug.noquote();
            QString s = "CONN:";
            for(int i = 0; i < out.size(); ++i){
                s += " " + QString("%1").arg((uint8_t)out.data()[i], 2, 16, QChar('0'));
            }
            debug << s;
        }
	}
}

void REF::send_ping()
{
	QByteArray out;
	out.append(0x03);
	out.append(0x60);
	out.append('\x00');
	m_udp->writeDatagram(out, m_address, m_modeinfo.port);

    if(m_debug){
        QDebug debug = qDebug();
        debug.noquote();
        QString s = "PING:";
        for(int i = 0; i < out.size(); ++i){
            s += " " + QString("%1").arg((uint8_t)out.data()[i], 2, 16, QChar('0'));
        }
        debug << s;
    }
}

void REF::send_disconnect()
{
	QByteArray out;
	out.append(0x05);
	out.append('\x00');
	out.append(0x18);
	out.append('\x00');
	out.append('\x00');
	m_udp->writeDatagram(out, m_address, m_modeinfo.port);

    if(m_debug){
        QDebug debug = qDebug();
        debug.noquote();
        QString s = "DISC:";
        for(int i = 0; i < out.size(); ++i){
            s += " " + QString("%1").arg((uint8_t)out.data()[i], 2, 16, QChar('0'));
        }
        debug << s;
    }
}

void REF::format_callsign(QString &s)
{
	QStringList l = s.simplified().split(' ');

	if(l.size() > 1){
		s = l.at(0).simplified();
		while(s.size() < 7){
			s.append(' ');
		}
		s += l.at(1).simplified();
	}
	else{
		while(s.size() < 8){
			s.append(' ');
		}
	}
}

void REF::process_modem_data(QByteArray d)
{
	QByteArray txdata;
	char cs[9];
	uint8_t ambe[9];

	uint8_t *p_frame = (uint8_t *)(d.data());
	if(p_frame[2] == MMDVM_DSTAR_HEADER){
		format_callsign(m_txrptr1);
		format_callsign(m_txrptr2);
		cs[8] = 0;
		memcpy(cs, p_frame + 22, 8);
		m_txurcall = QString(cs);
		memcpy(cs, p_frame + 30, 8);
		m_txmycall = QString(cs);
		m_modeinfo.stream_state = TRANSMITTING_MODEM;
		m_tx = true;
	}
	else if( (p_frame[2] == MMDVM_DSTAR_EOT) || (p_frame[2] == MMDVM_DSTAR_LOST) ){
		m_tx = false;
	}
	else if(p_frame[2] == MMDVM_DSTAR_DATA){
		memcpy(ambe, p_frame + 3, 9);
	}
	send_frame(ambe);
}

void REF::toggle_tx(bool tx)
{
	tx ? start_tx() : stop_tx();
}

void REF::start_tx()
{
	format_callsign(m_txmycall);
	format_callsign(m_txurcall);
	format_callsign(m_txrptr1);
	format_callsign(m_txrptr2);
	Mode::start_tx();
}

void REF::transmit()
{
	uint8_t ambe[9];
	uint8_t ambe_frame[72];
	int16_t pcm[160];
	memset(ambe_frame, 0, 72);
	memset(ambe, 0, 9);
#ifdef USE_FLITE
	if(m_ttsid > 0){
		for(int i = 0; i < 160; ++i){
			if(m_ttscnt >= tts_audio->num_samples/2){
				//audiotx_cnt = 0;
				pcm[i] = 0;
			}
			else{
				pcm[i] = tts_audio->samples[m_ttscnt*2] / 8;
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
		if(m_tx && (m_txcodecq.size() >= 9)){
			for(int i = 0; i < 9; ++i){
				ambe[i] = m_txcodecq.dequeue();
			}
			send_frame(ambe);
		}
		else if(!m_tx){
			send_frame(ambe);
		}
	}
	else{
		if(m_modeinfo.sw_vocoder_loaded){
			m_mbevocoder->encode_2400x1200(pcm, ambe);
		}
		send_frame(ambe);
	}
}

void REF::send_frame(uint8_t *ambe)
{
	QByteArray txdata;
	static uint16_t txstreamid = 0;
	static bool sendheader = 1;

	if(txstreamid == 0){
		txstreamid = static_cast<uint16_t>((::rand() & 0xFFFF));
	}
	if(sendheader){
		sendheader = 0;
		txdata.resize(58);
		txdata[0] = 0x3a;
		txdata[1] = 0x80;
		txdata[2] = 0x44;
		txdata[3] = 0x53;
		txdata[4] = 0x56;
		txdata[5] = 0x54;
		txdata[6] = 0x10;
		txdata[7] = 0x00;
		txdata[8] = 0x00;
		txdata[9] = 0x00;
		txdata[10] = 0x20;
		txdata[11] = 0x00;
		txdata[12] = 0x02;
		txdata[13] = 0x01;
		txdata[14] = (txstreamid >> 8) & 0xff;
		txdata[15] = txstreamid & 0xff;
		txdata[16] = 0x80;
		txdata[17] = 0x00;
		txdata[18] = 0x00;
		txdata[19] = 0x00;
		txdata.replace(20, 8, m_txrptr2.toLocal8Bit().data());
		txdata.replace(28, 8, m_txrptr1.toLocal8Bit().data());
		txdata.replace(36, 8, m_txurcall.toLocal8Bit().data());
		txdata.replace(44, 8, m_txmycall.toLocal8Bit().data());
		txdata.replace(52, 4, "AMBE");
		CCRC::addCCITT161((uint8_t *)txdata.data() + 17, 41);

		m_modeinfo.src = m_txmycall;
		m_modeinfo.dst = m_txurcall;
		m_modeinfo.gw = m_txrptr1;
		m_modeinfo.gw2 = m_txrptr2;
		m_modeinfo.streamid = txstreamid;
		m_modeinfo.frame_number = m_txcnt;

		m_udp->writeDatagram(txdata, m_address, m_modeinfo.port);

        if(m_debug){
            QDebug debug = qDebug();
            debug.noquote();
            QString s = "SEND:";
            for(int i = 0; i < txdata.size(); ++i){
                s += " " + QString("%1").arg((uint8_t)txdata.data()[i], 2, 16, QChar('0'));
            }
            debug << s;
        }
	}

	txdata.resize(29);
	txdata[0] = 0x1d;
	txdata[1] = 0x80;
	txdata[2] = 0x44;
	txdata[3] = 0x53;
	txdata[4] = 0x56;
	txdata[5] = 0x54;
	txdata[6] = 0x20;
	txdata[7] = 0x00;
	txdata[8] = 0x00;
	txdata[9] = 0x00;
	txdata[10] = 0x20;
	txdata[11] = 0x00;
	txdata[12] = 0x02;
	txdata[13] = 0x01;
	txdata[14] = (txstreamid >> 8) & 0xff;
	txdata[15] = txstreamid & 0xff;
	txdata[16] = m_txcnt;
	memcpy(txdata.data() + 17, ambe, 9);

	m_modeinfo.frame_number = m_txcnt;

	switch(txdata.data()[16]){
	case 0:
		txdata[26] = 0x55;
		txdata[27] = 0x2d;
		txdata[28] = 0x16;
		break;
	case 1:
		txdata[26] = 0x40 ^ 0x70;
		txdata[27] = m_txusrtxt.toLocal8Bit().data()[0] ^ 0x4f;
		txdata[28] = m_txusrtxt.toLocal8Bit().data()[1] ^ 0x93;
		break;
	case 2:
		txdata[26] = m_txusrtxt.toLocal8Bit().data()[2] ^ 0x70;
		txdata[27] = m_txusrtxt.toLocal8Bit().data()[3] ^ 0x4f;
		txdata[28] = m_txusrtxt.toLocal8Bit().data()[4] ^ 0x93;
		break;
	case 3:
		txdata[26] = 0x41 ^ 0x70;
		txdata[27] = m_txusrtxt.toLocal8Bit().data()[5] ^ 0x4f;
		txdata[28] = m_txusrtxt.toLocal8Bit().data()[6] ^ 0x93;
		break;
	case 4:
		txdata[26] = m_txusrtxt.toLocal8Bit().data()[7] ^ 0x70;
		txdata[27] = m_txusrtxt.toLocal8Bit().data()[8] ^ 0x4f;
		txdata[28] = m_txusrtxt.toLocal8Bit().data()[9] ^ 0x93;
		break;
	case 5:
		txdata[26] = 0x42 ^ 0x70;
		txdata[27] = m_txusrtxt.toLocal8Bit().data()[10] ^ 0x4f;
		txdata[28] = m_txusrtxt.toLocal8Bit().data()[11] ^ 0x93;
		break;
	case 6:
		txdata[26] = m_txusrtxt.toLocal8Bit().data()[12] ^ 0x70;
		txdata[27] = m_txusrtxt.toLocal8Bit().data()[13] ^ 0x4f;
		txdata[28] = m_txusrtxt.toLocal8Bit().data()[14] ^ 0x93;
		break;
	case 7:
		txdata[26] = 0x43 ^ 0x70;
		txdata[27] = m_txusrtxt.toLocal8Bit().data()[15] ^ 0x4f;
		txdata[28] = m_txusrtxt.toLocal8Bit().data()[16] ^ 0x93;
		break;
	case 8:
		txdata[26] = m_txusrtxt.toLocal8Bit().data()[17] ^ 0x70;
		txdata[27] = m_txusrtxt.toLocal8Bit().data()[18] ^ 0x4f;
		txdata[28] = m_txusrtxt.toLocal8Bit().data()[19] ^ 0x93;
		break;
	default:
		txdata[26] = 0x16;
		txdata[27] = 0x29;
		txdata[28] = 0xf5;
		break;
	}

	if(m_txcnt == 20){
		sendheader = 1;
		m_txcnt = 0;
	}
	else{
		++m_txcnt;
	}

	if(!m_tx){
		qDebug() << "TX stopped";
		txdata[0] = 0x20;
		//txdata[6] = 0x20;
		//txdata[16] = m_txcnt % 21;
		txdata[16] = 0x40 | txdata[16];
		memset(txdata.data() + 17, 0, 9);
		txdata[26] = 0x55;
		txdata[27] = 0x55;
		txdata[28] = 0x55;
		txdata.append(0x55);
		txdata.append(0xc8);
		txdata.append(0x7a);
		m_txcnt = 0;
		txstreamid = 0;
		m_modeinfo.streamid = 0;
		sendheader = 1;
		m_txtimer->stop();

		if((m_ttsid == 0) && (m_modeinfo.stream_state == TRANSMITTING) ){
			m_audio->stop_capture();
		}
		m_ttscnt = 0;
		m_modeinfo.stream_state = STREAM_IDLE;
	}

	m_udp->writeDatagram(txdata, m_address, m_modeinfo.port);
	emit update_output_level(m_audio->level() * 2);
	emit update(m_modeinfo);

    if(m_debug){
        QDebug debug = qDebug();
        debug.noquote();
        QString s = "SEND:";
        for(int i = 0; i < txdata.size(); ++i){
            s += " " + QString("%1").arg((uint8_t)txdata.data()[i], 2, 16, QChar('0'));
        }
        debug << s;
    }
}

void REF::get_ambe()
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

void REF::process_rx_data()
{
	int16_t pcm[160];
	uint8_t ambe[9];

	if(m_rxwatchdog++ > 50){
		qDebug() << "REF RX stream timeout ";
		m_rxwatchdog = 0;
		m_modeinfo.stream_state = STREAM_LOST;
		m_modeinfo.ts = QDateTime::currentMSecsSinceEpoch();
		emit update(m_modeinfo);
		m_modeinfo.streamid = 0;
	}

	if(m_rxmodemq.size() > 2){
		QByteArray out;
		int s = m_rxmodemq[1];
		if((m_rxmodemq[0] == 0xe0) && (m_rxmodemq.size() >= s)){
			for(int i = 0; i < s; ++i){
				out.append(m_rxmodemq.dequeue());
			}
#if !defined(Q_OS_IOS)
			m_modem->write(out);
#endif
		}
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
				m_mbevocoder->decode_2400x1200(pcm, ambe);
			}
			else{
				memset(pcm, 0, 160 * sizeof(int16_t));
			}
			m_audio->write(pcm, 160);
			emit update_output_level(m_audio->level());
		}
	}
	else if ( (m_modeinfo.stream_state == STREAM_END) || (m_modeinfo.stream_state == STREAM_LOST) ){
		m_rxtimer->stop();
		m_audio->stop_playback();
		m_rxwatchdog = 0;
		m_modeinfo.streamid = 0;
		m_rxcodecq.clear();
		qDebug() << "REF playback stopped";
		m_modeinfo.stream_state = STREAM_IDLE;
		return;
	}
}
