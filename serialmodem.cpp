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

#include <QMap>
#include <QThread>
#include <QDebug>
#include "serialmodem.h"

//#define DEBUGHW
#define ENDLINE "\n"

const unsigned char MODE_IDLE    = 0U;
const unsigned char MODE_DSTAR   = 1U;
const unsigned char MODE_DMR     = 2U;
const unsigned char MODE_YSF     = 3U;
const unsigned char MODE_P25     = 4U;
const unsigned char MODE_NXDN    = 5U;
const unsigned char MODE_CW      = 98U;
const unsigned char MODE_LOCKOUT = 99U;
const unsigned char MODE_ERROR   = 100U;

const unsigned char TAG_HEADER = 0x00U;
const unsigned char TAG_DATA   = 0x01U;
const unsigned char TAG_LOST   = 0x02U;
const unsigned char TAG_EOT    = 0x03U;
const unsigned char TAG_NODATA = 0x04U;

enum HW_TYPE {
	HWT_MMDVM,
	HWT_DVMEGA,
	HWT_MMDVM_ZUMSPOT,
	HWT_MMDVM_HS_HAT,
	HWT_NANO_HOTSPOT,
	HWT_MMDVM_HS,
	HWT_UNKNOWN
};

enum RPT_RF_STATE {
	RS_RF_LISTENING,
	RS_RF_LATE_ENTRY,
	RS_RF_AUDIO,
	RS_RF_DATA,
	RS_RF_REJECTED,
	RS_RF_INVALID
};

enum RPT_NET_STATE {
	RS_NET_IDLE,
	RS_NET_AUDIO,
	RS_NET_DATA
};

enum B_STATUS {
	BS_NO_DATA,
	BS_DATA,
	BS_MISSING
};

const unsigned char MMDVM_FRAME_START = 0xE0U;

const unsigned char MMDVM_GET_VERSION = 0x00U;
const unsigned char MMDVM_GET_STATUS  = 0x01U;
const unsigned char MMDVM_SET_CONFIG  = 0x02U;
const unsigned char MMDVM_SET_MODE    = 0x03U;
const unsigned char MMDVM_SET_FREQ    = 0x04U;

const unsigned char MMDVM_SEND_CWID   = 0x0AU;

const unsigned char MMDVM_DSTAR_HEADER = 0x10U;
const unsigned char MMDVM_DSTAR_DATA   = 0x11U;
const unsigned char MMDVM_DSTAR_LOST   = 0x12U;
const unsigned char MMDVM_DSTAR_EOT    = 0x13U;

const unsigned char MMDVM_DMR_DATA1   = 0x18U;
const unsigned char MMDVM_DMR_LOST1   = 0x19U;
const unsigned char MMDVM_DMR_DATA2   = 0x1AU;
const unsigned char MMDVM_DMR_LOST2   = 0x1BU;
const unsigned char MMDVM_DMR_SHORTLC = 0x1CU;
const unsigned char MMDVM_DMR_START   = 0x1DU;
const unsigned char MMDVM_DMR_ABORT   = 0x1EU;

const unsigned char MMDVM_YSF_DATA    = 0x20U;
const unsigned char MMDVM_YSF_LOST    = 0x21U;

const unsigned char MMDVM_P25_HDR     = 0x30U;
const unsigned char MMDVM_P25_LDU     = 0x31U;
const unsigned char MMDVM_P25_LOST    = 0x32U;

const unsigned char MMDVM_NXDN_DATA   = 0x40U;
const unsigned char MMDVM_NXDN_LOST   = 0x41U;

const unsigned char MMDVM_POCSAG_DATA = 0x50U;

const unsigned char MMDVM_FM_PARAMS1  = 0x60U;
const unsigned char MMDVM_FM_PARAMS2  = 0x61U;
const unsigned char MMDVM_FM_PARAMS3  = 0x62U;

const unsigned char MMDVM_ACK         = 0x70U;
const unsigned char MMDVM_NAK         = 0x7FU;

const unsigned char MMDVM_SERIAL      = 0x80U;

