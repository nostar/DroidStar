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
#include <QtMath>
#ifndef Q_OS_ANDROID
#include <QSerialPortInfo>
#endif
#include <algorithm>
#include "serialambe.h"

#define ENDLINE "\n"

//#define DEBUG

#define AMBE3000_START_BYTE		0x61
#define AMBE3000_TYPE_CONFIG	0x00
#define AMBE3000_TYPE_CHANNEL	0x01
#define AMBE3000_TYPE_SPEECH	0x02
#define AMBE3000_PKT_RATEP		0x0a
#define AMBE3000_PKT_INIT		0x0b
#define AMBE3000_PKT_RATEP		0x0a
#define AMBE3000_PKT_PRODID		0x30
#define AMBE3000_PKT_VERSTRING	0x31
#define AMBE3000_PKT_READY		0x39
#define AMBE3000_PKT_RESET		0x33
#define AMBE3000_PKT_PARITYMODE	0x3f

const uint8_t AMBEP251_4400_2800[17] = {AMBE3000_START_BYTE, 0x00, 0x0d, AMBE3000_TYPE_CONFIG, AMBE3000_PKT_RATEP, 0x05U, 0x58U, 0x08U, 0x6BU, 0x10U, 0x30U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U, 0x90U};		//DVSI P25 USB Dongle FEC
const uint8_t AMBE2000_2400_1200[17] = {AMBE3000_START_BYTE, 0x00, 0x0d, AMBE3000_TYPE_CONFIG, AMBE3000_PKT_RATEP, 0x01U, 0x30U, 0x07U, 0x63U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x48U};
const uint8_t AMBE3000_2450_1150[17] = {AMBE3000_START_BYTE, 0x00, 0x0d, AMBE3000_TYPE_CONFIG, AMBE3000_PKT_RATEP, 0x04U, 0x31U, 0x07U, 0x54U, 0x24U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x6fU, 0x48U};
const uint8_t AMBE3000_2450_0000[17] = {AMBE3000_START_BYTE, 0x00, 0x0d, AMBE3000_TYPE_CONFIG, AMBE3000_PKT_RATEP, 0x04U, 0x31U, 0x07U, 0x54U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x70U, 0x31U};
const uint8_t AMBE3000_PARITY_DISABLE[8] = {AMBE3000_START_BYTE, 0x00, 0x04, AMBE3000_TYPE_CONFIG, AMBE3000_PKT_PARITYMODE, 0x00, 0x2f, 0x14};
const uint8_t AMBE3000_PRODID[5] = {AMBE3000_START_BYTE, 0x00, 0x01, AMBE3000_TYPE_CONFIG, AMBE3000_PKT_PRODID};
const uint8_t AMBE3000_VERSION[5] = {AMBE3000_START_BYTE, 0x00, 0x01, AMBE3000_TYPE_CONFIG, AMBE3000_PKT_VERSTRING};

//const uint8_t AMBE2020[48] = {0x13, 0xec, 0x00, 0x00, 0x10, 0x30, 0x00, 0x01, 0x00, 0x00, 0x42, 0x30, 0x00, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
//const uint8_t AMBE2020[4] = {0x04, 0x20, 0x01, 0x00};
const uint8_t AMBE2020[5] = {0x05, 0x00, 0x18, 0x00, 0x01};
SerialAMBE::SerialAMBE(QString protocol) :
	m_protocol(protocol),
	m_decode_gain(1.0)
{
}

SerialAMBE::~SerialAMBE()
{
	m_serial->close();
}

