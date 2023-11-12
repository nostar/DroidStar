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

#include "ysf.h"
#include "YSFConvolution.h"
#include "CRCenc.h"
#include "Golay24128.h"
#include "chamming.h"
#include "MMDVMDefines.h"
#include <iostream>
#include <cstring>
#ifdef USE_MD380_VOCODER
#include <md380_vocoder.h>
#endif


const uint32_t IMBE_INTERLEAVE[] = {
	0,  7, 12, 19, 24, 31, 36, 43, 48, 55, 60, 67, 72, 79, 84, 91,  96, 103, 108, 115, 120, 127, 132, 139,
	1,  6, 13, 18, 25, 30, 37, 42, 49, 54, 61, 66, 73, 78, 85, 90,  97, 102, 109, 114, 121, 126, 133, 138,
	2,  9, 14, 21, 26, 33, 38, 45, 50, 57, 62, 69, 74, 81, 86, 93,  98, 105, 110, 117, 122, 129, 134, 141,
	3,  8, 15, 20, 27, 32, 39, 44, 51, 56, 63, 68, 75, 80, 87, 92,  99, 104, 111, 116, 123, 128, 135, 140,
	4, 11, 16, 23, 28, 35, 40, 47, 52, 59, 64, 71, 76, 83, 88, 95, 100, 107, 112, 119, 124, 131, 136, 143,
	5, 10, 17, 22, 29, 34, 41, 46, 53, 58, 65, 70, 77, 82, 89, 94, 101, 106, 113, 118, 125, 130, 137, 142
};

const int dvsi_interleave[49] = {
	0, 3, 6,  9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 41, 43, 45, 47,
	1, 4, 7, 10, 13, 16, 19, 22, 25, 28, 31, 34, 37, 40, 42, 44, 46, 48,
	2, 5, 8, 11, 14, 17, 20, 23, 26, 29, 32, 35, 38
};

const uint32_t INTERLEAVE_TABLE_5_20[] = {
	0U, 40U,  80U, 120U, 160U,
	2U, 42U,  82U, 122U, 162U,
	4U, 44U,  84U, 124U, 164U,
	6U, 46U,  86U, 126U, 166U,
	8U, 48U,  88U, 128U, 168U,
	10U, 50U,  90U, 130U, 170U,
	12U, 52U,  92U, 132U, 172U,
	14U, 54U,  94U, 134U, 174U,
	16U, 56U,  96U, 136U, 176U,
	18U, 58U,  98U, 138U, 178U,
	20U, 60U, 100U, 140U, 180U,
	22U, 62U, 102U, 142U, 182U,
	24U, 64U, 104U, 144U, 184U,
	26U, 66U, 106U, 146U, 186U,
	28U, 68U, 108U, 148U, 188U,
	30U, 70U, 110U, 150U, 190U,
	32U, 72U, 112U, 152U, 192U,
	34U, 74U, 114U, 154U, 194U,
	36U, 76U, 116U, 156U, 196U,
	38U, 78U, 118U, 158U, 198U};

const uint32_t INTERLEAVE_TABLE_9_20[] = {
		0U, 40U,  80U, 120U, 160U, 200U, 240U, 280U, 320U,
		2U, 42U,  82U, 122U, 162U, 202U, 242U, 282U, 322U,
		4U, 44U,  84U, 124U, 164U, 204U, 244U, 284U, 324U,
		6U, 46U,  86U, 126U, 166U, 206U, 246U, 286U, 326U,
		8U, 48U,  88U, 128U, 168U, 208U, 248U, 288U, 328U,
	   10U, 50U,  90U, 130U, 170U, 210U, 250U, 290U, 330U,
	   12U, 52U,  92U, 132U, 172U, 212U, 252U, 292U, 332U,
	   14U, 54U,  94U, 134U, 174U, 214U, 254U, 294U, 334U,
	   16U, 56U,  96U, 136U, 176U, 216U, 256U, 296U, 336U,
	   18U, 58U,  98U, 138U, 178U, 218U, 258U, 298U, 338U,
	   20U, 60U, 100U, 140U, 180U, 220U, 260U, 300U, 340U,
	   22U, 62U, 102U, 142U, 182U, 222U, 262U, 302U, 342U,
	   24U, 64U, 104U, 144U, 184U, 224U, 264U, 304U, 344U,
	   26U, 66U, 106U, 146U, 186U, 226U, 266U, 306U, 346U,
	   28U, 68U, 108U, 148U, 188U, 228U, 268U, 308U, 348U,
	   30U, 70U, 110U, 150U, 190U, 230U, 270U, 310U, 350U,
	   32U, 72U, 112U, 152U, 192U, 232U, 272U, 312U, 352U,
	   34U, 74U, 114U, 154U, 194U, 234U, 274U, 314U, 354U,
	   36U, 76U, 116U, 156U, 196U, 236U, 276U, 316U, 356U,
	   38U, 78U, 118U, 158U, 198U, 238U, 278U, 318U, 358U};

const uint32_t INTERLEAVE_TABLE_26_4[] = {
	0U, 4U,  8U, 12U, 16U, 20U, 24U, 28U, 32U, 36U, 40U, 44U, 48U, 52U, 56U, 60U, 64U, 68U, 72U, 76U, 80U, 84U, 88U, 92U, 96U, 100U,
	1U, 5U,  9U, 13U, 17U, 21U, 25U, 29U, 33U, 37U, 41U, 45U, 49U, 53U, 57U, 61U, 65U, 69U, 73U, 77U, 81U, 85U, 89U, 93U, 97U, 101U,
	2U, 6U, 10U, 14U, 18U, 22U, 26U, 30U, 34U, 38U, 42U, 46U, 50U, 54U, 58U, 62U, 66U, 70U, 74U, 78U, 82U, 86U, 90U, 94U, 98U, 102U,
	3U, 7U, 11U, 15U, 19U, 23U, 27U, 31U, 35U, 39U, 43U, 47U, 51U, 55U, 59U, 63U, 67U, 71U, 75U, 79U, 83U, 87U, 91U, 95U, 99U, 103U};

const uint32_t WHITENING_DATA[] = {0x93U, 0xD7U, 0x51U, 0x21U, 0x9CU, 0x2FU, 0x6CU, 0xD0U, 0xEFU, 0x0FU,
										0xF8U, 0x3DU, 0xF1U, 0x73U, 0x20U, 0x94U, 0xEDU, 0x1EU, 0x7CU, 0xD8U};

