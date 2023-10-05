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

#include "iax.h"
#include "iaxdefines.h"
#ifdef Q_OS_WIN
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#ifdef USE_FLITE
extern "C" {
extern cst_voice * register_cmu_us_slt(const char *);
extern cst_voice * register_cmu_us_kal16(const char *);
extern cst_voice * register_cmu_us_awb(const char *);
}
#endif

#define DEBUG

IAX::IAX() :
	m_scallno(0),
	m_dcallno(0),
	m_regscallno(0x7fff),
	m_regdcallno(0),
	m_iseq(0),
	m_oseq(0),
	m_tx(false),
	m_rxjitter(0),
	m_rxloss(1),
	m_rxframes(0),
	m_rxdelay(0),
	m_rxdropped(0),
	m_rxooo(0),
	m_ttsid(0),
	m_cnt(0)
{
#ifdef USE_FLITE
	flite_init();
	voice_slt = register_cmu_us_slt(nullptr);
	voice_kal = register_cmu_us_kal16(nullptr);
	voice_awb = register_cmu_us_awb(nullptr);
#endif
}

IAX::~IAX()
{
}

void IAX::set_iax_params(QString username, QString password, QString node, QString host, int port)
{
	m_username = username;
	m_password = password;
	m_node = node;
	m_host = host;
	m_port = port;

	QStringList l = m_node.split('@');
	if(l.size() == 2){
		m_node = l.at(0).simplified();
		m_context = l.at(1).simplified();
	}
	else{
		m_context = "iax-client";
	}
}

int16_t ulaw_decode(int8_t number)
{
   const uint16_t MULAW_BIAS = 33;
   uint8_t sign = 0, position = 0;
   int16_t decoded = 0;
   number = ~number;
   if (number & 0x80)
   {
	  number &= ~(1 << 7);
	  sign = -1;
   }
   position = ((number & 0xF0) >> 4) + 5;
   decoded = ((1 << position) | ((number & 0x0F) << (position - 4))
			 | (1 << (position - 5))) - MULAW_BIAS;
   return (sign == 0) ? (decoded) : (-(decoded));
}

int8_t ulaw_encode(int16_t number)
{
   const uint16_t MULAW_MAX = 0x1FFF;
   const uint16_t MULAW_BIAS = 33;
   uint16_t mask = 0x1000;
   uint8_t sign = 0;
   uint8_t position = 12;
   uint8_t lsb = 0;
   if (number < 0)
   {
	  number = -number;
	  sign = 0x80;
   }
   number += MULAW_BIAS;
   if (number > MULAW_MAX)
   {
	  number = MULAW_MAX;
   }
   for (; ((number & mask) != mask && position >= 5); mask >>= 1, position--)
		;
   lsb = (number >> (position - 4)) & 0x0f;
   return (~(sign | ((position - 5) << 4) | lsb));
}

void IAX::send_call()
{
	uint16_t scall = htons(++m_scallno | 0x8000);
	m_oseq = m_iseq = 0;
	QByteArray out;
	out.append((char *)&scall, 2);
	out.append('\x00');
	out.append('\x00');
	out.append('\x00');
	out.append('\x00');
	out.append('\x00');
	out.append('\x00');
	out.append(m_oseq);
	out.append(m_iseq);
	out.append(AST_FRAME_IAX);
	out.append(IAX_COMMAND_NEW);
	out.append(IAX_IE_VERSION);
	out.append(sizeof(short));
	out.append('\x00');
	out.append(IAX_PROTO_VERSION);
	out.append(IAX_IE_CALLED_NUMBER);
	out.append(m_node.size());
	out.append(m_node.toUtf8(), m_node.size());
	out.append(IAX_IE_CALLING_NUMBER);
	out.append('\x00');
	out.append(IAX_IE_CALLING_NAME);
	out.append(m_callsign.size());
	out.append(m_callsign.toUtf8(), m_callsign.size());
	out.append(IAX_IE_USERNAME);
	out.append(m_username.size());
	out.append(m_username.toUtf8(), m_username.size());
	out.append(IAX_IE_FORMAT);
	out.append(sizeof(int));
	out.append('\x00');
	out.append('\x00');
	out.append('\x00');
	out.append(AST_FORMAT_ULAW);
	m_timestamp = QDateTime::currentMSecsSinceEpoch();
	m_udp->writeDatagram(out, m_address, m_port);

    if(m_debug){
        QDebug debug = qDebug();
        debug.noquote();
        QString s = "SEND:";
        for(int i = 0; i < out.size(); ++i){
            s += " " + QString("%1").arg((uint8_t)out.data()[i], 2, 16, QChar('0'));
        }
        debug << s;
    }
}

