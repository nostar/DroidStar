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

#ifndef ANDROIDSERIALPORT_H
#define ANDROIDSERIALPORT_H

#include <QObject>
#ifdef Q_OS_ANDROID
#include <QCoreApplication>
#include <QJniObject>


class AndroidSerialPort : public QObject
{
	Q_OBJECT
public:
	static AndroidSerialPort & GetInstance()
	{
		static AndroidSerialPort instance;
		return instance;
	}
	QByteArray readAll();
	QStringList discover_devices();

	int open(int);
    void close();
	void setPortName(QString);
	void setBaudRate(int);
	void setDataBits(int);
	void setStopBits(int);
	void setParity(int);
	void setFlowControl(int);
	void setRequestToSend(int);
	int write(QByteArray);
	int write(char *, int);
signals:
	void readyRead();
	void data_received(QByteArray);
    void device_ready();
    void device_denied();
    void devices_changed();
private:
	explicit AndroidSerialPort(QObject * parent = nullptr);
	static void java_data_received(JNIEnv *env, jobject t, jbyteArray data);
    static void java_device_open(JNIEnv *env, jobject t);
    static void java_device_denied(JNIEnv *env, jobject t);
    static void java_devices_changed(JNIEnv *env, jobject t);
#if QT_VERSION < QT_VERSION_CHECK(6, 3, 0)
	QAndroidJniObject serialJavaObject;
#else
	QJniObject serialJavaObject;
	typedef QJniObject QAndroidJniObject;
	typedef QJniEnvironment QAndroidJniEnvironment;
#endif
	QByteArray m_received;
};
#endif
#endif // ANDROIDSERIALPORT_H
