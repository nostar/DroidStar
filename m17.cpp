/*
	Copyright (C) 2019-2021 Doug McLain
	Copyright (C) 2020,2021 Jonathan Naylor, G4KLX

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

#include <cstring>
#include <iostream>
#include "MMDVMDefines.h"
#include "m17.h"
#include "M17Defines.h"
#include "M17Convolution.h"
#include "Golay24128.h"

#define M17CHARACTERS " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-/."

const uint8_t SCRAMBLER[] = {
	0x00U, 0x00U, 0xD6U, 0xB5U, 0xE2U, 0x30U, 0x82U, 0xFFU, 0x84U, 0x62U, 0xBAU, 0x4EU, 0x96U, 0x90U, 0xD8U, 0x98U, 0xDDU,
	0x5DU, 0x0CU, 0xC8U, 0x52U, 0x43U, 0x91U, 0x1DU, 0xF8U, 0x6EU, 0x68U, 0x2FU, 0x35U, 0xDAU, 0x14U, 0xEAU, 0xCDU, 0x76U,
	0x19U, 0x8DU, 0xD5U, 0x80U, 0xD1U, 0x33U, 0x87U, 0x13U, 0x57U, 0x18U, 0x2DU, 0x29U, 0x78U, 0xC3U};

const uint32_t INTERLEAVER[] = {
	0U, 137U, 90U, 227U, 180U, 317U, 270U, 39U, 360U, 129U, 82U, 219U, 172U, 309U, 262U, 31U, 352U, 121U, 74U, 211U, 164U,
	301U, 254U, 23U, 344U, 113U, 66U, 203U, 156U, 293U, 246U, 15U, 336U, 105U, 58U, 195U, 148U, 285U, 238U, 7U, 328U, 97U,
	50U, 187U, 140U, 277U, 230U, 367U, 320U, 89U, 42U, 179U, 132U, 269U, 222U, 359U, 312U, 81U, 34U, 171U, 124U, 261U, 214U,
	351U, 304U, 73U, 26U, 163U, 116U, 253U, 206U, 343U, 296U, 65U, 18U, 155U, 108U, 245U, 198U, 335U, 288U, 57U, 10U, 147U,
	100U, 237U, 190U, 327U, 280U, 49U, 2U, 139U, 92U, 229U, 182U, 319U, 272U, 41U, 362U, 131U, 84U, 221U, 174U, 311U, 264U,
	33U, 354U, 123U, 76U, 213U, 166U, 303U, 256U, 25U, 346U, 115U, 68U, 205U, 158U, 295U, 248U, 17U, 338U, 107U, 60U, 197U,
	150U, 287U, 240U, 9U, 330U, 99U, 52U, 189U, 142U, 279U, 232U, 1U, 322U, 91U, 44U, 181U, 134U, 271U, 224U, 361U, 314U, 83U,
	36U, 173U, 126U, 263U, 216U, 353U, 306U, 75U, 28U, 165U, 118U, 255U, 208U, 345U, 298U, 67U, 20U, 157U, 110U, 247U, 200U,
	337U, 290U, 59U, 12U, 149U, 102U, 239U, 192U, 329U, 282U, 51U, 4U, 141U, 94U, 231U, 184U, 321U, 274U, 43U, 364U, 133U, 86U,
	223U, 176U, 313U, 266U, 35U, 356U, 125U, 78U, 215U, 168U, 305U, 258U, 27U, 348U, 117U, 70U, 207U, 160U, 297U, 250U, 19U,
	340U, 109U, 62U, 199U, 152U, 289U, 242U, 11U, 332U, 101U, 54U, 191U, 144U, 281U, 234U, 3U, 324U, 93U, 46U, 183U, 136U, 273U,
	226U, 363U, 316U, 85U, 38U, 175U, 128U, 265U, 218U, 355U, 308U, 77U, 30U, 167U, 120U, 257U, 210U, 347U, 300U, 69U, 22U,
	159U, 112U, 249U, 202U, 339U, 292U, 61U, 14U, 151U, 104U, 241U, 194U, 331U, 284U, 53U, 6U, 143U, 96U, 233U, 186U, 323U,
	276U, 45U, 366U, 135U, 88U, 225U, 178U, 315U, 268U, 37U, 358U, 127U, 80U, 217U, 170U, 307U, 260U, 29U, 350U, 119U, 72U,
	209U, 162U, 299U, 252U, 21U, 342U, 111U, 64U, 201U, 154U, 291U, 244U, 13U, 334U, 103U, 56U, 193U, 146U, 283U, 236U, 5U,
	326U, 95U, 48U, 185U, 138U, 275U, 228U, 365U, 318U, 87U, 40U, 177U, 130U, 267U, 220U, 357U, 310U, 79U, 32U, 169U, 122U,
	259U, 212U, 349U, 302U, 71U, 24U, 161U, 114U, 251U, 204U, 341U, 294U, 63U, 16U, 153U, 106U, 243U, 196U, 333U, 286U, 55U,
	8U, 145U, 98U, 235U, 188U, 325U, 278U, 47U};

const uint16_t CRC16_TABLE[] = {0x0000U, 0x5935U, 0xB26AU, 0xEB5FU, 0x3DE1U, 0x64D4U, 0x8F8BU, 0xD6BEU, 0x7BC2U, 0x22F7U, 0xC9A8U,
				   0x909DU, 0x4623U, 0x1F16U, 0xF449U, 0xAD7CU, 0xF784U, 0xAEB1U, 0x45EEU, 0x1CDBU, 0xCA65U, 0x9350U,
				   0x780FU, 0x213AU, 0x8C46U, 0xD573U, 0x3E2CU, 0x6719U, 0xB1A7U, 0xE892U, 0x03CDU, 0x5AF8U, 0xB63DU,
				   0xEF08U, 0x0457U, 0x5D62U, 0x8BDCU, 0xD2E9U, 0x39B6U, 0x6083U, 0xCDFFU, 0x94CAU, 0x7F95U, 0x26A0U,
				   0xF01EU, 0xA92BU, 0x4274U, 0x1B41U, 0x41B9U, 0x188CU, 0xF3D3U, 0xAAE6U, 0x7C58U, 0x256DU, 0xCE32U,
				   0x9707U, 0x3A7BU, 0x634EU, 0x8811U, 0xD124U, 0x079AU, 0x5EAFU, 0xB5F0U, 0xECC5U, 0x354FU, 0x6C7AU,
				   0x8725U, 0xDE10U, 0x08AEU, 0x519BU, 0xBAC4U, 0xE3F1U, 0x4E8DU, 0x17B8U, 0xFCE7U, 0xA5D2U, 0x736CU,
				   0x2A59U, 0xC106U, 0x9833U, 0xC2CBU, 0x9BFEU, 0x70A1U, 0x2994U, 0xFF2AU, 0xA61FU, 0x4D40U, 0x1475U,
				   0xB909U, 0xE03CU, 0x0B63U, 0x5256U, 0x84E8U, 0xDDDDU, 0x3682U, 0x6FB7U, 0x8372U, 0xDA47U, 0x3118U,
				   0x682DU, 0xBE93U, 0xE7A6U, 0x0CF9U, 0x55CCU, 0xF8B0U, 0xA185U, 0x4ADAU, 0x13EFU, 0xC551U, 0x9C64U,
				   0x773BU, 0x2E0EU, 0x74F6U, 0x2DC3U, 0xC69CU, 0x9FA9U, 0x4917U, 0x1022U, 0xFB7DU, 0xA248U, 0x0F34U,
				   0x5601U, 0xBD5EU, 0xE46BU, 0x32D5U, 0x6BE0U, 0x80BFU, 0xD98AU, 0x6A9EU, 0x33ABU, 0xD8F4U, 0x81C1U,
				   0x577FU, 0x0E4AU, 0xE515U, 0xBC20U, 0x115CU, 0x4869U, 0xA336U, 0xFA03U, 0x2CBDU, 0x7588U, 0x9ED7U,
				   0xC7E2U, 0x9D1AU, 0xC42FU, 0x2F70U, 0x7645U, 0xA0FBU, 0xF9CEU, 0x1291U, 0x4BA4U, 0xE6D8U, 0xBFEDU,
				   0x54B2U, 0x0D87U, 0xDB39U, 0x820CU, 0x6953U, 0x3066U, 0xDCA3U, 0x8596U, 0x6EC9U, 0x37FCU, 0xE142U,
				   0xB877U, 0x5328U, 0x0A1DU, 0xA761U, 0xFE54U, 0x150BU, 0x4C3EU, 0x9A80U, 0xC3B5U, 0x28EAU, 0x71DFU,
				   0x2B27U, 0x7212U, 0x994DU, 0xC078U, 0x16C6U, 0x4FF3U, 0xA4ACU, 0xFD99U, 0x50E5U, 0x09D0U, 0xE28FU,
				   0xBBBAU, 0x6D04U, 0x3431U, 0xDF6EU, 0x865BU, 0x5FD1U, 0x06E4U, 0xEDBBU, 0xB48EU, 0x6230U, 0x3B05U,
				   0xD05AU, 0x896FU, 0x2413U, 0x7D26U, 0x9679U, 0xCF4CU, 0x19F2U, 0x40C7U, 0xAB98U, 0xF2ADU, 0xA855U,
				   0xF160U, 0x1A3FU, 0x430AU, 0x95B4U, 0xCC81U, 0x27DEU, 0x7EEBU, 0xD397U, 0x8AA2U, 0x61FDU, 0x38C8U,
				   0xEE76U, 0xB743U, 0x5C1CU, 0x0529U, 0xE9ECU, 0xB0D9U, 0x5B86U, 0x02B3U, 0xD40DU, 0x8D38U, 0x6667U,
				   0x3F52U, 0x922EU, 0xCB1BU, 0x2044U, 0x7971U, 0xAFCFU, 0xF6FAU, 0x1DA5U, 0x4490U, 0x1E68U, 0x475DU,
				   0xAC02U, 0xF537U, 0x2389U, 0x7ABCU, 0x91E3U, 0xC8D6U, 0x65AAU, 0x3C9FU, 0xD7C0U, 0x8EF5U, 0x584BU,
				   0x017EU, 0xEA21U, 0xB314U};

const uint8_t BIT_MASK_TABLE[] = {0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U};

#define WRITE_BIT(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

M17::M17() :
	m_c2(NULL),
	m_txrate(1)
{
#ifdef Q_OS_WIN
	m_txtimerint = 30; // Qt timers on windows seem to be slower than desired value

#else
	m_txcan = 0;
#endif
    m_mode = "M17";
	m_attenuation = 1;
}

M17::~M17()
{
}

void M17::encode_callsign(uint8_t *callsign)
{
	const std::string m17_alphabet(M17CHARACTERS);
	char cs[10];
	memset(cs, 0, sizeof(cs));
	memcpy(cs, callsign, strlen((char *)callsign));
	uint64_t encoded = 0;
	for(int i = std::strlen((char *)callsign)-1; i >= 0; i--) {
		auto pos = m17_alphabet.find(cs[i]);
		if (pos == std::string::npos) {
			pos = 0;
		}
		encoded *= 40;
		encoded += pos;
	}
	for (int i=0; i<6; i++) {
		callsign[i] = (encoded >> (8*(5-i)) & 0xFFU);
	}
}

void M17::decode_callsign(uint8_t *callsign)
{
	const std::string m17_alphabet(M17CHARACTERS);
	uint8_t code[6];
	uint64_t coded = callsign[0];
	for (int i=1; i<6; i++)
		coded = (coded << 8) | callsign[i];
	if (coded > 0xee6b27ffffffu) {
		std::cerr << "Callsign code is too large, 0x" << std::hex << coded << std::endl;
		return;
	}
	memcpy(code, callsign, 6);
	memset(callsign, 0, 10);
	int i = 0;
	while (coded) {
		if(i < 10){
			callsign[i++] = m17_alphabet[coded % 40];
		}
		coded /= 40;
	}
}

void M17::set_mode(bool m)
{
#ifdef USE_EXTERNAL_CODEC2
	if(m_c2){
		codec2_destroy(m_c2);
		m_c2 = NULL;
	}

	if(m){
		m_c2 = codec2_create(CODEC2_MODE_3200);
	}
	else{
		m_c2 = codec2_create(CODEC2_MODE_1600);
	}
#else
	m_c2->codec2_set_mode(m);
#endif
}

bool M17::get_mode()
{
	bool m = true;
#ifdef USE_EXTERNAL_CODEC2
	if(m_c2){
		if(codec2_samples_per_frame(m_c2) == 160){
			m = true;
		}
		else{
			m = false;
		}
	}
#else
	return m_c2->codec2_get_mode();
#endif
	return m;
}
void M17::decode_c2(int16_t *audio, uint8_t *c)
{
#ifdef USE_EXTERNAL_CODEC2
	if(m_c2){
		codec2_decode(m_c2, audio, c);
	}
#else
	m_c2->codec2_decode(audio, c);
#endif
}

void M17::encode_c2(int16_t *audio, uint8_t *c)
{
#ifdef USE_EXTERNAL_CODEC2
	if(m_c2){
		codec2_encode(m_c2, c, audio);
	}
#else
	m_c2->codec2_encode(c, audio);
#endif
}

void M17::process_udp()
{
	QByteArray buf;
	QHostAddress sender;
	quint16 senderPort;

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
	if((m_modeinfo.status != CONNECTED_RW) && (buf.size() == 4) && (::memcmp(buf.data(), "NACK", 4U) == 0)){
		m_modeinfo.status = DISCONNECTED;
	}
	if((buf.size() == 4) && (::memcmp(buf.data(), "ACKN", 4U) == 0)){
		if(m_modeinfo.status == CONNECTING){
			m_modeinfo.status = CONNECTED_RW;
#ifndef USE_EXTERNAL_CODEC2
			m_c2 = new CCodec2(true);
#endif
			m_txtimer = new QTimer();
			connect(m_txtimer, SIGNAL(timeout()), this, SLOT(transmit()));
			m_rxtimer = new QTimer();
			connect(m_rxtimer, SIGNAL(timeout()), this, SLOT(process_rx_data()));
			m_ping_timer = new QTimer();
			connect(m_ping_timer, SIGNAL(timeout()), this, SLOT(send_ping()));
			m_ping_timer->start(8000);
			m_audio = new AudioEngine(m_audioin, m_audioout);
			m_audio->init();
			m_modeinfo.sw_vocoder_loaded = true;
		}
		emit update(m_modeinfo);
	}
	if((buf.size() == 10) && (::memcmp(buf.data(), "PING", 4U) == 0)){
		if(m_modeinfo.streamid == 0){
			m_modeinfo.stream_state = STREAM_IDLE;
		}
		m_modeinfo.count++;
		emit update(m_modeinfo);
	}
	if((buf.size() == 54) && (::memcmp(buf.data(), "M17 ", 4U) == 0)){
		uint16_t streamid = (buf.data()[4] << 8) | (buf.data()[5] & 0xff);
		if( (m_modeinfo.streamid != 0) && (streamid != m_modeinfo.streamid) ){
			qDebug() << "New streamid received before timeout";
			m_modeinfo.streamid = 0;
			m_audio->stop_playback();
		}
		if( !m_tx && (m_modeinfo.streamid == 0) ){
			uint8_t cs[10];
			::memcpy(cs, &(buf.data()[12]), 6);
			decode_callsign(cs);
			m_modeinfo.src = QString((char *)cs);
			::memcpy(cs, &(buf.data()[6]), 6);
			decode_callsign(cs);
			m_modeinfo.dst = QString((char *)cs);
			m_modeinfo.streamid = streamid;
			m_audio->start_playback();

			if((buf.data()[19] & 0x06U) == 0x04U){
				m_modeinfo.type = 1;//"3200 Voice";
				set_mode(true);
			}
			else{
				m_modeinfo.type = 0;//"1600 V/D";
				set_mode(false);
			}

			if(!m_rxtimer->isActive()){
#ifdef Q_OS_WIN
				m_rxtimer->start(m_modeinfo.type ? m_rxtimerint : 32);
#else
				m_rxtimer->start(m_modeinfo.type ? m_rxtimerint : m_rxtimerint*2);
#endif
			}

			m_modeinfo.stream_state = STREAM_NEW;
			m_modeinfo.ts = QDateTime::currentMSecsSinceEpoch();
			qDebug() << "New stream from " << m_modeinfo.src << " to " << m_modeinfo.dst << " id == " << QString::number(m_modeinfo.streamid, 16);
		}
		else{
			m_modeinfo.stream_state = STREAMING;
		}

		m_modeinfo.frame_number = (buf.data()[34] << 8) | (buf.data()[35] & 0xff);
		m_rxwatchdog = 0;
		int s = 8;
		if(get_mode()){
			s = 16;
		}

		for(int i = 0; i < s; ++i){
			m_rxcodecq.append((uint8_t )buf.data()[36+i]);
		}

		if(m_modeinfo.frame_number & 0x8000){ // EOT
			qDebug() << "M17 stream ended";
			m_rxwatchdog = 0;
			m_modeinfo.stream_state = STREAM_END;
			m_modeinfo.ts = QDateTime::currentMSecsSinceEpoch();
			emit update(m_modeinfo);
			m_modeinfo.streamid = 0;
		}
		else{
			emit update(m_modeinfo);
		}
		if(m_modem){
			send_modem_data(buf);
		}
	}
	//emit update(m_modeinfo);
}

void M17::hostname_lookup(QHostInfo i)
{
	if (!i.addresses().isEmpty()) {
		QByteArray out;
		uint8_t cs[10];
		memset(cs, ' ', 9);
		memcpy(cs, m_modeinfo.callsign.toLocal8Bit(), m_modeinfo.callsign.size());
		cs[8] = 'D';
		cs[9] = 0x00;
		M17::encode_callsign(cs);
		out.append('C');
		out.append('O');
		out.append('N');
		out.append('N');
		out.append((char *)cs, 6);
		out.append(m_module);
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

void M17::mmdvm_direct_connect()
{
	if(m_modemport != ""){
		if(m_modeinfo.status == CONNECTING){
			m_modeinfo.status = CONNECTED_RW;
            m_modeinfo.sw_vocoder_loaded = true;
		}
	}
	else{
		qDebug() << "No modem, cant do MMDVM_DIRECT";
	}

#ifndef USE_EXTERNAL_CODEC2
	m_c2 = new CCodec2(true);
#endif
	m_txtimer = new QTimer();
	connect(m_txtimer, SIGNAL(timeout()), this, SLOT(transmit()));
	m_rxtimer = new QTimer();
	connect(m_rxtimer, SIGNAL(timeout()), this, SLOT(process_rx_data()));
	m_audio = new AudioEngine(m_audioin, m_audioout);
	m_audio->init();
	emit update(m_modeinfo);
}

void M17::send_ping()
{
	QByteArray out;
	uint8_t cs[10];
	memset(cs, ' ', 9);
	memcpy(cs, m_modeinfo.callsign.toLocal8Bit(), m_modeinfo.callsign.size());
	cs[8] = 'D';
	cs[9] = 0x00;
	encode_callsign(cs);
	out.append('P');
	out.append('O');
	out.append('N');
	out.append('G');
	out.append((char *)cs, 6);
	m_udp->writeDatagram(out, m_address, m_modeinfo.port);
#ifdef DEBUG
	fprintf(stderr, "PING: ");
	for(int i = 0; i < out.size(); ++i){
		fprintf(stderr, "%02x ", (uint8_t)out.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
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

void M17::send_disconnect()
{
    if(m_mdirect){
		return;
	}

	qDebug() << "send_disconnect()";
	QByteArray out;
	uint8_t cs[10];
	memset(cs, ' ', 9);
	memcpy(cs, m_modeinfo.callsign.toLocal8Bit(), m_modeinfo.callsign.size());
	cs[8] = 'D';
	cs[9] = 0x00;
	encode_callsign(cs);
	out.append('D');
	out.append('I');
	out.append('S');
	out.append('C');
	out.append((char *)cs, 6);
	m_udp->writeDatagram(out, m_address, m_modeinfo.port);

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

void M17::send_modem_data(QByteArray d)
{
	CM17Convolution conv;
	static uint8_t lsf[M17_LSF_LENGTH_BYTES];
	static uint8_t lsfcnt = 0;
	uint8_t txframe[M17_FRAME_LENGTH_BYTES];
	uint8_t tmp[M17_FRAME_LENGTH_BYTES];

	if(m_modeinfo.stream_state == STREAM_NEW){
		::memcpy(lsf, &d.data()[6], M17_LSF_LENGTH_BYTES);
		encodeCRC16(lsf, M17_LSF_LENGTH_BYTES);
		::memcpy(txframe, M17_LINK_SETUP_SYNC_BYTES, 2);
		conv.encodeLinkSetup(lsf, txframe + M17_SYNC_LENGTH_BYTES);
		interleave(txframe, tmp);
		decorrelate(tmp, txframe);

		m_rxmodemq.append(MMDVM_FRAME_START);
		m_rxmodemq.append(M17_FRAME_LENGTH_BYTES + 4);
		m_rxmodemq.append(MMDVM_M17_LINK_SETUP);
		m_rxmodemq.append('\x00');

		//for(int j = 0; j < 3; j++){
			for(uint32_t i = 0; i < M17_FRAME_LENGTH_BYTES; ++i){
				m_rxmodemq.append(txframe[i]);
			}
		//}
	}

	if(lsfcnt == 0){
		::memcpy(lsf, &d.data()[6], M17_LSF_LENGTH_BYTES);
	}

	::memcpy(txframe, M17_STREAM_SYNC_BYTES, 2);

	uint8_t lich[M17_LICH_FRAGMENT_LENGTH_BYTES];
	encodeCRC16(lsf, M17_LSF_LENGTH_BYTES);
	::memcpy(lich, lsf + (lsfcnt * M17_LSF_FRAGMENT_LENGTH_BYTES), M17_LSF_FRAGMENT_LENGTH_BYTES);
	lich[5U] = (lsfcnt & 0x07U) << 5;

	uint32_t frag1, frag2, frag3, frag4;
	splitFragmentLICH(lich, frag1, frag2, frag3, frag4);
	uint32_t lich1 = CGolay24128::encode24128(frag1);
	uint32_t lich2 = CGolay24128::encode24128(frag2);
	uint32_t lich3 = CGolay24128::encode24128(frag3);
	uint32_t lich4 = CGolay24128::encode24128(frag4);
	combineFragmentLICHFEC(lich1, lich2, lich3, lich4, txframe + M17_SYNC_LENGTH_BYTES);

	conv.encodeData((uint8_t *)&d.data()[34], txframe + M17_SYNC_LENGTH_BYTES + M17_LICH_FRAGMENT_FEC_LENGTH_BYTES);
	interleave(txframe, tmp);
	decorrelate(tmp, txframe);

	m_rxmodemq.append(MMDVM_FRAME_START);
	m_rxmodemq.append(M17_FRAME_LENGTH_BYTES + 4);
	m_rxmodemq.append(MMDVM_M17_STREAM);
	m_rxmodemq.append('\x00');

	for(uint32_t i = 0; i < M17_FRAME_LENGTH_BYTES; ++i){
		m_rxmodemq.append(txframe[i]);
	}
	lsfcnt++;
	if (lsfcnt >= 6U)
		lsfcnt = 0U;
}

void M17::process_modem_data(QByteArray d)
{
	QByteArray txframe;
	static uint16_t txstreamid = 0;
	static uint8_t lsf[M17_LSF_LENGTH_BYTES] = {0};
	static uint8_t lsfchunks[M17_LSF_LENGTH_BYTES] = {0};
	static bool validlsf = false;
	CM17Convolution conv;
	uint8_t tmp[M17_FRAME_LENGTH_BYTES];

	if( (d.size() < 3) || m_tx ){
		return;
	}

	if( (d.data()[2] == MMDVM_M17_LINK_SETUP) &&
		(((uint8_t)d.data()[4] != M17_LINK_SETUP_SYNC_BYTES[0]) || ((uint8_t)d.data()[5] != M17_LINK_SETUP_SYNC_BYTES[1]))){
		qDebug() << "M17 LSF with no sync bytes" << (d.data()[2] == MMDVM_M17_LINK_SETUP) << ((uint8_t)d.data()[4] != M17_LINK_SETUP_SYNC_BYTES[0]) << ((uint8_t)d.data()[5] != M17_LINK_SETUP_SYNC_BYTES[1]);
		return;
	}

	if( (d.data()[2] == MMDVM_M17_STREAM) &&
		(((uint8_t)d.data()[4] != M17_STREAM_SYNC_BYTES[0]) || ((uint8_t)d.data()[5] != M17_STREAM_SYNC_BYTES[1]))){
		qDebug() << "M17 stream frame with no sync bytes" << (d.data()[2] == MMDVM_M17_STREAM) << ((uint8_t)d.data()[4] != M17_STREAM_SYNC_BYTES[0]) << ((uint8_t)d.data()[5] != M17_STREAM_SYNC_BYTES[1]);
		return;
	}

	uint8_t *p = (uint8_t *)d.data();

	if((d.data()[2] == MMDVM_M17_LINK_SETUP) || (d.data()[2] == MMDVM_M17_STREAM)){
		p += 4;
		decorrelate(p, tmp);
		interleave(tmp, p);
	}

	if((d.data()[2] == MMDVM_M17_LOST) || (d.data()[2] == MMDVM_M17_EOT)){
		txstreamid = 0;
        if(m_mdirect){
			m_modeinfo.streamid = 0;
			m_modeinfo.dst.clear();
			m_modeinfo.src.clear();
			m_modeinfo.stream_state = STREAM_END;
			::memset(lsf, 0, M17_LSF_LENGTH_BYTES);
			::memset(lsfchunks, 0, M17_LSF_LENGTH_BYTES);
			validlsf = false;
		}
		qDebug() << "End of M17 stream";
	}
	else if(d.data()[2] == MMDVM_M17_LINK_SETUP){
		::memset(lsf, 0x00U, M17_LSF_LENGTH_BYTES);
		uint32_t  ber = conv.decodeLinkSetup(p + M17_SYNC_LENGTH_BYTES, lsf);
		validlsf = checkCRC16(lsf, M17_LSF_LENGTH_BYTES);
		txstreamid = static_cast<uint16_t>((::rand() & 0xFFFF));
		qDebug() << "M17 LSF received valid == " << validlsf << "ber: " << ber;

        if(validlsf && m_mdirect){
			uint8_t cs[10];
			::memcpy(cs, lsf, 6);
			decode_callsign(cs);
			m_modeinfo.dst = QString((char *)cs);
			::memcpy(cs, lsf+6, 6);
			decode_callsign(cs);
			m_modeinfo.src = QString((char *)cs);
		}
	}
	else if(d.data()[2] == MMDVM_M17_STREAM){
		uint8_t frame[M17_FN_LENGTH_BYTES + M17_PAYLOAD_LENGTH_BYTES];
		uint32_t ber = conv.decodeData(p + M17_SYNC_LENGTH_BYTES + M17_LICH_FRAGMENT_FEC_LENGTH_BYTES, frame);
		uint16_t fn = (frame[0U] << 8) + (frame[1U] << 0);

		uint8_t netframe[M17_LSF_LENGTH_BYTES + M17_FN_LENGTH_BYTES + M17_PAYLOAD_LENGTH_BYTES + M17_CRC_LENGTH_BYTES];
		::memcpy(netframe, lsf, M17_LSF_LENGTH_BYTES);
		::memcpy(netframe + M17_LSF_LENGTH_BYTES - M17_CRC_LENGTH_BYTES, frame, M17_FN_LENGTH_BYTES + M17_PAYLOAD_LENGTH_BYTES);
		netframe[M17_LSF_LENGTH_BYTES - M17_CRC_LENGTH_BYTES + 0U] &= 0x7FU;

		uint32_t lich1, lich2, lich3, lich4;
		bool valid1 = CGolay24128::decode24128(p + M17_SYNC_LENGTH_BYTES + 0U, lich1);
		bool valid2 = CGolay24128::decode24128(p + M17_SYNC_LENGTH_BYTES + 3U, lich2);
		bool valid3 = CGolay24128::decode24128(p + M17_SYNC_LENGTH_BYTES + 6U, lich3);
		bool valid4 = CGolay24128::decode24128(p + M17_SYNC_LENGTH_BYTES + 9U, lich4);

		if (valid1 && valid2 && valid3 && valid4) {
			uint8_t lich[M17_LICH_FRAGMENT_LENGTH_BYTES];
			combineFragmentLICH(lich1, lich2, lich3, lich4, lich);

			uint32_t n = (lich4 >> 5) & 0x07U;
			::memcpy(lsfchunks + (n * M17_LSF_FRAGMENT_LENGTH_BYTES), lich, M17_LSF_FRAGMENT_LENGTH_BYTES);

			bool valid = checkCRC16(lsfchunks, M17_LSF_LENGTH_BYTES);
			qDebug() << "lich valid == " << valid << " lich n == " << n;
			if (valid) {
				::memcpy(lsf, lsfchunks, M17_LSF_LENGTH_BYTES);
				::memset(lsfchunks, 0, M17_LSF_LENGTH_BYTES);
				validlsf = valid;
			}

			if(!validlsf){
				qDebug() << "No LSF yet...";
				return;
			}
		}

        if(m_mdirect){
			if( !m_tx && (m_modeinfo.streamid == 0) ){
				if(txstreamid == 0){
					qDebug() << "No header, late entry...";
					uint8_t cs[10];
					::memcpy(cs, lsf, 6);
					decode_callsign(cs);
					m_modeinfo.dst = QString((char *)cs);
					::memcpy(cs, lsf+6, 6);
					decode_callsign(cs);
					m_modeinfo.src = QString((char *)cs);
					txstreamid = static_cast<uint16_t>((::rand() & 0xFFFF));
				}

				m_modeinfo.stream_state = STREAM_NEW;
				m_modeinfo.streamid = txstreamid;
				m_modeinfo.ts = QDateTime::currentMSecsSinceEpoch();
				qDebug() << "New RF stream from " << m_modeinfo.src << " to " << m_modeinfo.dst << " id == " << QString::number(m_modeinfo.streamid, 16) << "FN == " << fn << " ber == " << ber;

				m_audio->start_playback();

				if(!m_rxtimer->isActive()){
	#ifdef Q_OS_WIN
					m_rxtimer->start(m_rxtimerint);
	#else
					m_rxtimer->start(m_rxtimerint);
	#endif
				}


			}
			else{
				m_modeinfo.stream_state = STREAMING;
			}

			qDebug() << "RF streaming from " << m_modeinfo.src << " to " << m_modeinfo.dst << " id == " << QString::number(m_modeinfo.streamid, 16) << "FN == " << fn << " ber == " << ber << " type == " << netframe[13];

			if((netframe[13] & 0x06U) == 0x04U){
				m_modeinfo.type = 1;//"3200 Voice";
				set_mode(true);
			}
			else{
				m_modeinfo.type = 0;//"1600 V/D";
				set_mode(false);
			}

			m_modeinfo.frame_number = (netframe[28] << 8) | (netframe[29] & 0xff);
			m_rxwatchdog = 0;

			int s = 8;
			if(get_mode()){
				s = 16;
			}

			for(int i = 0; i < s; ++i){
				m_rxcodecq.append(netframe[30+i]);
			}

			emit update(m_modeinfo);
		}
		else{
			if(txstreamid == 0){
				qDebug() << "No header for netframe";
				txstreamid = static_cast<uint16_t>((::rand() & 0xFFFF));
			}

			uint8_t dst[10];
			memset(dst, ' ', 9);
			memcpy(dst, m_refname.toLocal8Bit(), m_refname.size());
			dst[8] =  m_module;
			dst[9] = 0x00;
			encode_callsign(dst);

			txframe.append('M');
			txframe.append('1');
			txframe.append('7');
			txframe.append(' ');
			txframe.append(txstreamid >> 8);
			txframe.append(txstreamid & 0xff);
			txframe.append((char *)dst, 6);
			//txframe.append((char *)src, 6);
			txframe.append((char *)&netframe[6], 6);
			txframe.append(netframe[12]);
			txframe.append(netframe[13]);
			txframe.append(14, 0x00); //Blank nonce
			txframe.append((char)(netframe[28] >> 8));
			txframe.append((char)netframe[29] & 0xff);
			txframe.append((char *)&netframe[30], 16);
			txframe.append(2, 0x00);
			m_udp->writeDatagram(txframe, m_address, m_modeinfo.port);

            if(m_debug){
                QDebug debug = qDebug();
                debug.noquote();
                QString s = "SEND:";
                for(int i = 0; i < txframe.size(); ++i){
                    s += " " + QString("%1").arg((uint8_t)txframe.data()[i], 2, 16, QChar('0'));
                }
                debug << s;
            }
		}
	}
}

void M17::toggle_tx(bool tx)
{
	qDebug() << "M17Codec::toggle_tx(bool tx) == " << tx;
	tx ? start_tx() : stop_tx();
}

void M17::start_tx()
{
	m_txtimerint = 38;
	set_mode(m_txrate);
	Mode::start_tx();
}

void M17::transmit()
{
	QByteArray txframe;
	static uint16_t txstreamid = 0;
	static uint16_t tx_cnt = 0;
	int16_t pcm[320];
	uint8_t c2[16];
#ifdef USE_FLITE
	static uint16_t ttscnt = 0;
	if(m_ttsid > 0){
		for(int i = 0; i < 320; ++i){
			if(ttscnt >= tts_audio->num_samples/2){
				//audiotx_cnt = 0;
				pcm[i] = 0;
			}
			else{
				pcm[i] = tts_audio->samples[ttscnt*2] / 8;
				ttscnt++;
			}
		}

		encode_c2(pcm, c2);

		if(get_mode()){
			encode_c2(pcm+160, c2+8);
		}
	}
#endif
	if(m_ttsid == 0){
		if(m_audio->read(pcm, 320)){
			encode_c2(pcm, c2);
			if(get_mode()){
				encode_c2(pcm+160, c2+8);
			}
		}
		else{
			return;
		}
	}

	txframe.clear();
	emit update_output_level(m_audio->level() * 2);
	int r = get_mode() ? 0x05 : 0x07;

	if(m_tx){
		if(txstreamid == 0){
		   txstreamid = static_cast<uint16_t>((::rand() & 0xFFFF));
           if(!m_rxtimer->isActive() && m_mdirect){
			   m_rxmodemq.clear();
			   m_modeinfo.stream_state = STREAM_NEW;
#ifdef Q_OS_WIN
			   m_rxtimer->start(m_modeinfo.type ? m_rxtimerint : 32);
#else
			   m_rxtimer->start(19);
#endif
		   }

		}
		else{
            if(m_mdirect){
				m_modeinfo.stream_state = STREAMING;
			}
		}

		uint8_t src[10];
		uint8_t dst[10];
		uint8_t lsf[30];
		memset(dst, ' ', 9);
		memcpy(dst, m_refname.toLocal8Bit(), m_refname.size());

		dst[8] =  m_module;
		dst[9] = 0x00;
		encode_callsign(dst);
		memset(src, ' ', 9);
		memcpy(src, m_modeinfo.callsign.toLocal8Bit(), m_modeinfo.callsign.size());
		src[8] = 'D';
		src[9] = 0x00;
		encode_callsign(src);

		txframe.append('M');
		txframe.append('1');
		txframe.append('7');
		txframe.append(' ');
		txframe.append(txstreamid >> 8);
		txframe.append(txstreamid & 0xff);
		txframe.append((char *)dst, 6);
		txframe.append((char *)src, 6);
		txframe.append(m_txcan >> 1);
		txframe.append(((m_txcan << 7) & 0x80U) | r);
		txframe.append(14, 0x00); //Blank nonce
		txframe.append((char)(tx_cnt >> 8));
		txframe.append((char)tx_cnt & 0xff);
		txframe.append((char *)c2, 16);

		for(int i = 0; i < 28; ++i){
			lsf[i] = txframe.data()[6+i];
		}

		encodeCRC16(lsf, M17_LSF_LENGTH_BYTES);
		txframe.append(lsf[28]);
		txframe.append(lsf[29]);

        if(m_mdirect){
			send_modem_data(txframe);
			m_rxwatchdog = 0;
		}
		else{
			m_udp->writeDatagram(txframe, m_address, m_modeinfo.port);
		}

		++tx_cnt;
		m_modeinfo.src = m_modeinfo.callsign;
		m_modeinfo.dst = m_refname;
        m_modeinfo.module = m_module;
		m_modeinfo.type = get_mode();
		m_modeinfo.frame_number = tx_cnt;
		m_modeinfo.streamid = txstreamid;
		emit update(m_modeinfo);
#ifdef DEBUG
		fprintf(stderr, "SEND:%d: ", txframe.size());
		for(int i = 0; i < txframe.size(); ++i){
			fprintf(stderr, "%02x ", (uint8_t)txframe.data()[i]);
		}
		fprintf(stderr, "\n");
		fflush(stderr);
#endif
        if(m_debug){
            QDebug debug = qDebug();
            debug.noquote();
            QString s = "SEND:";
            for(int i = 0; i < txframe.size(); ++i){
                s += " " + QString("%1").arg((uint8_t)txframe.data()[i], 2, 16, QChar('0'));
            }
            debug << s;
        }
	}
	else{
		const uint8_t quiet3200[] = { 0x00, 0x01, 0x43, 0x09, 0xe4, 0x9c, 0x08, 0x21 };
		const uint8_t quiet1600[] = { 0x01, 0x00, 0x04, 0x00, 0x25, 0x75, 0xdd, 0xf2 };
		const uint8_t *quiet = (get_mode()) ? quiet3200 : quiet1600;
		uint8_t src[10];
		uint8_t dst[10];
		memset(dst, ' ', 9);
		memcpy(dst, m_refname.toLocal8Bit(), m_refname.size());
		dst[8] =  m_module;
		dst[9] = 0x00;
		encode_callsign(dst);
		memset(src, ' ', 9);
		memcpy(src, m_modeinfo.callsign.toLocal8Bit(), m_modeinfo.callsign.size());
		src[8] = 'D';
		src[9] = 0x00;
		M17::encode_callsign(src);
		tx_cnt |= 0x8000u;

		txframe.append('M');
		txframe.append('1');
		txframe.append('7');
		txframe.append(' ');
		txframe.append(txstreamid >> 8);
		txframe.append(txstreamid & 0xff);
		txframe.append((char *)dst, 6);
		txframe.append((char *)src, 6);
		txframe.append('\x00');
		txframe.append(r); // Frame type voice only
		txframe.append(14, 0x00); //Blank nonce
		txframe.append((char)(tx_cnt >> 8));
		txframe.append((char)tx_cnt & 0xff);
		txframe.append((char *)quiet, 8);
		txframe.append((char *)quiet, 8);
		txframe.append(2, 0x00);

        if(m_mdirect){
			send_modem_data(txframe);
			m_modeinfo.stream_state = STREAM_END;
		}
		else{
			m_udp->writeDatagram(txframe, m_address, m_modeinfo.port);
		}
		txstreamid = 0;
		tx_cnt = 0;
#ifdef USE_FLITE
		ttscnt = 0;
#endif
		m_txtimer->stop();
		if(m_ttsid == 0){
			m_audio->stop_capture();
		}
		m_modeinfo.src = m_modeinfo.callsign;
		m_modeinfo.dst = m_refname;
		m_modeinfo.type = get_mode();
		m_modeinfo.frame_number = tx_cnt;
		m_modeinfo.streamid = txstreamid;
		emit update(m_modeinfo);

        if(m_debug){
            QDebug debug = qDebug();
            debug.noquote();
            QString s = "LAST:";
            for(int i = 0; i < txframe.size(); ++i){
                s += " " + QString("%1").arg((uint8_t)txframe.data()[i], 2, 16, QChar('0'));
            }
            debug << s;
        }
	}
}

void M17::process_rx_data()
{
	int16_t pcm[320];
	uint8_t codec2[8];
	static uint8_t cnt = 0;

	if(m_rxwatchdog++ > 50){
		qDebug() << "RX stream timeout ";
		m_rxwatchdog = 0;
		m_modeinfo.stream_state = STREAM_LOST;
		m_modeinfo.ts = QDateTime::currentMSecsSinceEpoch();
		emit update(m_modeinfo);
		m_modeinfo.streamid = 0;
	}

	if((m_rxmodemq.size() > 2) && (++cnt >= 2)){
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

	if((!m_tx) && (m_rxcodecq.size() > 7) ){
		for(int i = 0; i < 8; ++i){
			codec2[i] = m_rxcodecq.dequeue();
		}
		decode_c2(pcm, codec2);
		int s = get_mode() ? 160 : 320;
		m_audio->write(pcm, s);
		emit update_output_level(m_audio->level());
	}
	else if ( ((m_modeinfo.stream_state == STREAM_END) || (m_modeinfo.stream_state == STREAM_LOST)) && (m_rxmodemq.size() < 50) ){
		m_rxtimer->stop();
		m_audio->stop_playback();
		m_rxwatchdog = 0;
		m_modeinfo.streamid = 0;
		m_rxcodecq.clear();
		m_rxmodemq.clear();
		qDebug() << "M17 playback stopped";
		m_modeinfo.stream_state = STREAM_IDLE;
		return;
	}
}

void M17::decorrelate(uint8_t *in, uint8_t *out)
{
	for (uint32_t i = M17_SYNC_LENGTH_BYTES; i < M17_FRAME_LENGTH_BYTES; i++) {
		out[i] = in[i] ^ SCRAMBLER[i];
	}
}

void M17::interleave(uint8_t *in, uint8_t *out)
{
	for (uint32_t i = 0U; i < (M17_FRAME_LENGTH_BITS - M17_SYNC_LENGTH_BITS); i++) {
		uint32_t n1 = i + M17_SYNC_LENGTH_BITS;
		bool b = READ_BIT(in, n1) != 0U;
		uint32_t n2 = INTERLEAVER[i] + M17_SYNC_LENGTH_BITS;
		WRITE_BIT(out, n2, b);
	}
}

void M17::splitFragmentLICH(const uint8_t* data, uint32_t& frag1, uint32_t& frag2, uint32_t& frag3, uint32_t& frag4)
{
	assert(data != NULL);

	frag1 = frag2 = frag3 = frag4 = 0x00U;

	uint32_t offset = 0U;
	uint32_t MASK = 0x800U;
	for (uint32_t i = 0U; i < (M17_LICH_FRAGMENT_LENGTH_BITS / 4U); i++, offset++, MASK >>= 1) {
		bool b = READ_BIT(data, offset) != 0x00U;
		if (b)
			frag1 |= MASK;
	}

	MASK = 0x800U;
	for (uint32_t i = 0U; i < (M17_LICH_FRAGMENT_LENGTH_BITS / 4U); i++, offset++, MASK >>= 1) {
		bool b = READ_BIT(data, offset) != 0x00U;
		if (b)
			frag2 |= MASK;
	}

	MASK = 0x800U;
	for (uint32_t i = 0U; i < (M17_LICH_FRAGMENT_LENGTH_BITS / 4U); i++, offset++, MASK >>= 1) {
		bool b = READ_BIT(data, offset) != 0x00U;
		if (b)
			frag3 |= MASK;
	}

	MASK = 0x800U;
	for (uint32_t i = 0U; i < (M17_LICH_FRAGMENT_LENGTH_BITS / 4U); i++, offset++, MASK >>= 1) {
		bool b = READ_BIT(data, offset) != 0x00U;
		if (b)
			frag4 |= MASK;
	}
}

void M17::combineFragmentLICH(uint32_t frag1, uint32_t frag2, uint32_t frag3, uint32_t frag4, uint8_t* data)
{
	assert(data != NULL);

	uint32_t offset = 0U;
	uint32_t MASK = 0x800U;
	for (uint32_t i = 0U; i < (M17_LICH_FRAGMENT_LENGTH_BITS / 4U); i++, offset++, MASK >>= 1) {
		bool b = (frag1 & MASK) == MASK;
		WRITE_BIT(data, offset, b);
	}

	MASK = 0x800U;
	for (uint32_t i = 0U; i < (M17_LICH_FRAGMENT_LENGTH_BITS / 4U); i++, offset++, MASK >>= 1) {
		bool b = (frag2 & MASK) == MASK;
		WRITE_BIT(data, offset, b);
	}

	MASK = 0x800U;
	for (uint32_t i = 0U; i < (M17_LICH_FRAGMENT_LENGTH_BITS / 4U); i++, offset++, MASK >>= 1) {
		bool b = (frag3 & MASK) == MASK;
		WRITE_BIT(data, offset, b);
	}

	MASK = 0x800U;
	for (uint32_t i = 0U; i < (M17_LICH_FRAGMENT_LENGTH_BITS / 4U); i++, offset++, MASK >>= 1) {
		bool b = (frag4 & MASK) == MASK;
		WRITE_BIT(data, offset, b);
	}
}

void M17::combineFragmentLICHFEC(uint32_t frag1, uint32_t frag2, uint32_t frag3, uint32_t frag4, uint8_t* data)
{
	assert(data != NULL);

	uint32_t offset = 0U;
	uint32_t MASK = 0x800000U;
	for (uint32_t i = 0U; i < (M17_LICH_FRAGMENT_FEC_LENGTH_BITS / 4U); i++, offset++, MASK >>= 1) {
		bool b = (frag1 & MASK) == MASK;
		WRITE_BIT(data, offset, b);
	}

	MASK = 0x800000U;
	for (uint32_t i = 0U; i < (M17_LICH_FRAGMENT_FEC_LENGTH_BITS / 4U); i++, offset++, MASK >>= 1) {
		bool b = (frag2 & MASK) == MASK;
		WRITE_BIT(data, offset, b);
	}

	MASK = 0x800000U;
	for (uint32_t i = 0U; i < (M17_LICH_FRAGMENT_FEC_LENGTH_BITS / 4U); i++, offset++, MASK >>= 1) {
		bool b = (frag3 & MASK) == MASK;
		WRITE_BIT(data, offset, b);
	}

	MASK = 0x800000U;
	for (uint32_t i = 0U; i < (M17_LICH_FRAGMENT_FEC_LENGTH_BITS / 4U); i++, offset++, MASK >>= 1) {
		bool b = (frag4 & MASK) == MASK;
		WRITE_BIT(data, offset, b);
	}
}

bool M17::checkCRC16(const uint8_t* in, uint32_t nBytes)
{
	assert(in != NULL);
	assert(nBytes > 2U);

	uint16_t crc = createCRC16(in, nBytes - 2U);

	uint8_t temp[2U];
	temp[0U] = (crc >> 8) & 0xFFU;
	temp[1U] = (crc >> 0) & 0xFFU;

	return temp[0U] == in[nBytes - 2U] && temp[1U] == in[nBytes - 1U];
}

void M17::encodeCRC16(uint8_t* in, uint32_t nBytes)
{
	assert(in != NULL);
	assert(nBytes > 2U);

	uint16_t crc = createCRC16(in, nBytes - 2U);

	in[nBytes - 2U] = (crc >> 8) & 0xFFU;
	in[nBytes - 1U] = (crc >> 0) & 0xFFU;
}

uint16_t M17::createCRC16(const uint8_t* in, uint32_t nBytes)
{
	assert(in != NULL);

	uint16_t crc = 0xFFFFU;

	for (uint32_t i = 0U; i < nBytes; i++)
		crc = (crc << 8) ^ CRC16_TABLE[((crc >> 8) ^ uint16_t(in[i])) & 0x00FFU];

	return crc;
}