QMap<QString, QString> SerialAMBE::discover_devices()
{
	QMap<QString, QString> devlist;
	const QString blankString = "N/A";
	QString out;
#ifndef Q_OS_ANDROID
	const auto serialPortInfos = QSerialPortInfo::availablePorts();

	if(serialPortInfos.count()){
		for(const QSerialPortInfo &serialPortInfo : serialPortInfos) {
			out = "Port: " + serialPortInfo.portName() + ENDLINE
				+ "Location: " + serialPortInfo.systemLocation() + ENDLINE
				+ "Description: " + (!serialPortInfo.description().isEmpty() ? serialPortInfo.description() : blankString) + ENDLINE
				+ "Manufacturer: " + (!serialPortInfo.manufacturer().isEmpty() ? serialPortInfo.manufacturer() : blankString) + ENDLINE
				+ "Serial number: " + (!serialPortInfo.serialNumber().isEmpty() ? serialPortInfo.serialNumber() : blankString) + ENDLINE
				+ "Vendor Identifier: " + (serialPortInfo.hasVendorIdentifier() ? QByteArray::number(serialPortInfo.vendorIdentifier(), 16) : blankString) + ENDLINE
				+ "Product Identifier: " + (serialPortInfo.hasProductIdentifier() ? QByteArray::number(serialPortInfo.productIdentifier(), 16) : blankString) + ENDLINE;
            //fprintf(stderr, "%s", out.toStdString().c_str());fflush(stderr);
			devlist[serialPortInfo.systemLocation()] = serialPortInfo.description() + ":" + serialPortInfo.systemLocation();
		}
	}
#else
	QStringList list = AndroidSerialPort::GetInstance().discover_devices();
	for ( const auto& i : list  ){
		devlist[i] = i;
	}
#endif
return devlist;
}

void SerialAMBE::connect_to_serial(QString p)
{
	const QString blankString = "N/A";
	int br = 460800;

	if((m_protocol != "P25") && (m_protocol != "M17") && (p != "")){
#ifndef Q_OS_ANDROID
		m_serial = new QSerialPort;
		QSerialPortInfo info(*m_serial);
		QString out = "Port: " + info.portName() + ENDLINE
			+ "Location: " + info.systemLocation() + ENDLINE
			+ "Description: " + (!info.description().isEmpty() ? info.description() : blankString) + ENDLINE
			+ "Manufacturer: " + (!info.manufacturer().isEmpty() ? info.manufacturer() : blankString) + ENDLINE
			+ "Serial number: " + (!info.serialNumber().isEmpty() ? info.serialNumber() : blankString) + ENDLINE
			+ "Vendor Identifier: " + (info.hasVendorIdentifier() ? QByteArray::number(info.vendorIdentifier(), 16) : blankString) + ENDLINE
			+ "Product Identifier: " + (info.hasProductIdentifier() ? QByteArray::number(info.productIdentifier(), 16) : blankString) + ENDLINE;
		fprintf(stderr, "%s", out.toStdString().c_str());fflush(stderr);
		m_description = info.description();

		if(m_description == "DV Dongle"){
			br = 230400;
		}

#else
		m_serial = &AndroidSerialPort::GetInstance();
        connect(m_serial, SIGNAL(device_ready()), this, SLOT(config_ambe()));
        //connect(m_serial, SIGNAL(device_denied()), this, SLOT(config_ambe()));
#endif
		m_serial->setPortName(p);
		m_serial->setBaudRate(br);
		m_serial->setDataBits(QSerialPort::Data8);
		m_serial->setStopBits(QSerialPort::OneStop);
		m_serial->setParity(QSerialPort::NoParity);

		if (m_serial->open(QIODevice::ReadWrite)) {
#ifndef Q_OS_ANDROID
			connect(m_serial, &QSerialPort::readyRead, this, &SerialAMBE::process_serial);
            config_ambe();
#else
			connect(m_serial, SIGNAL(data_received(QByteArray)), this, SLOT(receive_serial(QByteArray)));
#endif
		}
		else{
			qDebug() << "Error: Failed to open device.";
		}
	}
}