const unsigned char MMDVM_TRANSPARENT = 0x90U;
const unsigned char MMDVM_QSO_INFO    = 0x91U;

const unsigned char MMDVM_DEBUG1      = 0xF1U;
const unsigned char MMDVM_DEBUG2      = 0xF2U;
const unsigned char MMDVM_DEBUG3      = 0xF3U;
const unsigned char MMDVM_DEBUG4      = 0xF4U;
const unsigned char MMDVM_DEBUG5      = 0xF5U;

const unsigned int MAX_RESPONSES = 30U;

const unsigned int BUFFER_LENGTH = 2000U;

SerialModem::SerialModem(QString protocol)
{
	set_mode(protocol);
	m_dmrDelay = 0;
	m_debug = false;
	m_dmrColorCode = 1;
}

SerialModem::~SerialModem()
{
	m_serial->close();
}

void SerialModem::set_mode(QString m)
{
	m_dstarEnabled = 0;
	m_dmrEnabled = 0;
	m_ysfEnabled = 0;
	m_p25Enabled = 0;
	m_nxdnEnabled = 0;
	m_pocsagEnabled = 0;

	if((m == "REF") || (m == "DCS") || (m == "XRF")){
		m_dstarEnabled = 1;
	}
	else if(m == "DMR"){
		m_dmrEnabled = 1;
	}
	else if((m == "YSF") || (m == "FCS")){
		m_ysfEnabled = 1;
	}
	else if(m == "P25"){
		m_p25Enabled = 1;
	}
	else if(m == "NXDN"){
		m_nxdnEnabled = 1;
	}
}

void SerialModem::set_modem_flags(bool rxInvert, bool txInvert, bool pttInvert, bool useCOSAsLockout, bool duplex)
{
	m_rxInvert = rxInvert;
	m_txInvert = txInvert;
	m_pttInvert = pttInvert;
	m_useCOSAsLockout = useCOSAsLockout;
	m_duplex = duplex;
	m_ysfLoDev = 0;
}

void SerialModem::set_modem_params(uint32_t rxfreq, uint32_t txfreq, uint32_t txDelay, float rxLevel, float rfLevel, uint32_t ysfTXHang, float cwIdTXLevel, float dstarTXLevel, float dmrTXLevel, float ysfTXLevel, float p25TXLevel, float nxdnTXLevel, float pocsagTXLevel)
{
	m_rxfreq = rxfreq;
	m_txfreq = txfreq;
	m_txDelay = txDelay;
	m_rxLevel = rxLevel;
	m_rfLevel = rfLevel;
	m_cwIdTXLevel = cwIdTXLevel;
	m_dstarTXLevel = dstarTXLevel;
	m_dmrTXLevel = dmrTXLevel;
	m_ysfTXLevel = ysfTXLevel;
	m_p25TXLevel = p25TXLevel;
	m_nxdnTXLevel = nxdnTXLevel;
	m_pocsagTXLevel = pocsagTXLevel;
	m_ysfTXHang = ysfTXHang;
}

void SerialModem::connect_to_serial(QString p)
{
#ifndef Q_OS_ANDROID
		m_serial = new QSerialPort;
#else
		m_serial = &AndroidSerialPort::GetInstance();
#endif
	m_serial->setPortName(p);
	m_serial->setBaudRate(115200);
	m_serial->setDataBits(QSerialPort::Data8);
	m_serial->setStopBits(QSerialPort::OneStop);
	m_serial->setParity(QSerialPort::NoParity);
		//out << "Baud rate == " << serial->baudRate() << endl;
	if (m_serial->open(QIODevice::ReadWrite)) {
		m_modemtimer = new QTimer();
		connect(m_modemtimer, SIGNAL(timeout()), this, SLOT(process_modem()));
		m_modemtimer->start(19);
#ifndef Q_OS_ANDROID
		connect(m_serial, &QSerialPort::readyRead, this, &SerialModem::process_serial);
#else
		//connect(m_serial, &AndroidSerialPort::readyRead, this, &SerialModem::process_serial);
		connect(m_serial, SIGNAL(data_received(QByteArray)), this, SLOT(receive_serial(QByteArray)));
#endif
		m_serial->setFlowControl(QSerialPort::HardwareControl);
		m_serial->setRequestToSend(true);
		QByteArray a;
		a.clear();
		a.append(MMDVM_FRAME_START);
		a.append(3);
		a.append(MMDVM_GET_VERSION);
		m_serial->write(a);
#ifdef DEBUGHW
			fprintf(stderr, "MODEMTX %d:%d:", a.size(), m_serialdata.size());
			for(int i = 0; i < a.size(); ++i){
				//if((d.data()[i] == 0x61) && (data.data()[i+1] == 0x01) && (data.data()[i+2] == 0x42) && (data.data()[i+3] == 0x02)){
				//	i+= 6;
				//}
				fprintf(stderr, "%02x ", (unsigned char)a.data()[i]);
			}
			fprintf(stderr, "\n");
			fflush(stderr);
#endif
	}
}

