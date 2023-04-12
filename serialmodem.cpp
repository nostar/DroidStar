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
#include "MMDVMDefines.h"

//#define DEBUGHW

SerialModem::SerialModem(QString mode)
{
	set_mode(mode);
	m_dmrDelay = 0;
	m_debug = false;
	m_dmrColorCode = 1;
	m_m17TXHang = 5;
	m_ax25Enabled = false;
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
	m_m17Enabled = 0;

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
	else if(m == "M17"){
		m_m17Enabled = 1;
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

void SerialModem::set_modem_params(uint32_t baudrate, uint32_t rxfreq, uint32_t txfreq, uint32_t txDelay, float rxLevel, float rfLevel, uint32_t ysfTXHang, float cwIdTXLevel, float dstarTXLevel, float dmrTXLevel, float ysfTXLevel, float p25TXLevel, float nxdnTXLevel, float pocsagTXLevel, float m17TXLevel)
{
	m_baudrate = baudrate;
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
	m_m17TXLevel = m17TXLevel;
	m_ysfTXHang = ysfTXHang;
}

void SerialModem::connect_to_serial(QString p)
{
#ifndef Q_OS_ANDROID
		m_serial = new QSerialPort;
#else
		m_serial = &AndroidSerialPort::GetInstance();
        connect(m_serial, SIGNAL(device_ready()), this, SLOT(config_modem()));
#endif
	m_serial->setPortName(p);
	m_serial->setBaudRate(m_baudrate);
	m_serial->setDataBits(QSerialPort::Data8);
	m_serial->setStopBits(QSerialPort::OneStop);
	m_serial->setParity(QSerialPort::NoParity);

	if (m_serial->open(QIODevice::ReadWrite)) {
		m_modemtimer = new QTimer();
		connect(m_modemtimer, SIGNAL(timeout()), this, SLOT(process_modem()));
		m_modemtimer->start(19);
#ifndef Q_OS_ANDROID
		connect(m_serial, &QSerialPort::readyRead, this, &SerialModem::process_serial);
        config_modem();
#else
		connect(m_serial, SIGNAL(data_received(QByteArray)), this, SLOT(receive_serial(QByteArray)));
#endif
		m_serial->setRequestToSend(true);
	}
}

void SerialModem::config_modem()
{
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
        fprintf(stderr, "%02x ", (uint8_t)a.data()[i]);
    }
    fprintf(stderr, "\n");
    fflush(stderr);
#endif
    emit modem_ready();
}

void SerialModem::receive_serial(QByteArray d)
{
	for(int i = 0; i < d.size(); i++){
		m_serialdata.enqueue(d[i]);
	}
}

void SerialModem::process_serial()
{
	QByteArray d = m_serial->readAll();

	for(int i = 0; i < d.size(); i++){
		m_serialdata.enqueue(d[i]);
	}
#ifdef DEBUGHW
	fprintf(stderr, "MODEMRX %d:%d:", d.size(), m_serialdata.size());
	for(int i = 0; i < d.size(); ++i){
		//if((d.data()[i] == 0x61) && (data.data()[i+1] == 0x01) && (data.data()[i+2] == 0x42) && (data.data()[i+3] == 0x02)){
		//	i+= 6;
		//}
		fprintf(stderr, "%02x ", (uint8_t)d.data()[i]);
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

	if(((uint8_t)m_serialdata[0] == MMDVM_FRAME_START) && (m_serialdata.size() >= m_serialdata[1])){
		const uint8_t r = (uint8_t)m_serialdata[2];
		const uint8_t s = (uint8_t)m_serialdata[1];

		if(r == MMDVM_NAK){
			qDebug() << "Received MMDVM_NAK";
			for(int i = 0; i < s; ++i){
				m_serialdata.dequeue();
			}
			if( (m_serialdata.size() > 3) && (m_serialdata[3] == 2) ){

			}
		}

		else if(r == MMDVM_ACK){
			qDebug() << "Received MMDVM_ACK";
			if( (m_serialdata.size() > 3) && (m_serialdata[3] == 2) ){
				emit connected(true);
			}
			for(int i = 0; i < s; ++i){
				m_serialdata.dequeue();
			}
		}

		else if(r == MMDVM_GET_VERSION){
			if(m_serialdata.size() >= s){
				m_protocol = m_serialdata[3];
				m_version.clear();
				uint8_t desc_offset = (m_protocol == 2) ? 23 : 4;

				for(int i = 0; i < (s-desc_offset); ++i){
					m_version.append(m_serialdata[desc_offset+i]);
				}
				qDebug() << "MMDVM Protocol " << m_protocol << ": " << m_version;
			}
			QThread::msleep(100);
			set_freq();
			QThread::msleep(100);
			set_config();
			for(int i = 0; i < s; ++i){
				m_serialdata.dequeue();
			}
		}

		else if(m_serialdata.size() >= s){
			for(int i = 0; i < s; ++i){
				out.append(m_serialdata.dequeue());
			}
			emit modem_data_ready(out);
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
	out.append((uint8_t)(m_rfLevel * 2.55F + 0.5F));
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
		fprintf(stderr, "%02x ", (uint8_t)out.data()[i]);
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

	if(m_protocol == 1){
		out.append(26U);
	}
	else if(m_protocol == 2){
		out.append(40U);
	}

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
	if (m_m17Enabled)
		c |= 0x40U;

	out.append(c);


	if(m_protocol == 1){
		out.append(m_txDelay / 10U);		// In 10ms units
		out.append(MODE_IDLE);
		out.append((uint8_t)(m_rxLevel * 2.55F + 0.5F));
		out.append((uint8_t)(m_cwIdTXLevel * 2.55F + 0.5F));
		out.append(m_dmrColorCode);
		out.append(m_dmrDelay);
		out.append(128U);           // Was OscOffset
		out.append((uint8_t)(m_dstarTXLevel * 2.55F + 0.5F));
		out.append((uint8_t)(m_dmrTXLevel * 2.55F + 0.5F));
		out.append((uint8_t)(m_ysfTXLevel * 2.55F + 0.5F));
		out.append((uint8_t)(m_p25TXLevel * 2.55F + 0.5F));
		out.append((uint8_t)(m_txDCOffset + 128));
		out.append((uint8_t)(m_rxDCOffset + 128));
		out.append((uint8_t)(m_nxdnTXLevel * 2.55F + 0.5F));
		out.append((uint8_t)m_ysfTXHang);
		out.append((uint8_t)(m_pocsagTXLevel * 2.55F + 0.5F));
		out.append((uint8_t)(m_fmTXLevel * 2.55F + 0.5F));
		out.append((uint8_t)m_p25TXHang);
		out.append((uint8_t)m_nxdnTXHang);
		out.append((uint8_t)(m_m17TXLevel * 2.55F + 0.5F));
		out.append((uint8_t)m_m17TXHang);
	}
	else if(m_protocol == 2){
		c = 0x00U;
		if (m_pocsagEnabled)
			c |= 0x01U;
		if (m_ax25Enabled)
			c |= 0x02U;
		out.append(c);
		out.append(m_txDelay / 10U);
		out.append(MODE_IDLE);
		out.append((uint8_t)(m_txDCOffset + 128));
		out.append((uint8_t)(m_rxDCOffset + 128));
		out.append((uint8_t)(m_rxLevel * 2.55F + 0.5F));
		out.append((uint8_t)(m_cwIdTXLevel * 2.55F + 0.5F));
		out.append((uint8_t)(m_dstarTXLevel * 2.55F + 0.5F));
		out.append((uint8_t)(m_dmrTXLevel * 2.55F + 0.5F));
		out.append((uint8_t)(m_ysfTXLevel * 2.55F + 0.5F));
		out.append((uint8_t)(m_p25TXLevel * 2.55F + 0.5F));
		out.append((uint8_t)(m_nxdnTXLevel * 2.55F + 0.5F));
		out.append((uint8_t)(m_m17TXLevel * 2.55F + 0.5F));
		out.append((uint8_t)(m_pocsagTXLevel * 2.55F + 0.5F));
		out.append((uint8_t)(m_fmTXLevel * 2.55F + 0.5F));
		out.append((uint8_t)(m_ax25TXLevel * 2.55F + 0.5F));
		out.append('\00');
		out.append('\00');
		out.append((uint8_t)m_ysfTXHang);
		out.append((uint8_t)m_p25TXHang);
		out.append((uint8_t)m_nxdnTXHang);
		out.append((uint8_t)m_m17TXHang);
		out.append('\00');
		out.append('\00');
		out.append(m_dmrColorCode);
		out.append(m_dmrDelay);
		out.append((uint8_t)(m_ax25RXTwist + 128));
		out.append(m_ax25TXDelay / 10U);
		out.append(m_ax25SlotTime / 10U);
		out.append(m_ax25PPersist);
		out.append('\00');
		out.append('\00');
		out.append('\00');
		out.append('\00');
		out.append('\00');
	}

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