const uint8_t BIT_MASK_TABLE[] = {0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U};

#define WRITE_BIT(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

YSF::YSF() :
	m_fcs(false),
	m_txfullrate(false)
{
    m_mode = "YSF";
	m_attenuation = 5;
#ifdef USE_MD380_VOCODER
    md380_init();
#endif
}

YSF::~YSF()
{
}

void YSF::process_udp()
{
	QByteArray buf;
    QByteArray out;
	QHostAddress sender;
	quint16 senderPort;
	char ysftag[11];
	buf.resize(m_udp->pendingDatagramSize());
	int p = 5000;
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

	if(((buf.size() == 14) && (m_refname.left(3) != "FCS")) || ((buf.size() == 7) && (m_refname.left(3) == "FCS"))){
		if(m_modeinfo.status == CONNECTING){
			m_modeinfo.status = CONNECTED_RW;
			m_txtimer = new QTimer();
			connect(m_txtimer, SIGNAL(timeout()), this, SLOT(transmit()));
			m_ping_timer = new QTimer();
			connect(m_ping_timer, SIGNAL(timeout()), this, SLOT(send_ping()));
			set_fcs_mode(false);
			//m_mbeenc->set_gain_adjust(2.5);
			m_modeinfo.sw_vocoder_loaded = load_vocoder_plugin();

			m_rxtimer = new QTimer();
			connect(m_rxtimer, SIGNAL(timeout()), this, SLOT(process_rx_data()));

			m_audio = new AudioEngine(m_audioin, m_audioout);
			m_audio->init();

			if(m_refname.left(3) == "FCS"){
				char info[100U];
				::sprintf(info, "%9u%9u%-6.6s%-12.12s%7u", 438000000, 438000000, "AA00AA", "MMDVM", 1234567);
				::memset(info + 43U, ' ', 57U);
				out.append(info, 100);
				m_udp->writeDatagram(out, m_address, m_modeinfo.port);
				p = 800;
				set_fcs_mode(true, m_refname.left(8).toStdString());
			}
			m_ping_timer->start(p);
		}
		if((m_modeinfo.stream_state == STREAM_LOST) || (m_modeinfo.stream_state == STREAM_END) ){
			m_modeinfo.stream_state = STREAM_IDLE;
		}
		m_modeinfo.count++;
	}
	if((buf.size() == 10) && (::memcmp(buf.data(), "ONLINE", 6U) == 0)){
		m_modeinfo.count++;
		if((m_modeinfo.stream_state == STREAM_LOST) || (m_modeinfo.stream_state == STREAM_END) ){
			m_modeinfo.stream_state = STREAM_IDLE;
		}
	}
	uint8_t *p_data = nullptr;
	if((buf.size() == 155) && (::memcmp(buf.data(), "YSFD", 4U) == 0)){
		memcpy(ysftag, buf.data() + 4, 10);ysftag[10] = '\0';
		m_modeinfo.gw = QString(ysftag);
		//memcpy(ysftag, buf.data() + 14, 10);ysftag[10] = '\0';
		//m_modeinfo.src = QString(ysftag);
		//memcpy(ysftag, buf.data() + 24, 10);ysftag[10] = '\0';
		//m_modeinfo.dst = QString(ysftag);

		p_data = (uint8_t *)buf.data() + 35;
		if(m_modem){
			m_rxmodemq.append(MMDVM_FRAME_START);
			m_rxmodemq.append(124);
			m_rxmodemq.append(MMDVM_YSF_DATA);
			m_rxmodemq.append('\x00');

			for(int i = 0; i < 120; ++i){
				m_rxmodemq.append(buf.data()[35+i]);
			}
			//m_rxmodemq.append(buf.mid(35));
		}
	}
	else if(buf.size() == 130){
		memcpy(ysftag, buf.data() + 0x79, 8);ysftag[8] = '\0';
		m_modeinfo.gw = QString(ysftag);
		p_data = (uint8_t *)buf.data();
		if(m_modem){
			m_rxmodemq.append(MMDVM_FRAME_START);
			m_rxmodemq.append(124);
			m_rxmodemq.append(MMDVM_YSF_DATA);
			m_rxmodemq.append('\x00');

			for(int i = 0; i < 120; ++i){
				m_rxmodemq.append(buf.data()[i]);
			}
			//m_rxmodemq.append(buf.mid(35));
		}
	}

	if(p_data != nullptr){
		m_rxwatchdog = 0;
		CYSFFICH fich;
		if(fich.decode(p_data)){
			m_fi = fich.getFI();
			m_modeinfo.frame_number = fich.getFN();
			m_modeinfo.frame_total = fich.getFT();
			m_modeinfo.path = fich.getVoIP();
			m_modeinfo.type = fich.getDT();

			if(m_fi == YSF_FI_HEADER){
				m_modeinfo.stream_state = STREAM_NEW;
				m_modeinfo.ts = QDateTime::currentMSecsSinceEpoch();
				if(!m_tx && !m_rxtimer->isActive() ){
					m_audio->start_playback();
					m_rxtimer->start(m_rxtimerint);
				}
				decode_header(p_data);
				qDebug() << "New YSF stream from gw" << m_modeinfo.gw;
			}
			else if(m_fi == YSF_FI_TERMINATOR){
				m_modeinfo.stream_state = STREAM_END;
				m_modeinfo.ts = QDateTime::currentMSecsSinceEpoch();
				qDebug() << "YSF stream ended" << m_modeinfo.gw;
			}
			else if(YSF_FI_COMMUNICATIONS){
				if( (m_modeinfo.stream_state == STREAM_END) ||
					(m_modeinfo.stream_state == STREAM_LOST) ||
					(m_modeinfo.stream_state == STREAM_IDLE))
				{
					m_modeinfo.stream_state = STREAM_NEW;
					m_modeinfo.ts = QDateTime::currentMSecsSinceEpoch();
					if(!m_tx && !m_rxtimer->isActive() ){
						m_audio->start_playback();
						m_rxtimer->start(m_rxtimerint);
					}
					qDebug() << "New YSF stream in progress from gw" << m_modeinfo.gw;

				}
				else{
					m_modeinfo.stream_state = STREAMING;
				}
			}
		}
		if(m_modeinfo.type == 3){
			decode_vw(p_data);
		}
		else if(m_modeinfo.type != 1){
			decode_dn(p_data);
		}
	}
	emit update(m_modeinfo);
}