void SerialAMBE::config_ambe()
{
    QByteArray a;
    a.clear();
    if(m_description != "DV Dongle"){
        m_serial->setFlowControl(QSerialPort::HardwareControl);
        m_serial->setRequestToSend(true);
        a.append(reinterpret_cast<const char*>(AMBE3000_PARITY_DISABLE), sizeof(AMBE3000_PARITY_DISABLE));
        m_serial->write(a);
        QThread::msleep(100);
        a.clear();
        a.append(reinterpret_cast<const char*>(AMBE3000_PRODID), sizeof(AMBE3000_PRODID));
        m_serial->write(a);
        QThread::msleep(100);
        a.clear();
        a.append(reinterpret_cast<const char*>(AMBE3000_VERSION), sizeof(AMBE3000_VERSION));
        m_serial->write(a);
        QThread::msleep(100);
        a.clear();
    }

    if(m_protocol == "DMR"){
        a.append(reinterpret_cast<const char*>(AMBE3000_2450_1150), sizeof(AMBE3000_2450_1150));
        packet_size = 9;
    }
    else if( (m_protocol == "YSF") || (m_protocol == "NXDN") ){
        a.append(reinterpret_cast<const char*>(AMBE3000_2450_0000), sizeof(AMBE3000_2450_0000));
        packet_size = 7;
    }
    else if(m_protocol == "P25"){
        a.append(reinterpret_cast<const char*>(AMBEP251_4400_2800), sizeof(AMBEP251_4400_2800));
    }
    else if(m_description != "DV Dongle"){ //D-Star with AMBE3000
        a.append(reinterpret_cast<const char*>(AMBE2000_2400_1200), sizeof(AMBE2000_2400_1200));
        packet_size = 9;
    }
    else{
        a.append(reinterpret_cast<const char*>(AMBE2020), sizeof(AMBE2020));
        packet_size = 9;
    }
    m_serial->write(a);
#ifdef DEBUG
    fprintf(stderr, "SENDHW %d:%d:", a.size(), m_serialdata.size());
    for(int i = 0; i < a.size(); ++i){
        //if((d.data()[i] == 0x61) && (data.data()[i+1] == 0x01) && (data.data()[i+2] == 0x42) && (data.data()[i+3] == 0x02)){
        //	i+= 6;
        //}
        fprintf(stderr, "%02x ", (unsigned char)a.data()[i]);
    }
    fprintf(stderr, "\n");
    fflush(stderr);
#endif
    emit ambedev_ready();
}

void SerialAMBE::receive_serial(QByteArray d)
{
	for(int i = 0; i < d.size(); i++){
		m_serialdata.append(d[i]);
	}

	if(m_description == "DV Dongle"){
		process_serial_2020();
	}
	else{
		process_serial_3000();
	}
}