void IAX::send_call_auth()
{
	QByteArray out;
	m_md5seed.append(m_password.toUtf8());
	QByteArray result = QCryptographicHash::hash(m_md5seed, QCryptographicHash::Md5);
	uint16_t scall = htons(m_scallno | 0x8000);
	uint16_t dcall = htons(m_dcallno);
	uint32_t ts = htonl((QDateTime::currentMSecsSinceEpoch() - m_timestamp));// + 3);

	out.append((char *)&scall, 2);
	out.append((char *)&dcall, 2);
	out.append((char *)&ts, 4);
	out.append(m_oseq);
	out.append(m_iseq);
	out.append(AST_FRAME_IAX);
	out.append(IAX_COMMAND_AUTHREP);
	out.append(IAX_IE_MD5_RESULT);
	out.append(result.toHex().size());
	out.append(result.toHex());
	m_udp->writeDatagram(out, m_address, m_port);
#ifdef DEBUG
	fprintf(stderr, "SEND: ");
	for(int i = 0; i < out.size(); ++i){
		fprintf(stderr, "%02x ", (unsigned char)out.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
}

void IAX::send_dtmf(QByteArray dtmf)
{
	QByteArray out;
	uint16_t scall = htons(m_scallno | 0x8000);
	uint16_t dcall = htons(m_dcallno);
	//uint32_t ts = htonl((QDateTime::currentMSecsSinceEpoch() - m_timestamp) + 3);
	for(int i = 0; i < dtmf.size(); ++i){
		uint32_t ts = htonl((QDateTime::currentMSecsSinceEpoch() - m_timestamp) + 3 + (i*3));
		out.clear();
		out.append((char *)&scall, 2);
		out.append((char *)&dcall, 2);
		out.append((char *)&ts, 4);
		out.append(m_oseq+i);
		out.append(m_iseq);
		out.append(AST_FRAME_DTMF);
		out.append(dtmf.data()[i]);
		m_udp->writeDatagram(out, m_address, m_port);
#ifdef DEBUG
		fprintf(stderr, "SEND: ");
		for(int i = 0; i < out.size(); ++i){
			fprintf(stderr, "%02x ", (unsigned char)out.data()[i]);
		}
		fprintf(stderr, "\n");
		fflush(stderr);
#endif
	}
}

void IAX::send_radio_key(bool key)
{
	QByteArray out;
	uint16_t scall = htons(m_scallno | 0x8000);
	uint16_t dcall = htons(m_dcallno);
	uint32_t ts = htonl((QDateTime::currentMSecsSinceEpoch() - m_timestamp));// + 3);
	out.clear();
	out.append((char *)&scall, 2);
	out.append((char *)&dcall, 2);
	out.append((char *)&ts, 4);
	out.append(m_oseq);
	out.append(m_iseq);
	out.append(AST_FRAME_CONTROL);
	out.append(key ? AST_CONTROL_KEY : AST_CONTROL_UNKEY);
	m_udp->writeDatagram(out, m_address, m_port);
#ifdef DEBUG
		fprintf(stderr, "SEND: ");
		for(int i = 0; i < out.size(); ++i){
			fprintf(stderr, "%02x ", (unsigned char)out.data()[i]);
		}
		fprintf(stderr, "\n");
		fflush(stderr);
#endif
}

void IAX::send_ping()
{
	QByteArray out;
	uint16_t scall = htons(m_scallno | 0x8000);
	uint16_t dcall = htons(m_dcallno);
	uint32_t ts = htonl((QDateTime::currentMSecsSinceEpoch() - m_timestamp));// + 3);

	out.append((char *)&scall, 2);
	out.append((char *)&dcall, 2);
	out.append((char *)&ts, 4);
	out.append(m_oseq);
	out.append(m_iseq);
	out.append(AST_FRAME_IAX);
	out.append(IAX_COMMAND_PING);
	m_udp->writeDatagram(out, m_address, m_port);
#ifdef DEBUG
	fprintf(stderr, "SEND: ");
	for(int i = 0; i < out.size(); ++i){
		fprintf(stderr, "%02x ", (unsigned char)out.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
}

void IAX::send_pong()
{
	QByteArray out;
	uint16_t scall = htons(m_scallno | 0x8000);
	uint16_t dcall = htons(m_dcallno);
	uint32_t ts = htonl((QDateTime::currentMSecsSinceEpoch() - m_timestamp));// + 3);
	uint32_t jitter = htonl(m_rxjitter);
	//uint32_t loss = htonl(m_rxloss);
	uint32_t frames = htonl(m_rxframes);
	uint16_t delay = htons(m_rxdelay);
	uint32_t dropped = htonl(m_rxdropped);
	uint32_t ooo = htonl(m_rxooo);

	out.append((char *)&scall, 2);
	out.append((char *)&dcall, 2);
	out.append((char *)&ts, 4);
	out.append(m_oseq);
	out.append(m_iseq);
	out.append(AST_FRAME_IAX);
	out.append(IAX_COMMAND_PONG);
	out.append(IAX_IE_RR_JITTER);
	out.append(sizeof(jitter));
	out.append((char *)&jitter, sizeof(jitter));
	out.append(IAX_IE_RR_LOSS);
	out.append(sizeof(m_rxloss));
	out.append((char *)&m_rxloss, sizeof(m_rxloss));
	out.append(IAX_IE_RR_PKTS);
	out.append(sizeof(frames));
	out.append((char *)&frames, sizeof(frames));
	out.append(IAX_IE_RR_DELAY);
	out.append(sizeof(delay));
	out.append((char *)&delay, sizeof(delay));
	out.append(IAX_IE_RR_DROPPED);
	out.append(sizeof(dropped));
	out.append((char *)&dropped, sizeof(dropped));
	out.append(IAX_IE_RR_OOO);
	out.append(sizeof(ooo));
	out.append((char *)&ooo, sizeof(ooo));
	m_udp->writeDatagram(out, m_address, m_port);
#ifdef DEBUG
	fprintf(stderr, "SEND: ");
	for(int i = 0; i < out.size(); ++i){
		fprintf(stderr, "%02x ", (unsigned char)out.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
}

void IAX::send_ack(uint16_t scall, uint16_t dcall, uint8_t oseq, uint8_t iseq)
{
	QByteArray out;
	scall = htons(scall | 0x8000);
	dcall = htons(dcall);
	uint32_t ts = htonl((QDateTime::currentMSecsSinceEpoch() - m_timestamp));// + 3);

	out.append((char *)&scall, 2);
	out.append((char *)&dcall, 2);
	out.append((char *)&ts, 4);
	out.append(oseq);
	out.append(iseq);
	out.append(AST_FRAME_IAX);
	out.append(IAX_COMMAND_ACK);
	m_udp->writeDatagram(out, m_address, m_port);
#ifdef DEBUG
	fprintf(stderr, "SEND: ");
	for(int i = 0; i < out.size(); ++i){
		fprintf(stderr, "%02x ", (unsigned char)out.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
}

void IAX::send_lag_response()
{
	QByteArray out;
	uint16_t scall = htons(m_scallno | 0x8000);
	uint16_t dcall = htons(m_dcallno);
	uint32_t ts = htonl((QDateTime::currentMSecsSinceEpoch() - m_timestamp));// + 3);

	out.append((char *)&scall, 2);
	out.append((char *)&dcall, 2);
	out.append((char *)&ts, 4);
	out.append(m_oseq);
	out.append(m_iseq);
	out.append(AST_FRAME_IAX);
	out.append(IAX_COMMAND_LAGRP);
	m_udp->writeDatagram(out, m_address, m_port);
#ifdef DEBUG
	fprintf(stderr, "SEND: ");
	for(int i = 0; i < out.size(); ++i){
		fprintf(stderr, "%02x ", (unsigned char)out.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
}

void IAX::send_voice_frame(int16_t *f)
{
	QByteArray out;
	uint16_t scall = htons(m_scallno | 0x8000);
	uint16_t dcall = htons(m_dcallno);
	uint32_t ts = htonl((QDateTime::currentMSecsSinceEpoch() - m_timestamp));// + 3);

	out.append((char *)&scall, 2);
	out.append((char *)&dcall, 2);
	out.append((char *)&ts, 4);
	out.append(m_oseq);
	out.append(m_iseq);
	out.append(AST_FRAME_VOICE);
	out.append(AST_FORMAT_ULAW);

	for(int i = 0; i < 160; ++i){
		out.append(ulaw_encode(f[i]));
	}

	m_udp->writeDatagram(out, m_address, m_port);
#ifdef DEBUG
	fprintf(stderr, "SEND: ");
	for(int i = 0; i < out.size(); ++i){
		fprintf(stderr, "%02x ", (unsigned char)out.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
}

void IAX::send_registration(uint16_t dcall)
{
	//static qint64 time = QDateTime::currentMSecsSinceEpoch();
	uint32_t ts;
	uint8_t seq;

	if(dcall){
		dcall = htons(dcall);
		seq = 1;
		ts = htonl((QDateTime::currentMSecsSinceEpoch() - m_timestamp));// + 3);
	}
	else{
		--m_regscallno;
		seq = 0;
		//ts = htonl(3);
		ts  = 0;
		m_md5seed.clear();
	}

	uint16_t scall = htons(m_regscallno | 0x8000);
	uint16_t refresh = htons(60);
	QByteArray out;
	out.append((char *)&scall, 2);
	out.append((char *)&dcall, 2);
	out.append((char *)&ts, 4);
	out.append(seq);
	out.append(seq);
	out.append(AST_FRAME_IAX);
	out.append(IAX_COMMAND_REGREQ);
	out.append(IAX_IE_USERNAME);
	out.append(m_username.size());
	out.append(m_username.toUtf8(), m_username.size());

	if(dcall){
		m_md5seed.append(m_password.toUtf8());
		QByteArray result = QCryptographicHash::hash(m_md5seed, QCryptographicHash::Md5);
		out.append(IAX_IE_MD5_RESULT);
		out.append(result.toHex().size());
		out.append(result.toHex());
	}

	out.append(IAX_IE_REFRESH);
	out.append(0x02);
	out.append((char *)&refresh, 2);			// refresh time = 60 secs
	m_udp->writeDatagram(out, m_address, m_port);
#ifdef DEBUG
	fprintf(stderr, "SEND: ");
	for(int i = 0; i < out.size(); ++i){
		fprintf(stderr, "%02x ", (unsigned char)out.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
}

void IAX::send_disconnect()
{
	QByteArray out;
	uint16_t scall = htons(m_scallno | 0x8000);
	uint16_t dcall = htons(m_dcallno);
	uint32_t ts = htonl((QDateTime::currentMSecsSinceEpoch() - m_timestamp));// + 3);
	QString bye("BuhBye Dudesters");
	out.append((char *)&scall, 2);
	out.append((char *)&dcall, 2);
	out.append((char *)&ts, 4);
	out.append(m_oseq);
	out.append(m_iseq);
	out.append(AST_FRAME_IAX);
	out.append(IAX_COMMAND_HANGUP);
	out.append(IAX_IE_CAUSE);
	out.append(bye.size());
	out.append(bye.toUtf8(), bye.size());
	m_udp->writeDatagram(out, m_address, m_port);
#ifdef DEBUG
	fprintf(stderr, "SEND: ");
	for(int i = 0; i < out.size(); ++i){
		fprintf(stderr, "%02x ", (unsigned char)out.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
}

void IAX::hostname_lookup(QHostInfo i)
{
	if (!i.addresses().isEmpty()) {
		m_address = i.addresses().first();
		m_udp = new QUdpSocket(this);
		m_regtimer = new QTimer();
		connect(m_udp, SIGNAL(readyRead()), this, SLOT(process_udp()));
		connect(m_regtimer, SIGNAL(timeout()), this, SLOT(send_registration()));
		m_timestamp = QDateTime::currentMSecsSinceEpoch();
		send_registration(0);
		m_regtimer->start(60000);
	}
}

void IAX::send_connect()
{
	m_modeinfo.status = CONNECTING;
	qDebug() << "lookup IP = " << m_host << ":" << m_port;
	QHostInfo::lookupHost(m_host, this, SLOT(hostname_lookup(QHostInfo)));
}

void IAX::process_udp()
{
	QByteArray buf;
	QHostAddress sender;
	quint16 senderPort;

	buf.resize(m_udp->pendingDatagramSize());
	m_udp->readDatagram(buf.data(), buf.size(), &sender, &senderPort);
#ifdef DEBUG
	if(buf.data()[0] & 0x80){
	fprintf(stderr, "RECV: ");
	for(int i = 0; i < buf.size(); ++i){
		fprintf(stderr, "%02x ", (unsigned char)buf.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
	}
#endif
	if( (buf.data()[0] & 0x80) &&
		(buf.data()[10] == AST_FRAME_IAX) &&
		(buf.data()[11] == IAX_COMMAND_REGAUTH) &&
		(buf.data()[12] == IAX_IE_AUTHMETHODS) &&
		((buf.data()[15] & 0x02) == IAX_AUTH_MD5) &&
		(buf.data()[16] == IAX_IE_CHALLENGE) )
	{
		uint16_t dcallno = (((buf.data()[0] & 0x7f) << 8) | ((uint8_t)buf.data()[1]));
		m_md5seed.clear();
		m_md5seed.append(buf.mid(18, buf.data()[17]));
		send_registration(dcallno);
	}
	else if((buf.data()[0] & 0x80) &&
		(buf.data()[10] == AST_FRAME_IAX) &&
		(buf.data()[11] == IAX_COMMAND_REGACK) )
	{
		uint16_t dcallno = (((buf.data()[0] & 0x7f) << 8) | ((uint8_t)buf.data()[1]));
		uint16_t scallno = (((buf.data()[2] & 0x7f) << 8) | ((uint8_t)buf.data()[3]));
		send_ack(scallno, dcallno, 2, 2);
		if(m_modeinfo.status == CONNECTING){
			send_call();
		}
	}
	else if((buf.data()[0] & 0x80) &&
		(buf.data()[10] == AST_FRAME_IAX) &&
		(buf.data()[11] == IAX_COMMAND_REGREJ) )
	{
		m_modeinfo.status = DISCONNECTED;
	}
	else if( (buf.data()[0] & 0x80) &&
		(buf.data()[10] == AST_FRAME_IAX) &&
		(buf.data()[11] == IAX_COMMAND_AUTHREQ) &&
		(buf.data()[12] == IAX_IE_AUTHMETHODS) &&
		(buf.data()[15] == IAX_AUTH_MD5) &&
		(buf.data()[16] == IAX_IE_CHALLENGE) )
	{
		++m_rxframes;
		m_dcallno = (((buf.data()[0] & 0x7f) << 8) | ((uint8_t)buf.data()[1]));
		m_md5seed.clear();
		m_md5seed.append(buf.mid(18, buf.data()[17]));
		m_iseq = buf.data()[8] + 1;
		m_oseq = buf.data()[9];
		send_call_auth();
	}
	else if( (buf.data()[0] & 0x80) &&
		(buf.data()[10] == AST_FRAME_IAX) &&
		(buf.data()[11] == IAX_COMMAND_ACK) )
	{
		uint16_t scall = ((buf.data()[2] << 8) | ((uint8_t)buf.data()[3]));
		if(scall == m_scallno){
			m_dcallno = (((buf.data()[0] & 0x7f) << 8) | ((uint8_t)buf.data()[1]));
			m_iseq = buf.data()[8] + 1;
			m_oseq = buf.data()[9];
		}
	}
	else if( (buf.data()[0] & 0x80) &&
		(buf.data()[10] == AST_FRAME_IAX) &&
		(buf.data()[11] == IAX_COMMAND_ACCEPT) )
	{
		++m_rxframes;
		m_dcallno = (((buf.data()[0] & 0x7f) << 8) | ((uint8_t)buf.data()[1]));
		m_iseq = buf.data()[8] + 1;
		m_oseq = buf.data()[9];
		send_ack(m_scallno, m_dcallno, m_oseq, m_iseq);
	}
	else if( (buf.data()[0] & 0x80) &&
		(buf.data()[10] == AST_FRAME_IAX) &&
		(buf.data()[11] == IAX_COMMAND_REJECT) )
	{
		m_modeinfo.status = DISCONNECTED;
	}
	else if( (buf.data()[0] & 0x80) &&
		(buf.data()[10] == AST_FRAME_CONTROL) &&
		(buf.data()[11] == AST_CONTROL_RINGING) )
	{
		//int16_t zeropcm[160];
		//memset(zeropcm, 0, 160 * sizeof(int16_t));
		++m_rxframes;
		m_dcallno = (((buf.data()[0] & 0x7f) << 8) | ((uint8_t)buf.data()[1]));
		m_iseq = buf.data()[8] + 1;
		m_oseq = buf.data()[9];
		send_ack(m_scallno, m_dcallno, m_oseq, m_iseq);
		//send_voice_frame(zeropcm);
	}
	else if( (buf.data()[0] & 0x80) &&
		(buf.data()[10] == AST_FRAME_CONTROL) &&
		(buf.data()[11] == AST_CONTROL_ANSWER) )
	{
		if(m_modeinfo.status == CONNECTING){
			m_modeinfo.status = CONNECTED_RW;
			m_txtimer = new QTimer();
			connect(m_txtimer, SIGNAL(timeout()), this, SLOT(transmit()));
			m_rxtimer = new QTimer();
			connect(m_rxtimer, SIGNAL(timeout()), this, SLOT(process_rx_data()));
			m_rxtimer->start(19);
			m_pingtimer = new QTimer();
			connect(m_pingtimer, SIGNAL(timeout()), this, SLOT(send_ping()));
			m_pingtimer->start(2000);
			m_audio = new AudioEngine(m_audioin, m_audioout);
			m_audio->init();
			m_audio->start_playback();
			m_audio->set_input_buffer_size(640);
			m_audio->start_capture();
			//m_txtimer->start(19);
			m_modeinfo.sw_vocoder_loaded = true;
		}
		++m_rxframes;
		m_dcallno = (((buf.data()[0] & 0x7f) << 8) | ((uint8_t)buf.data()[1]));
		m_iseq = buf.data()[8] + 1;
		m_oseq = buf.data()[9];
		send_ack(m_scallno, m_dcallno, m_oseq, m_iseq);
	}
	else if( (buf.data()[0] & 0x80) &&
		(buf.data()[10] == AST_FRAME_IAX) &&
		(buf.data()[11] == IAX_COMMAND_PING) )
	{
		++m_rxframes;
		++m_cnt;
		m_dcallno = (((buf.data()[0] & 0x7f) << 8) | ((uint8_t)buf.data()[1]));
		m_iseq = buf.data()[8] + 1;
		m_oseq = buf.data()[9];
		send_ack(m_scallno, m_dcallno, m_oseq, m_iseq);
		send_pong();
	}
	else if( (buf.data()[0] & 0x80) &&
		(buf.data()[10] == AST_FRAME_IAX) &&
		(buf.data()[11] == IAX_COMMAND_PONG) )
	{
		++m_rxframes;
		m_dcallno = (((buf.data()[0] & 0x7f) << 8) | ((uint8_t)buf.data()[1]));
		m_iseq = buf.data()[8] + 1;
		m_oseq = buf.data()[9];
		send_ack(m_scallno, m_dcallno, m_oseq, m_iseq);
	}
	else if( (buf.data()[0] & 0x80) &&
		(buf.data()[10] == AST_FRAME_IAX) &&
		(buf.data()[11] == IAX_COMMAND_VNAK) )
	{
		++m_rxframes;
		m_dcallno = (((buf.data()[0] & 0x7f) << 8) | ((uint8_t)buf.data()[1]));
		m_iseq = buf.data()[8] + 1;
		m_oseq = buf.data()[9];
		send_ack(m_scallno, m_dcallno, m_oseq, m_iseq);
	}
	else if( (buf.data()[0] & 0x80) &&
		(buf.data()[10] == AST_FRAME_VOICE) &&
		(buf.data()[11] == AST_FORMAT_ULAW) )
	{
		int16_t zeropcm[160];
		memset(zeropcm, 0, 160 * sizeof(int16_t));
		++m_rxframes;
		m_dcallno = (((buf.data()[0] & 0x7f) << 8) | ((uint8_t)buf.data()[1]));
		m_iseq = buf.data()[8] + 1;
		m_oseq = buf.data()[9];
		send_ack(m_scallno, m_dcallno, m_oseq, m_iseq);
		for(int i = 0; i < buf.size() - 12; ++i){
			m_audioq.append(ulaw_decode(buf.data()[12+i]));
		}
		send_voice_frame(zeropcm);
		if(!m_txtimer->isActive()){
			m_txtimer->start(19);
		}
	}
	else if( (buf.data()[0] & 0x80) &&
		(buf.data()[10] == AST_FRAME_TEXT) )
	{
		++m_rxframes;
		m_dcallno = (((buf.data()[0] & 0x7f) << 8) | ((uint8_t)buf.data()[1]));
		m_iseq = buf.data()[8] + 1;
		m_oseq = buf.data()[9];
		send_ack(m_scallno, m_dcallno, m_oseq, m_iseq);
	}
	else if( (buf.data()[0] & 0x80) &&
		(buf.data()[10] == AST_FRAME_CONTROL) &&
		(buf.data()[11] == AST_CONTROL_OPTION) )
	{
		++m_rxframes;
		m_dcallno = (((buf.data()[0] & 0x7f) << 8) | ((uint8_t)buf.data()[1]));
		m_iseq = buf.data()[8] + 1;
		m_oseq = buf.data()[9];
		send_ack(m_scallno, m_dcallno, m_oseq, m_iseq);
	}
	else if( (buf.data()[0] & 0x80) &&
		(buf.data()[10] == AST_FRAME_IAX) &&
		(buf.data()[11] == IAX_COMMAND_LAGRQ) )
	{
		++m_rxframes;
		m_dcallno = (((buf.data()[0] & 0x7f) << 8) | ((uint8_t)buf.data()[1]));
		m_iseq = buf.data()[8] + 1;
		m_oseq = buf.data()[9];
		send_ack(m_scallno, m_dcallno, m_oseq, m_iseq);
		send_lag_response();
	}
	else if(!(buf.data()[0] & 0x80)){
		uint16_t dcallno = ((buf.data()[0] << 8) | ((uint8_t)buf.data()[1]));
		if(dcallno == m_dcallno){
			for(int i = 0; i < buf.size() - 4; ++i){
				m_audioq.append(ulaw_decode(buf.data()[4+i]));
			}
		}
	}
	emit update(m_modeinfo);
}

void IAX::process_rx_data()
{
	int16_t pcm[160];

	if(m_audioq.size() > 160){
		for(int i = 0; i < 160; ++i){
			pcm[i] = (qreal)m_audioq.dequeue(); // * m_rxgain;
		}
		m_audio->write(pcm, 160);
		emit update_output_level(m_audio->level());
	}
	else return;
}

void IAX::toggle_tx(bool tx)
{
	qDebug() << "IAXCodec::toggle_tx(bool tx) == " << tx;
	tx ? start_tx() : stop_tx();
}

void IAX::start_tx()
{
	//std::cerr << "Pressed TX buffersize == " << audioin->bufferSize() << std::endl;
	//QByteArray tx("*99", 3);
	//send_dtmf(tx);
	send_radio_key(true);
	m_ttscnt = 0;
	qDebug() << "start_tx() " << m_ttsid << " " << m_ttstext;
	m_tx = true;
#ifdef USE_FLITE
	if(m_ttsid == 1){
		tts_audio = flite_text_to_wave(m_ttstext.toStdString().c_str(), voice_kal);
	}
	else if(m_ttsid == 2){
		tts_audio = flite_text_to_wave(m_ttstext.toStdString().c_str(), voice_awb);
	}
	else if(m_ttsid == 3){
		tts_audio = flite_text_to_wave(m_ttstext.toStdString().c_str(), voice_slt);
	}
#endif
}

void IAX::stop_tx()
{
	m_tx = false;
	send_radio_key(false);
	//QByteArray tx("#", 1);
	//send_dtmf(tx);
}

void IAX::transmit()
{
	QByteArray out;
	int16_t pcm[160];
	 uint16_t s = 0;
#ifdef USE_FLITE
	if(m_ttsid > 0){
		m_audio->read(pcm);
		s = 160;
		if(m_tx){
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
	}
#endif
	if(m_ttsid == 0){
		s = m_audio->read(pcm);
	}
	if (s == 0) return;

	uint16_t scall = htons(m_scallno);
	uint16_t ts = htons( (QDateTime::currentMSecsSinceEpoch() - m_timestamp));// + 3 );
	out.append((char *)&scall, 2);
	out.append((char *)&ts, 2);
	for(int i = 0; i < s; ++i){
		out.append(ulaw_encode(pcm[i]));
	}
	m_udp->writeDatagram(out, m_address, m_port);
#ifdef DEBUGG
	fprintf(stderr, "SEND: ");
	for(int i = 0; i < out.size(); ++i){
		fprintf(stderr, "%02x ", (unsigned char)out.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
}

void IAX::deleteLater()
{
	if(m_modeinfo.status == CONNECTED_RW){
		m_udp->disconnect();
		m_txtimer->stop();
		m_rxtimer->stop();
		m_regtimer->stop();
		send_disconnect();
		//delete m_audio;
	}
	//m_cnt = 0;
	QObject::deleteLater();
}
