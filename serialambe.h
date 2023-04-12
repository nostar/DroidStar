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

#ifndef SERIALAMBE_H
#define SERIALAMBE_H

#include <QObject>
#include <QSerialPort>
#ifdef Q_OS_ANDROID
#include "androidserialport.h"
#endif
#include <QQueue>

class SerialAMBE : public QObject
{
	Q_OBJECT
public:
	SerialAMBE(QString);
	~SerialAMBE();
	static QMap<QString, QString>  discover_devices();
	void connect_to_serial(QString);
	QString get_ambe_description(){ return m_description; }
	QString get_ambe_prodid(){ return m_ambeprodid; }
	QString get_ambe_verstring(){ return m_ambeverstring; }
	bool get_audio(int16_t *);
	bool get_ambe(uint8_t *ambe);
	void decode(uint8_t *);
	void encode(int16_t *);
	void clear_queue();//{ m_serialdata.clear(); }
	void set_decode_gain(qreal g){ m_decode_gain = g; }
private slots:
	void process_serial();
	void receive_serial(QByteArray);
    void config_ambe();
private:
#ifndef Q_OS_ANDROID
	QSerialPort *m_serial;
#else
	AndroidSerialPort *m_serial;
#endif
	QString m_description;
	QString m_manufacturer;
	QString m_serialnum;
	QString m_protocol;
	QString m_ambeverstring;
	QString m_ambeprodid;
	uint8_t packet_size;
	qreal m_decode_gain;
	QQueue<char> m_serialdata;
	void decode_2020(uint8_t *);
	void encode_2020(int16_t *);
	void decode_3000(uint8_t *);
	void encode_3000(int16_t *);
	void process_serial_2020();
	void process_serial_3000();
signals:
	void connected(bool);
	void data_ready();
    void ambedev_ready();
};

#endif // SERIALAMBE_H