void SerialAMBE::process_serial()
{
	QByteArray d = m_serial->readAll();
	for(int i = 0; i < d.size(); i++){
		m_serialdata.append(d[i]);
	}
#ifdef DEBUG
	fprintf(stderr, "AMBEHW %d:%d:", d.size(), m_serialdata.size());
	for(int i = 0; i < d.size(); ++i){
		//if((d.data()[i] == 0x61) && (data.data()[i+1] == 0x01) && (data.data()[i+2] == 0x42) && (data.data()[i+3] == 0x02)){
		//	i+= 6;
		//}
		fprintf(stderr, "%02x ", (unsigned char)d.data()[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
#endif

	if(m_description == "DV Dongle"){
		process_serial_2020();
	}
	else{
		process_serial_3000();
	}
}

void SerialAMBE::decode(uint8_t *ambe)
{
	if(m_description == "DV Dongle"){
		decode_2020(ambe);
	}
	else{
		decode_3000(ambe);
	}
}

void SerialAMBE::encode(int16_t *audio)
{
	uint8_t packet[327] = {AMBE3000_START_BYTE, 0x01, 0x43, AMBE3000_TYPE_SPEECH, 0x40, 0x00, 0xa0};
	for(int i = 0; i < 160; ++i){
		packet [(i*2)+7] = (audio[i] >> 8) & 0xff;
		packet [(i*2)+8] = audio[i] & 0xff;
	}
	m_serial->write((char *)packet, 327);
#ifdef DEBUG
			fprintf(stderr, "SENDHW: ");
			for(int i = 0; i < 326; ++i){
				//if((d.data()[i] == 0x61) && (data.data()[i+1] == 0x01) && (data.data()[i+2] == 0x42) && (data.data()[i+3] == 0x02)){
				//	i+= 6;
				//}
				fprintf(stderr, "%02x ", packet[i]);
			}
			fprintf(stderr, "\n");
			fflush(stderr);
#endif
}

void SerialAMBE::decode_2020(uint8_t *ambe)
{
	uint8_t packet[50] = {0x32, 0xa0, 0xec, 0x13, 0x00, 0x00, 0x30, 0x10, 0x01, 0x00, 0x00, 0x00, 0x30, 0x42, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	uint8_t pcm[322];
	memset(pcm, 0, 322);
	pcm[0] = 0x42;
	pcm[1] = 0x81;
	memcpy(packet+24, ambe, packet_size);
	m_serial->write((char *)packet, 50);
	m_serial->write((char *)pcm, 322);
}

void SerialAMBE::decode_3000(uint8_t *ambe)
{
	uint8_t packet[15] = {AMBE3000_START_BYTE, 0x00, 0x0b, AMBE3000_TYPE_CHANNEL, 0x01, 0x48};
	if( packet_size == 7 ){
		packet[2] = 0x09;
		packet[5] = 0x31;
	}
	memcpy(packet+6, ambe, packet_size);

	m_serial->write((char *)packet, (6 + packet_size));
}

void SerialAMBE::process_serial_2020()
{
	if( (m_serialdata.size() > 321) &&
		((uint8_t)m_serialdata[0] == 0x42) &&
		((uint8_t)m_serialdata[1] == 0x81)
		)
	{
		emit data_ready();
	}
	if( (m_serialdata.size() > 49) &&
		((uint8_t)m_serialdata[0] == 0x32) &&
		((uint8_t)m_serialdata[1] == 0xa0) &&
		((uint8_t)m_serialdata[0] == 0xec) &&
		((uint8_t)m_serialdata[1] == 0x13)
		)
	{
		emit data_ready();
	}
}

void SerialAMBE::process_serial_3000()
{
	if(m_serialdata.size() < 3){
		return;
	}

	while( (m_serialdata.size() > 3) &&
		   (m_serialdata[0] == AMBE3000_START_BYTE) &&
		   (m_serialdata[3] == 0x00) &&
		   (m_serialdata.size() >= m_serialdata[2])
		)
	{
		switch(m_serialdata[4]){
		case AMBE3000_PKT_PARITYMODE:
			if(!m_serialdata[5]){
				qDebug() << "AMBE3000 Parity disabled";
			}
			else{
				qDebug() << "ERROR: AMBE3000 Parity not disabled";
			}
			break;
		case AMBE3000_PKT_PRODID:
			m_ambeprodid.clear();

			for(int i = 0; i < (m_serialdata[2] - 2); ++i){
				m_ambeprodid.append(m_serialdata[5+i]);
			}

			qDebug() << "PRODID == " << m_ambeprodid;
			break;
		case AMBE3000_PKT_VERSTRING:
			m_ambeverstring.clear();

			for(int i = 0; i < (m_serialdata[2] - 2); ++i){
				m_ambeverstring.append(m_serialdata[5+i]);
			}

			qDebug() << "VERSTRING == " << m_ambeverstring;
			break;
		case AMBE3000_PKT_RATEP:
			if(!m_serialdata[5]){
				qDebug() << "AMBE3000 Rate set";
				emit connected(true);
			}
			else{
				qDebug() << "ERROR: AMBE3000 Rate not set";
				emit connected(false);
			}
			break;
		default:
			break;
		}

		do {
			m_serialdata.dequeue();
		}
		while(m_serialdata.size() && m_serialdata[0] != AMBE3000_START_BYTE);
	}

	if( (m_serialdata.size() >= (6 + packet_size)) &&
		(m_serialdata[0] == AMBE3000_START_BYTE) &&
		(m_serialdata[3] == AMBE3000_TYPE_CHANNEL)
		)
	{
		emit data_ready();
	}
}

bool SerialAMBE::get_ambe(uint8_t *ambe)
{
	bool r = false;
	if(m_serialdata.isEmpty()){
		return r;
	}

	if( (m_serialdata.size() > 3) &&
		(m_serialdata[0] == AMBE3000_START_BYTE) &&
		(m_serialdata[3] != AMBE3000_TYPE_CHANNEL)
		)
	{
		do {
			m_serialdata.dequeue();
		}
		while(m_serialdata.size() && m_serialdata[0] != AMBE3000_START_BYTE);
	}

	if( (m_serialdata.size() >= (6 + packet_size)) &&
		(m_serialdata[0] == AMBE3000_START_BYTE) &&
		(m_serialdata[3] == AMBE3000_TYPE_CHANNEL)
		)
	{
		for(int i = 0; i < 6; ++i){
			m_serialdata.dequeue();
		}
		for(int i = 0; i < packet_size; i++){
			ambe[i] = m_serialdata.dequeue();
		}
		r = true;
	}
	return r;
}

bool SerialAMBE::get_audio(int16_t *audio)
{
	bool r = false;
	if(m_serialdata.isEmpty()){
		return r;
	}
	uint8_t header[] = {AMBE3000_START_BYTE, 0x01, 0x42, AMBE3000_TYPE_SPEECH, 0x00, 0xA0};
/*
	if( (m_serialdata.size() > 3) &&
		(m_serialdata[0] == 0x61) &&
		(m_serialdata[3] != 0x02)
		)
	{
		do {
			m_serialdata.dequeue();
		}
		while(m_serialdata.size() && m_serialdata[0] != 0x61);
	}

	//qDebug() << "get_audio() m_serialdata.size() == " << m_serialdata.size();

	fprintf(stderr, "AMBEHW %d:%d:", m_serialdata.size(), m_serialdata.size());
	for(int i = 0; i < m_serialdata.size(); ++i){
		//if((d.data()[i] == 0x61) && (data.data()[i+1] == 0x01) && (data.data()[i+2] == 0x42) && (data.data()[i+3] == 0x02)){
		//	i+= 6;
		//}
		fprintf(stderr, "%02x ", (unsigned char)m_serialdata[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
*/
	if(m_serialdata.size() >= 326){
		if ( ((uint8_t)m_serialdata[0] == header[0]) &&
			 ((uint8_t)m_serialdata[1] == header[1]) &&
			 ((uint8_t)m_serialdata[2] == header[2]) &&
			 ((uint8_t)m_serialdata[3] == header[3]) &&
			 ((uint8_t)m_serialdata[4] == header[4]) &&
			 ((uint8_t)m_serialdata[5] == header[5])){
			m_serialdata.dequeue();
			m_serialdata.dequeue();
			m_serialdata.dequeue();
			m_serialdata.dequeue();
			m_serialdata.dequeue();
			m_serialdata.dequeue();
			for(int i = 0; i < 160; i++){
				//Byte swap BE to LE
				audio[i] =  ((m_serialdata.dequeue() << 8) & 0xff00) | (m_serialdata.dequeue() & 0xff);
				audio[i] = (qreal)audio[i] * m_decode_gain;
			}
			r = true;
		}
		else{
			while( (m_serialdata.size() > 5) && (
				((uint8_t)m_serialdata[0] != header[0]) ||
				((uint8_t)m_serialdata[1] != header[1]) ||
				((uint8_t)m_serialdata[2] != header[2]) ||
				((uint8_t)m_serialdata[3] != header[3]) ||
				((uint8_t)m_serialdata[4] != header[4]) ||
				((uint8_t)m_serialdata[5] != header[5]))){
					m_serialdata.dequeue();
			}
		}
	}
	return r;
}

void SerialAMBE::clear_queue()
{
	m_serialdata.clear();
}