void SerialModem::receive_serial(QByteArray d)
{
	for(int i = 0; i < d.size(); i++){
		m_serialdata.enqueue(d[i]);
	}
	//qDebug() << "SerialModem::process_serial()" << d.toHex();
}

void SerialModem::process_serial()
{
	QByteArray d = m_serial->readAll();

	for(int i = 0; i < d.size(); i++){
		m_serialdata.enqueue(d[i]);
	}
	//qDebug() << "SerialModem::process_serial()" << d.toHex();
#ifdef DEBUGHW
	fprintf(stderr, "MODEMRX %d:%d:", d.size(), m_serialdata.size());
	for(int i = 0; i < d.size(); ++i){
		//if((d.data()[i] == 0x61) && (data.data()[i+1] == 0x01) && (data.data()[i+2] == 0x42) && (data.data()[i+3] == 0x02)){
		//	i+= 6;
		//}
		fprintf(stderr, "%02x ", (unsigned char)d.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
}

void SerialModem::process_modem()
{
	QByteArray out;
	if(m_serialdata.size() < 3){
		return;
	}
	//qDebug() << "process_modem() " << (uint8_t)m_serialdata[0] << ":" << (uint8_t)m_serialdata[1] << ":" << m_serialdata.size();
	if(((uint8_t)m_serialdata[0] == MMDVM_FRAME_START) && (m_serialdata.size() >= m_serialdata[1])){
		const uint8_t r = (uint8_t)m_serialdata[2];
		const uint8_t s = (uint8_t)m_serialdata[1];

		if(m_serialdata.size() >= m_serialdata[1]){
			for(int i = 0; i < s; ++i){
				out.append(m_serialdata.dequeue());
			}
			emit modem_data_ready(out);
		}

		if(r == MMDVM_GET_VERSION){
			QThread::msleep(100);
			set_freq();
			QThread::msleep(100);
			set_config();
		}
	}
}

void SerialModem::set_freq()
{
	QByteArray out;
	uint32_t pfreq = 433000000U;
	qDebug() << "set_freq() rx:tx == " << m_rxfreq << ":" << m_txfreq;
	out.clear();
	out.append(MMDVM_FRAME_START);
	out.append(17);
	out.append(MMDVM_SET_FREQ);
	out.append('\00');
	out.append((m_rxfreq >> 0) & 0xFFU);
	out.append((m_rxfreq >> 8) & 0xFFU);
	out.append((m_rxfreq >> 16) & 0xFFU);
	out.append((m_rxfreq >> 24) & 0xFFU);
	out.append((m_txfreq >> 0) & 0xFFU);
	out.append((m_txfreq >> 8) & 0xFFU);
	out.append((m_txfreq >> 16) & 0xFFU);
	out.append((m_txfreq >> 24) & 0xFFU);
	out.append((unsigned char)(m_rfLevel * 2.55F + 0.5F));
	out.append((pfreq >> 0) & 0xFFU);
	out.append((pfreq >> 8) & 0xFFU);
	out.append((pfreq >> 16) & 0xFFU);
	out.append((pfreq >> 24) & 0xFFU);
	m_serial->write(out);
#ifdef DEBUGHW
	fprintf(stderr, "MODEMTX %d:%d:", out.size(), m_serialdata.size());
	for(int i = 0; i < out.size(); ++i){
		//if((d.data()[i] == 0x61) && (data.data()[i+1] == 0x01) && (data.data()[i+2] == 0x42) && (data.data()[i+3] == 0x02)){
		//	i+= 6;
		//}
		fprintf(stderr, "%02x ", (unsigned char)out.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
}

void SerialModem::set_config()
{
	QByteArray out;
	uint8_t c;

	out.clear();

	out.append(MMDVM_FRAME_START);
	out.append(24U);
	out.append(MMDVM_SET_CONFIG);

	c = 0x00U;
	if (m_rxInvert)
		c |= 0x01U;
	if (m_txInvert)
		c |= 0x02U;
	if (m_pttInvert)
		c |= 0x04U;
	if (m_ysfLoDev)
		c |= 0x08U;
	if (m_debug)
		c |= 0x10U;
	if (m_useCOSAsLockout)
		c |= 0x20U;
	if (!m_duplex)
		c |= 0x80U;

	out.append(c);

	c = 0x00U;
	if (m_dstarEnabled)
		c |= 0x01U;
	if (m_dmrEnabled)
		c |= 0x02U;
	if (m_ysfEnabled)
		c |= 0x04U;
	if (m_p25Enabled)
		c |= 0x08U;
	if (m_nxdnEnabled)
		c |= 0x10U;
	if (m_pocsagEnabled)
		c |= 0x20U;

	out.append(c);

	out.append(m_txDelay / 10U);		// In 10ms units
	out.append(MODE_IDLE);
	out.append((unsigned char)(m_rxLevel * 2.55F + 0.5F));
	out.append((unsigned char)(m_cwIdTXLevel * 2.55F + 0.5F));
	out.append(m_dmrColorCode);
	out.append(m_dmrDelay);
	out.append(128U);           // Was OscOffset
	out.append((unsigned char)(m_dstarTXLevel * 2.55F + 0.5F));
	out.append((unsigned char)(m_dmrTXLevel * 2.55F + 0.5F));
	out.append((unsigned char)(m_ysfTXLevel * 2.55F + 0.5F));
	out.append((unsigned char)(m_p25TXLevel * 2.55F + 0.5F));
	out.append((unsigned char)(m_txDCOffset + 128));
	out.append((unsigned char)(m_rxDCOffset + 128));
	out.append((unsigned char)(m_nxdnTXLevel * 2.55F + 0.5F));
	out.append((unsigned char)m_ysfTXHang);
	out.append((unsigned char)(m_pocsagTXLevel * 2.55F + 0.5F));
	out.append((unsigned char)(m_fmTXLevel * 2.55F + 0.5F));
	out.append((unsigned char)m_p25TXHang);
	out.append((unsigned char)m_nxdnTXHang);
	m_serial->write(out);
#ifdef DEBUGHW
	fprintf(stderr, "MODEMTX %d:%d:", out.size(), m_serialdata.size());
	for(int i = 0; i < out.size(); ++i){
		//if((d.data()[i] == 0x61) && (data.data()[i+1] == 0x01) && (data.data()[i+2] == 0x42) && (data.data()[i+3] == 0x02)){
		//	i+= 6;
		//}
		fprintf(stderr, "%02x ", (unsigned char)out.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
}

void SerialModem::set_mode(uint8_t m)
{
	QByteArray out;
	out.clear();
	out.append(MMDVM_FRAME_START);
	out.append(4);
	out.append(MMDVM_SET_MODE);
	out.append(m);
	m_serial->write(out);
}

void SerialModem::write(QByteArray b)
{
	m_serial->write(b);
#ifdef DEBUGHW
	fprintf(stderr, "MODEMTX %d:%d:", b.size(), m_serialdata.size());
	for(int i = 0; i < b.size(); ++i){
		fprintf(stderr, "%02x ", (unsigned char)b.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
}