void YSF::hostname_lookup(QHostInfo i)
{
	if (!i.addresses().isEmpty()) {
		QByteArray out;
		if(m_refname.left(3) == "FCS"){
			out.append('P');
			out.append('I');
			out.append('N');
			out.append('G');
			out.append(m_modeinfo.callsign.toUtf8());
			out.append(6 - m_modeinfo.callsign.size(), ' ');
			out.append(m_refname.toUtf8());
			out.append(7, '\x00');
		}
		else{
			out.append('Y');
			out.append('S');
			out.append('F');
			out.append('P');
			out.append(m_modeinfo.callsign.toUtf8());
			out.append(10 - m_modeinfo.callsign.size(), ' ');
		}
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

void YSF::send_ping()
{
	QByteArray out;
	if(m_refname.left(3) == "FCS"){
		out.append('P');
		out.append('I');
		out.append('N');
		out.append('G');
		out.append(m_modeinfo.callsign.toUtf8());
		out.append(6 - m_modeinfo.callsign.size(), ' ');
		out.append(m_refname.toUtf8());
		out.append(7, '\x00');
	}
	else{
		out.append('Y');
		out.append('S');
		out.append('F');
		out.append('P');
		out.append(m_modeinfo.callsign.toUtf8());
		out.append(10 - m_modeinfo.callsign.size(), ' ');
	}
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

void YSF::send_disconnect()
{
	QByteArray out;
	if(m_refname.left(3) == "FCS"){
		out.append('C');
		out.append('L');
		out.append('O');
		out.append('S');
		out.append('E');
		out.append(6, ' ');
	}
	else{
		out.append('Y');
		out.append('S');
		out.append('F');
		out.append('U');
		out.append(m_modeinfo.callsign.toUtf8());
		out.append(10 - m_modeinfo.callsign.size(), ' ');
	}
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

void YSF::decode_header(uint8_t* data)
{
	assert(data != NULL);

	data += YSF_SYNC_LENGTH_BYTES + YSF_FICH_LENGTH_BYTES;

	uint8_t* source = NULL;
	uint8_t* dest = NULL;
	uint8_t dch[45U];

	uint8_t* p1 = data;
	uint8_t* p2 = dch;
	for (uint32_t i = 0U; i < 5U; i++) {
		::memcpy(p2, p1, 9U);
		p1 += 18U; p2 += 9U;
	}

	CYSFConvolution conv;
	conv.start();

	for (uint32_t i = 0U; i < 180U; i++) {
		uint32_t n = INTERLEAVE_TABLE_9_20[i];
		uint8_t s0 = READ_BIT(dch, n) ? 1U : 0U;

		n++;
		uint8_t s1 = READ_BIT(dch, n) ? 1U : 0U;

		conv.decode(s0, s1);
	}

	uint8_t output[23U];
	conv.chainback(output, 176U);

	bool valid = CCRC::checkCCITT162(output, 22U);
	if (valid) {
		for (uint32_t i = 0U; i < 20U; i++)
			output[i] ^= WHITENING_DATA[i];

		if (dest == NULL) {
			dest = new uint8_t[YSF_CALLSIGN_LENGTH];
			::memcpy(dest, output + 0U, YSF_CALLSIGN_LENGTH);
		}

		if (source == NULL) {
			source = new uint8_t[YSF_CALLSIGN_LENGTH];
			::memcpy(source, output + YSF_CALLSIGN_LENGTH, YSF_CALLSIGN_LENGTH);
		}

		m_modeinfo.src = QString::fromUtf8((const char *)source);
		m_modeinfo.dst = QString::fromUtf8((const char *)dest);
		m_modeinfo.gw2 = QString::fromUtf8((const char *)dest);
	}
}

void YSF::decode_vw(uint8_t* data)
{
	uint8_t vch[18U];
	uint8_t imbe[11U];
	bool bit[144U];

	data += YSF_SYNC_LENGTH_BYTES + YSF_FICH_LENGTH_BYTES;

	uint32_t offset = 0U;

	// We have a total of 5 VCH sections, iterate through each
	for (uint32_t j = 0U; j < 5U; j++, offset += 18U) {
		::memcpy(vch, data + offset, 18U);

		for (uint32_t i = 0U; i < 144U; i++) {
			uint32_t n = IMBE_INTERLEAVE[i];
			bit[i] = READ_BIT(vch, n);
		}
		uint32_t c0data = 0U;
		for (uint32_t i = 0U; i < 12U; i++)
			c0data = (c0data << 1) | (bit[i] ? 0x01U : 0x00U);

		bool prn[114U];

		// Create the whitening vector and save it for future use
		uint32_t p = 16U * c0data;
		for (uint32_t i = 0U; i < 114U; i++) {
			p = (173U * p + 13849U) % 65536U;
			prn[i] = p >= 32768U;
		}

		// De-whiten some bits
		for (uint32_t i = 0U; i < 114U; i++)
			bit[i + 23U] ^= prn[i];

		uint32_t offset = 0U;
		for (uint32_t i = 0U; i < 12U; i++, offset++)
			WRITE_BIT(imbe, offset, bit[i + 0U]);
		for (uint32_t i = 0U; i < 12U; i++, offset++)
			WRITE_BIT(imbe, offset, bit[i + 23U]);
		for (uint32_t i = 0U; i < 12U; i++, offset++)
			WRITE_BIT(imbe, offset, bit[i + 46U]);
		for (uint32_t i = 0U; i < 12U; i++, offset++)
			WRITE_BIT(imbe, offset, bit[i + 69U]);
		for (uint32_t i = 0U; i < 11U; i++, offset++)
			WRITE_BIT(imbe, offset, bit[i + 92U]);
		for (uint32_t i = 0U; i < 11U; i++, offset++)
			WRITE_BIT(imbe, offset, bit[i + 107U]);
		for (uint32_t i = 0U; i < 11U; i++, offset++)
			WRITE_BIT(imbe, offset, bit[i + 122U]);
		for (uint32_t i = 0U; i < 7U; i++, offset++)
			WRITE_BIT(imbe, offset, bit[i + 137U]);

		for(int i = 0; i < 11; ++i){
			m_rximbecodecq.append(imbe[i]);
			//m_rxcodecq.append(imbe[i]);
		}
	}
}

void YSF::decode_vd1(uint8_t* data, uint8_t *dt)
{
	uint8_t dch[45U];

	const uint8_t* p1 = data;
	uint8_t* p2 = dch;
	for (uint32_t i = 0U; i < 5U; i++) {
		::memcpy(p2, p1, 9U);
		p1 += 18U; p2 += 9U;
	}

	CYSFConvolution conv;
	conv.start();

	for (uint32_t i = 0U; i < 180U; i++) {
		uint32_t n = INTERLEAVE_TABLE_9_20[i];
		uint8_t s0 = READ_BIT(dch, n) ? 1U : 0U;

		n++;
		uint8_t s1 = READ_BIT(dch, n) ? 1U : 0U;

		conv.decode(s0, s1);
	}

	uint8_t output[23U];
	conv.chainback(output, 176U);

	bool ret = CCRC::checkCCITT162(output, 22U);
	if (ret) {
		for (uint32_t i = 0U; i < 20U; i++){
			output[i] ^= WHITENING_DATA[i];
		}
		::memcpy(dt, output, 20U);
	}
}

void YSF::decode_vd2(uint8_t* data, uint8_t *dt)
{
	uint8_t dch[25U];

	const uint8_t* p1 = data;
	uint8_t* p2 = dch;
	for (uint32_t i = 0U; i < 5U; i++) {
		::memcpy(p2, p1, 5U);
		p1 += 18U; p2 += 5U;
	}

	CYSFConvolution conv;
	conv.start();

	for (uint32_t i = 0U; i < 100U; i++) {
		uint32_t n = INTERLEAVE_TABLE_5_20[i];
		uint8_t s0 = READ_BIT(dch, n) ? 1U : 0U;

		n++;
		uint8_t s1 = READ_BIT(dch, n) ? 1U : 0U;

		conv.decode(s0, s1);
	}

	uint8_t output[13U];
	conv.chainback(output, 96U);

	bool ret = CCRC::checkCCITT162(output, 12U);
	if (ret) {
		for (uint32_t i = 0U; i < 10U; i++){
			output[i] ^= WHITENING_DATA[i];
		}
		::memcpy(dt, output, YSF_CALLSIGN_LENGTH);
	}
}

void YSF::decode_dn(uint8_t* data)
{
	uint8_t v_tmp[7U];
	uint8_t dt[20];
	::memset(v_tmp, 0, 7U);

	data += YSF_SYNC_LENGTH_BYTES + YSF_FICH_LENGTH_BYTES;
	switch(m_modeinfo.type){
	case 0:
		decode_vd1(data, dt);
		break;
	case 2:
		decode_vd2(data, dt);
		dt[10] = 0x00;
		break;
	}

	switch (m_modeinfo.frame_number) {
	case 0:
		if(m_fi == YSF_FI_COMMUNICATIONS){
			m_modeinfo.dst = QString((char *)dt);
		}
		break;
	case 1:
		m_modeinfo.src = QString((char *)dt);
		break;
	}

	uint32_t offset = 40U; // DCH(0)

	// We have a total of 5 VCH sections, iterate through each
	for (uint32_t j = 0U; j < 5U; j++, offset += 144U) {

		uint8_t vch[13U];
		uint32_t dat_a = 0U;
		uint32_t dat_b = 0U;
		uint32_t dat_c = 0U;

		// Deinterleave
		for (uint32_t i = 0U; i < 104U; i++) {
			uint32_t n = INTERLEAVE_TABLE_26_4[i];
			bool s = READ_BIT(data, offset + n);
			WRITE_BIT(vch, i, s);
		}

		// "Un-whiten" (descramble)
		for (uint32_t i = 0U; i < 13U; i++)
			vch[i] ^= WHITENING_DATA[i];

		for (uint32_t i = 0U; i < 12U; i++) {
			dat_a <<= 1U;
			if (READ_BIT(vch, 3U*i + 1U))
				dat_a |= 0x01U;;
		}

		for (uint32_t i = 0U; i < 12U; i++) {
			dat_b <<= 1U;
			if (READ_BIT(vch, 3U*(i + 12U) + 1U))
				dat_b |= 0x01U;;
		}

		for (uint32_t i = 0U; i < 3U; i++) {
			dat_c <<= 1U;
			if (READ_BIT(vch, 3U*(i + 24U) + 1U))
				dat_c |= 0x01U;;
		}

		for (uint32_t i = 0U; i < 22U; i++) {
			dat_c <<= 1U;
			if (READ_BIT(vch, i + 81U))
				dat_c |= 0x01U;;
		}

		for (uint32_t i = 0U; i < 12U; i++) {
			bool s1 = (dat_a << (i + 20U)) & 0x80000000;
			bool s2 = (dat_b << (i + 20U)) & 0x80000000;
			WRITE_BIT(v_tmp, i, s1);
			WRITE_BIT(v_tmp, i + 12U, s2);
		}

		for (uint32_t i = 0U; i < 25U; i++) {
			bool s = (dat_c << (i + 7U)) & 0x80000000;
			WRITE_BIT(v_tmp, i + 24U, s);
		}
		if(m_hwrx){
			interleave(v_tmp);
		}
		for(int i = 0; i < 7; ++i){
			m_rxcodecq.append(v_tmp[i]);
		}
	}
}

void YSF::interleave(uint8_t *ambe)
{
	char ambe_data[49];
	char dvsi_data[7];
	memset(dvsi_data, 0, 7);

	for(int i = 0; i < 6; ++i){
		for(int j = 0; j < 8; j++){
			ambe_data[j+(8*i)] = (1 & (ambe[i] >> (7 - j)));
		}
	}
	ambe_data[48] = (1 & (ambe[6] >> 7));
	for(int i = 0, j; i < 49; ++i){
		j = dvsi_interleave[i];
		dvsi_data[j/8] += (ambe_data[i])<<(7-(j%8));
	}
	memcpy(ambe, dvsi_data, 7);
}

void YSF::process_modem_data(QByteArray d)
{
	if(d.size() < 126){
		return;
	}

	char callsign[YSF_CALLSIGN_LENGTH+1];
	::memcpy(callsign, "          ", YSF_CALLSIGN_LENGTH);
	::memcpy(callsign, m_modeinfo.callsign.toStdString().c_str(), ::strlen(m_modeinfo.callsign.toStdString().c_str()));

	d.remove(0, 4);

	if(m_fcs){
		d.insert(120, 10, 0);
		d.insert(121, m_fcsname.c_str(), 8);
		d.resize(130);
	}
	else{
		d.insert(0U, "YSFD", 4U);
		d.insert(4U, callsign, YSF_CALLSIGN_LENGTH);
		d.insert(14U, callsign, YSF_CALLSIGN_LENGTH);
		d.insert(24U, "ALL       ", YSF_CALLSIGN_LENGTH);
		d.insert(34U, (m_txcnt & 0x7f) << 1);
		d.resize(155);
	}

	++m_txcnt;
	m_udp->writeDatagram(d, m_address, m_modeinfo.port);
	qDebug() << "Sending modem to network.....................................................";

    if(m_debug){
        QDebug debug = qDebug();
        debug.noquote();
        QString s = "SEND:";
        for(int i = 0; i < d.size(); ++i){
            s += " " + QString("%1").arg((uint8_t)d.data()[i], 2, 16, QChar('0'));
        }
        debug << s;
    }
}

void YSF::transmit()
{
	uint8_t ambe_frame[88];
	uint8_t ambe[7];
	int16_t pcm[160];
	uint8_t s = 7;

	memset(ambe, 0, 7);
#ifdef USE_FLITE
	if(m_ttsid > 0){
		for(int i = 0; i < 160; ++i){
			if(m_ttscnt >= tts_audio->num_samples/2){
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
	if(m_hwtx && !m_txfullrate){
#if !defined(Q_OS_IOS)
		m_ambedev->encode(pcm);
#endif
	}
	else{
		if(m_txfullrate){
			s = 11;
			vocoder.encode_4400(pcm, ambe_frame);
		}
		else{
			s = 7;
			if(m_modeinfo.sw_vocoder_loaded){
#ifdef USE_MD380_VOCODER
                md380_encode(ambe, pcm);
#else
				m_mbevocoder->encode_2450(pcm, ambe);
#endif
			}
		}

		for(int i = 0; i < s; ++i){
			if(!m_txfullrate){
				m_txcodecq.append(ambe[i]);
			}
			else{
				m_txcodecq.append(ambe_frame[i]);
			}
		}
	}
	if(m_tx && (m_txcodecq.size() >= (s*5))){
		for(int i = 0; i < (s*5); ++i){
			m_ambe[i] = m_txcodecq.dequeue();
		}
		send_frame();
	}
	else if(m_tx == false){
		send_frame();
	}
}

void YSF::send_frame()
{
	QByteArray txdata;
	int frame_size;

	if(m_tx){
		m_modeinfo.stream_state = TRANSMITTING;

		if(!m_txcnt){
			encode_header();
		}
		else{
			m_txfullrate ? encode_vw() : encode_dv2();
		}

		frame_size = ::memcmp(m_ysfFrame, "YSFD", 4) ? 130 : 155;
		txdata.append((char *)m_ysfFrame, frame_size);
		m_udp->writeDatagram(txdata, m_address, m_modeinfo.port);
		++m_txcnt;

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
	else{
		fprintf(stderr, "YSF TX stopped\n");
		m_txtimer->stop();
		if(m_ttsid == 0){
			m_audio->stop_capture();
		}
		encode_header(1);
		m_txcnt = 0;
		m_ttscnt = 0;
		frame_size = ::memcmp(m_ysfFrame, "YSFD", 4) ? 130 : 155;
		txdata.append((char *)m_ysfFrame, frame_size);
		m_udp->writeDatagram(txdata, m_address, m_modeinfo.port);
		m_modeinfo.stream_state = STREAM_IDLE;
	}
	emit update_output_level(m_audio->level() * 8);
	emit update(m_modeinfo);
}

void YSF::encode_header(bool eot)
{
	uint8_t callsign[12];
	::memcpy(callsign, "          ", 10);
	::memcpy(callsign, m_modeinfo.callsign.toStdString().c_str(), ::strlen(m_modeinfo.callsign.toStdString().c_str()));

	uint8_t *p_frame = m_ysfFrame;
	if(m_fcs){
		::memset(p_frame + 120U, 0, 10U);
		::memcpy(p_frame + 121U, m_fcsname.c_str(), 8);
	}
	else{
		::memcpy(p_frame + 0U, "YSFD", 4U);
		::memcpy(p_frame + 4U, callsign, YSF_CALLSIGN_LENGTH);
		::memcpy(p_frame + 14U, callsign, YSF_CALLSIGN_LENGTH);
		::memcpy(p_frame + 24U, "ALL       ", YSF_CALLSIGN_LENGTH);

		if(eot){
			p_frame[34U] = ((m_txcnt & 0x7f) << 1U) | 1U;
		}
		else{
			p_frame[34U] = 0U;
		}
		p_frame = m_ysfFrame + 35U;
	}
	::memcpy(p_frame, YSF_SYNC_BYTES, 5);
	
	m_fich.setFI(eot ? YSF_FI_TERMINATOR : YSF_FI_HEADER);
	m_fich.setCS(2U);
	m_fich.setCM(0U);
	m_fich.setBN(0U);
	m_fich.setBT(0U);
	m_fich.setFN(0U);
	m_fich.setFT(6U);
	m_fich.setDev(0U);
	m_fich.setMR(0U);
	m_fich.setVoIP(false);
	m_fich.setDT(m_txfullrate ? YSF_DT_VOICE_FR_MODE : YSF_DT_VD_MODE2);
	m_fich.setSQL(false);
	m_fich.setSQ(0U);
	m_fich.encode(p_frame);

	uint8_t csd1[20U], csd2[20U];
	memset(csd1, '*', YSF_CALLSIGN_LENGTH);
	//memset(csd1, '*', YSF_CALLSIGN_LENGTH/2);
	//memcpy(csd1 + YSF_CALLSIGN_LENGTH/2, ysf_radioid, YSF_CALLSIGN_LENGTH/2);
	memcpy(csd1 + YSF_CALLSIGN_LENGTH, callsign, YSF_CALLSIGN_LENGTH);
	memcpy(csd2, callsign, YSF_CALLSIGN_LENGTH);
	memcpy(csd2 + YSF_CALLSIGN_LENGTH, callsign, YSF_CALLSIGN_LENGTH);
	//memset(csd2, ' ', YSF_CALLSIGN_LENGTH + YSF_CALLSIGN_LENGTH);

	writeDataFRModeData1(csd1, p_frame);
	writeDataFRModeData2(csd2, p_frame);
}

void YSF::encode_vw()
{
	uint8_t callsign[12];
	::memcpy(callsign, "          ", 10);
	::memcpy(callsign, m_modeinfo.callsign.toStdString().c_str(), ::strlen(m_modeinfo.callsign.toStdString().c_str()));
	uint8_t *p_frame = m_ysfFrame;
	if(m_fcs){
		::memset(p_frame + 120U, 0, 10U);
		::memcpy(p_frame + 121U, m_fcsname.c_str(), 8);
	}
	else{
		::memcpy(m_ysfFrame + 0U, "YSFD", 4U);
		::memcpy(m_ysfFrame + 4U, callsign, YSF_CALLSIGN_LENGTH);
		::memcpy(m_ysfFrame + 14U, callsign, YSF_CALLSIGN_LENGTH);
		::memcpy(m_ysfFrame + 24U, "ALL       ", YSF_CALLSIGN_LENGTH);
		m_ysfFrame[34U] = (m_txcnt & 0x7f) << 1;
		p_frame = m_ysfFrame + 35U;
	}
	::memcpy(p_frame, YSF_SYNC_BYTES, 5);
	uint32_t fn = (m_txcnt - 1U) % 7U;

	m_fich.setFI(YSF_FI_COMMUNICATIONS);
	m_fich.setCS(2U);
	m_fich.setCM(0U);
	m_fich.setBN(0U);
	m_fich.setBT(0U);
	m_fich.setFN(fn);
	m_fich.setFT(6U);
	m_fich.setDev(0U);
	m_fich.setMR(0U);
	m_fich.setVoIP(false);
	m_fich.setDT(YSF_DT_VOICE_FR_MODE);
	m_fich.setSQL(false);
	m_fich.setSQ(0U);
	m_fich.encode(p_frame);

	m_modeinfo.gw = m_modeinfo.callsign;
	m_modeinfo.src = m_modeinfo.callsign;
	m_modeinfo.dst = "ALL       ";
	m_modeinfo.type = YSF_DT_VOICE_FR_MODE;
	m_modeinfo.path = false;
	m_modeinfo.frame_number = fn;
	m_modeinfo.frame_total = 6;

	p_frame += YSF_SYNC_LENGTH_BYTES + YSF_FICH_LENGTH_BYTES;
	for(int i = 0; i < 5; ++i){
		//uint8_t imbe4400[11];
		uint8_t imbe7200[18];

		//for(int j = 0; j < 11; ++j){
		//	imbe4400[j] = m_txcodecq.dequeue();
		//}
		encode_imbe(imbe7200, m_ambe + (11*i));
		memcpy(p_frame + (18*i), imbe7200, 18);
	}
}

void YSF::encode_imbe(uint8_t* data, const uint8_t* imbe)
{
	bool bTemp[144U];
	bool* bit = bTemp;

	// c0
	uint32_t c0 = 0U;
	for (uint32_t i = 0U; i < 12U; i++) {
		bool b = READ_BIT(imbe, i);
		c0 = (c0 << 1) | (b ? 0x01U : 0x00U);
	}
	uint32_t g2 = CGolay24128::encode23127(c0);
	for (int i = 23; i >= 0; i--) {
		bit[i] = (g2 & 0x01U) == 0x01U;
		g2 >>= 1;
	}
	bit += 23U;

	// c1
	uint32_t c1 = 0U;
	for (uint32_t i = 12U; i < 24U; i++) {
		bool b = READ_BIT(imbe, i);
		c1 = (c1 << 1) | (b ? 0x01U : 0x00U);
	}
	g2 = CGolay24128::encode23127(c1);
	for (int i = 23; i >= 0; i--) {
		bit[i] = (g2 & 0x01U) == 0x01U;
		g2 >>= 1;
	}
	bit += 23U;

	// c2
	uint32_t c2 = 0;
	for (uint32_t i = 24U; i < 36U; i++) {
		bool b = READ_BIT(imbe, i);
		c2 = (c2 << 1) | (b ? 0x01U : 0x00U);
	}
	g2 = CGolay24128::encode23127(c2);
	for (int i = 23; i >= 0; i--) {
		bit[i] = (g2 & 0x01U) == 0x01U;
		g2 >>= 1;
	}
	bit += 23U;

	// c3
	uint32_t c3 = 0U;
	for (uint32_t i = 36U; i < 48U; i++) {
		bool b = READ_BIT(imbe, i);
		c3 = (c3 << 1) | (b ? 0x01U : 0x00U);
	}
	g2 = CGolay24128::encode23127(c3);
	for (int i = 23; i >= 0; i--) {
		bit[i] = (g2 & 0x01U) == 0x01U;
		g2 >>= 1;
	}
	bit += 23U;

	// c4
	for (uint32_t i = 0U; i < 11U; i++)
		bit[i] = READ_BIT(imbe, i + 48U);
	CHamming::encode15113_1(bit);
	bit += 15U;

	// c5
	for (uint32_t i = 0U; i < 11U; i++)
		bit[i] = READ_BIT(imbe, i + 59U);
	CHamming::encode15113_1(bit);
	bit += 15U;

	// c6
	for (uint32_t i = 0U; i < 11U; i++)
		bit[i] = READ_BIT(imbe, i + 70U);
	CHamming::encode15113_1(bit);
	bit += 15U;

	// c7
	for (uint32_t i = 0U; i < 7U; i++)
		bit[i] = READ_BIT(imbe, i + 81U);

	bool prn[114U];

	// Create the whitening vector and save it for future use
	uint32_t p = 16U * c0;
	for (uint32_t i = 0U; i < 114U; i++) {
		p = (173U * p + 13849U) % 65536U;
		prn[i] = p >= 32768U;
	}

	// Whiten some bits
	for (uint32_t i = 0U; i < 114U; i++)
		bTemp[i + 23U] ^= prn[i];

	// Interleave
	for (uint32_t i = 0U; i < 144U; i++) {
		uint32_t n = IMBE_INTERLEAVE[i];
		WRITE_BIT(data, n, bTemp[i]);
	}
}

void YSF::encode_dv2()
{
	uint8_t callsign[12];
	::memcpy(callsign, "          ", 10);
	::memcpy(callsign, m_modeinfo.callsign.toStdString().c_str(), ::strlen(m_modeinfo.callsign.toStdString().c_str()));
	uint8_t *p_frame = m_ysfFrame;
	if(m_fcs){
		::memset(p_frame + 120U, 0, 10U);
		::memcpy(p_frame + 121U, m_fcsname.c_str(), 8);
	}
	else{
		::memcpy(m_ysfFrame + 0U, "YSFD", 4U);
		::memcpy(m_ysfFrame + 4U, callsign, YSF_CALLSIGN_LENGTH);
		::memcpy(m_ysfFrame + 14U, callsign, YSF_CALLSIGN_LENGTH);
		::memcpy(m_ysfFrame + 24U, "ALL       ", YSF_CALLSIGN_LENGTH);
		m_ysfFrame[34U] = (m_txcnt & 0x7f) << 1;
		p_frame = m_ysfFrame + 35U;
	}
	::memcpy(p_frame, YSF_SYNC_BYTES, 5);
	uint32_t fn = (m_txcnt - 1U) % 7U;

	m_fich.setFI(YSF_FI_COMMUNICATIONS);
	m_fich.setCS(2U);
	m_fich.setCM(0U);
	m_fich.setBN(0U);
	m_fich.setBT(0U);
	m_fich.setFN(fn);
	m_fich.setFT(6U);
	m_fich.setDev(0U);
	m_fich.setMR(0U);
	m_fich.setVoIP(false);
	m_fich.setDT(YSF_DT_VD_MODE2);
	m_fich.setSQL(false);
	m_fich.setSQ(0U);
	m_fich.encode(p_frame);

	m_modeinfo.gw = m_modeinfo.callsign;
	m_modeinfo.src = m_modeinfo.callsign;
	m_modeinfo.dst = "ALL       ";
	m_modeinfo.type = YSF_DT_VD_MODE2;
	m_modeinfo.path = false;
	m_modeinfo.frame_number = fn;
	m_modeinfo.frame_total = 6;

	const uint8_t ft70d1[10] = {0x01, 0x22, 0x61, 0x5f, 0x2b, 0x03, 0x11, 0x00, 0x00, 0x00};
	//const uint8_t dt1_temp[] = {0x31, 0x22, 0x62, 0x5F, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00};
	const uint8_t dt2_temp[10] = {0x00, 0x00, 0x00, 0x00, 0x6C, 0x20, 0x1C, 0x20, 0x03, 0x08};

	switch (fn) {
	case 0:
		//memset(dch, '*', YSF_CALLSIGN_LENGTH/2);
		//memcpy(dch + YSF_CALLSIGN_LENGTH/2, ysf_radioid, YSF_CALLSIGN_LENGTH/2);
		//writeVDMode2Data(m_ysfFrame + 35U, dch);	//Dest
		writeVDMode2Data(p_frame, (const uint8_t*)"**********");
		break;
	case 1:
		writeVDMode2Data(p_frame, (const uint8_t*)callsign);		//Src
		break;
	case 2:
		writeVDMode2Data(p_frame, (const uint8_t*)callsign);				//D/L
		break;
	case 3:
		writeVDMode2Data(p_frame, (const uint8_t*)callsign);				//U/L
		break;
	case 4:
		writeVDMode2Data(p_frame, (const uint8_t*)"          ");			//Rem1/2
		break;
	case 5:
		writeVDMode2Data(p_frame, (const uint8_t*)"          ");			//Rem3/4
		//memset(dch, ' ', YSF_CALLSIGN_LENGTH/2);
		//memcpy(dch + YSF_CALLSIGN_LENGTH/2, ysf_radioid, YSF_CALLSIGN_LENGTH/2);
		//writeVDMode2Data(frame, dch);	// Rem3/4
		break;
	case 6:
		writeVDMode2Data(p_frame, ft70d1);
		break;
	case 7:
		writeVDMode2Data(p_frame, dt2_temp);
		break;
	default:
		writeVDMode2Data(p_frame, (const uint8_t*)"          ");
	}
}

void YSF::writeDataFRModeData1(const uint8_t* dt, uint8_t* data)
{
	data += YSF_SYNC_LENGTH_BYTES + YSF_FICH_LENGTH_BYTES;

	uint8_t output[25U];
	for (uint32_t i = 0U; i < 20U; i++)
		output[i] = dt[i] ^ WHITENING_DATA[i];

	CCRC::addCCITT162(output, 22U);
	output[22U] = 0x00U;

	uint8_t convolved[45U];

	CYSFConvolution conv;
	conv.encode(output, convolved, 180U);

	uint8_t bytes[45U];
	uint32_t j = 0U;
	for (uint32_t i = 0U; i < 180U; i++) {
		uint32_t n = INTERLEAVE_TABLE_9_20[i];

		bool s0 = READ_BIT(convolved, j) != 0U;
		j++;

		bool s1 = READ_BIT(convolved, j) != 0U;
		j++;

		WRITE_BIT(bytes, n, s0);

		n++;
		WRITE_BIT(bytes, n, s1);
	}

	uint8_t* p1 = data;
	uint8_t* p2 = bytes;
	for (uint32_t i = 0U; i < 5U; i++) {
		::memcpy(p1, p2, 9U);
		p1 += 18U; p2 += 9U;
	}
}

void YSF::writeDataFRModeData2(const uint8_t* dt, uint8_t* data)
{
	data += YSF_SYNC_LENGTH_BYTES + YSF_FICH_LENGTH_BYTES;

	uint8_t output[25U];
	for (uint32_t i = 0U; i < 20U; i++)
		output[i] = dt[i] ^ WHITENING_DATA[i];

	CCRC::addCCITT162(output, 22U);
	output[22U] = 0x00U;

	uint8_t convolved[45U];

	CYSFConvolution conv;
	conv.encode(output, convolved, 180U);

	uint8_t bytes[45U];
	uint32_t j = 0U;
	for (uint32_t i = 0U; i < 180U; i++) {
		uint32_t n = INTERLEAVE_TABLE_9_20[i];

		bool s0 = READ_BIT(convolved, j) != 0U;
		j++;

		bool s1 = READ_BIT(convolved, j) != 0U;
		j++;

		WRITE_BIT(bytes, n, s0);

		n++;
		WRITE_BIT(bytes, n, s1);
	}

	uint8_t* p1 = data + 9U;
	uint8_t* p2 = bytes;
	for (uint32_t i = 0U; i < 5U; i++) {
		::memcpy(p1, p2, 9U);
		p1 += 18U; p2 += 9U;
	}
}

void YSF::ysf_scramble(uint8_t *buf, const int len)
{	// buffer is (de)scrambled in place
	static const uint8_t scramble_code[180] = {
	1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1,
	0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1,
	1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1,
	0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0,
	1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1,
	1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 1, 1,
	0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0,
	1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 1, 0,
	0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0
	};

	for (int i=0; i<len; i++) {
		buf[i] = buf[i] ^ scramble_code[i];
	}
}

void YSF::generate_vch_vd2(const uint8_t *a)
{
	uint8_t buf[104];
	uint8_t result[104];
	//uint8_t a[56];
	uint8_t vch[13];
	memset(vch, 0, 13);
/*
	for(int i = 0; i < 7; ++i){
		for(int j = 0; j < 8; ++j){
			a[(8*i)+j] = (1 & (input[i] >> (7-j)));
			//a[((8*i)+j)+1] = (1 & (data[5-i] >> j));
		}
	}
*/
	for (int i=0; i<27; i++) {
		buf[0+i*3] = a[i];
		buf[1+i*3] = a[i];
		buf[2+i*3] = a[i];
	}
	memcpy(buf+81, a+27, 22);
	buf[103] = 0;
	ysf_scramble(buf, 104);

	//uint8_t bit_result[104];
	int x=4;
	int y=26;
	for (int i=0; i<x; i++) {
		for (int j=0; j<y; j++) {
			result[i+j*x] = buf[j+i*y];
		}
	}
	for(int i = 0; i < 13; ++i){
		for(int j = 0; j < 8; ++j){
			//ambe_bytes[i] |= (ambe_frame[((8-i)*8)+(7-j)] << (7-j));
			vch[i] |= (result[(i*8)+j] << (7-j));
		}
	}
	::memcpy(m_vch, vch, 13);
}

void YSF::writeVDMode2Data(uint8_t* data, const uint8_t* dt)
{
	data += YSF_SYNC_LENGTH_BYTES + YSF_FICH_LENGTH_BYTES;
	
	uint8_t dt_tmp[13];
	::memcpy(dt_tmp, dt, YSF_CALLSIGN_LENGTH);

	for (uint32_t i = 0U; i < 10U; i++)
		dt_tmp[i] ^= WHITENING_DATA[i];

	CCRC::addCCITT162(dt_tmp, 12U);
	dt_tmp[12U] = 0x00U;

	uint8_t convolved[25U];
	CYSFConvolution conv;
	conv.start();
	conv.encode(dt_tmp, convolved, 100U);

	uint8_t bytes[25U];
	uint32_t j = 0U;
	for (uint32_t i = 0U; i < 100U; i++) {
		uint32_t n = INTERLEAVE_TABLE_5_20[i];

		bool s0 = READ_BIT(convolved, j) != 0U;
		j++;

		bool s1 = READ_BIT(convolved, j) != 0U;
		j++;

		WRITE_BIT(bytes, n, s0);

		n++;
		WRITE_BIT(bytes, n, s1);
	}

	uint8_t* p1 = data;
	uint8_t* p2 = bytes;
#ifdef SWDEBUG
	fprintf(stderr, "AMBE: ");
	for(int i = 0; i < 45; ++i){
		fprintf(stderr, "%02x ", m_ambe[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
	for (uint32_t i = 0U; i < 5U; i++) {
		::memcpy(p1, p2, 5U);
		if(m_hwtx){
			char ambe_bits[56];
			uint8_t di_bits[56];
			uint8_t *d = &m_ambe[7*i];
			for(int k = 0; k < 7; ++k){
				for(int j = 0; j < 8; j++){
					ambe_bits[j+(8*k)] = (1 & (d[k] >> (7 - j)));
					//ambe_bits[j+(8*ii)] = (1 & (d[ii] >> j));
				}
			}
			for(int k = 0; k < 49; ++k){
				di_bits[k] = ambe_bits[dvsi_interleave[k]];
			}
			generate_vch_vd2(di_bits);
		}
		else{
			uint8_t a[56];
			uint8_t *d = &m_ambe[7*i];
			for(int k = 0; k < 7; ++k){
				for(int j = 0; j < 8; ++j){
					a[(8*k)+j] = (1 & (d[k] >> (7-j)));
					//a[((8*i)+j)+1] = (1 & (data[5-i] >> j));
				}
			}
			generate_vch_vd2(a);
		}
		::memcpy(p1+5, m_vch, 13);
		p1 += 18U; p2 += 5U;
	}
}

void YSF::get_ambe()
{
#if !defined(Q_OS_IOS)
	uint8_t ambe[7];

	if(m_ambedev->get_ambe(ambe)){
		for(int i = 0; i < 7; ++i){
			m_txcodecq.append(ambe[i]);
		}
	}
#endif
}

void YSF::process_rx_data()
{
	int16_t pcm[160];
	uint8_t ambe[7];
	uint8_t imbe[11];
	static uint8_t cnt = 0;

	if(m_rxwatchdog++ > 20){
		qDebug() << "YSF RX stream timeout ";
		m_modeinfo.stream_state = STREAM_LOST;
		m_modeinfo.ts = QDateTime::currentMSecsSinceEpoch();
		emit update(m_modeinfo);
	}

	if((m_rxmodemq.size() > 2) && (++cnt >= 5)){
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

	if((!m_tx) && (m_rximbecodecq.size() > 10)){
		for(int i = 0; i < 11; ++i){
			imbe[i] = m_rximbecodecq.dequeue();
		}
		vocoder.decode_4400(pcm, imbe);
		m_audio->write(pcm, 160);
		emit update_output_level(m_audio->level());
	}

	else if((!m_tx) && (m_rxcodecq.size() > 6) ){
		for(int i = 0; i < 7; ++i){
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
#ifdef USE_MD380_VOCODER
                md380_decode(ambe, pcm);
#else
				m_mbevocoder->decode_2450(pcm, ambe);
#endif
			}
			else{
				memset(pcm, 0, 160 * sizeof(int16_t));
			}
			m_audio->write(pcm, 160);
			emit update_output_level(m_audio->level());
		}
	}

	else if ( ((m_modeinfo.stream_state == STREAM_END) || (m_modeinfo.stream_state == STREAM_LOST)) && (m_rxmodemq.size() < 100) ){
		m_rxtimer->stop();
		m_audio->stop_playback();
		m_rxwatchdog = 0;
		m_modeinfo.streamid = 0;
		m_rxcodecq.clear();
		m_rximbecodecq.clear();
		//m_ambedev->clear_queue();
		qDebug() << "YSF playback stopped";
		m_modeinfo.stream_state = STREAM_IDLE;
		return;
	}
}
