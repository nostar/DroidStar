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
#include <QDebug>
#include "androidserialport.h"

#ifdef Q_OS_ANDROID

AndroidSerialPort::AndroidSerialPort(QObject *)
{
	if(QAndroidJniObject::isClassAvailable("com.hoho.android.usbserial.driver/UsbSerialDriver")) {
		qDebug() << "com.hoho.android.usbserial.driver/UsbSerialDriver available";
		serialJavaObject = QAndroidJniObject("DroidStar/USBSerialWrapper");
        QAndroidJniEnvironment env;
        JNINativeMethod methods[] = { {"data_received", "([B)V", reinterpret_cast<void*>(java_data_received)}, {"device_open", "()V", reinterpret_cast<void*>(java_device_open)}, {"device_denied", "()V", reinterpret_cast<void*>(java_device_denied)}, {"devices_changed", "()V", reinterpret_cast<void*>(java_devices_changed)}};
		jclass objectClass = env->GetObjectClass(serialJavaObject.object<jobject>());
		env->RegisterNatives(objectClass, methods, sizeof(methods) / sizeof(methods[0]));
		env->DeleteLocalRef(objectClass);
	}
	else {
		qDebug() << "com.hoho.android.usbserial.driver/UsbSerialDriver not available";
	}
}

QStringList AndroidSerialPort::discover_devices()
{
	QAndroidJniEnvironment env;
	QStringList l;
	l.clear();
	qDebug() << "AndroidSerialPort::discover_devices()";
    auto activity = QJniObject(QNativeInterface::QAndroidApplication::context());
    QAndroidJniObject d = serialJavaObject.callObjectMethod("discover_devices", "(Landroid/content/Context;)[Ljava/lang/String;", activity.object());
	jobjectArray devices = d.object<jobjectArray>();
	int size = env->GetArrayLength(devices);

	for (int i=0; i < size; ++i) {
		jstring s = (jstring) env->GetObjectArrayElement(devices, i);
		l.append(env->GetStringUTFChars(s, JNI_FALSE));
	}
	return l;
}

int AndroidSerialPort::open(int p)
{
    auto activity = QJniObject(QNativeInterface::QAndroidApplication::context());
    serialJavaObject.callObjectMethod("setup_serial", "(Landroid/content/Context;)Ljava/lang/String;", activity.object());
    qDebug() << "AndroidSerialPort::open() finished";
	return p;
}

void AndroidSerialPort::close()
{
    serialJavaObject.callMethod<void>("close", "(I)V", 1);
}

int AndroidSerialPort::write(char *data, int s)
{
	QAndroidJniEnvironment env;
	jbyteArray buffer = env->NewByteArray(s);
	env->SetByteArrayRegion(buffer, 0, s, (jbyte *)data);
	serialJavaObject.callMethod<void>("write", "([B)V", buffer);
	env->DeleteLocalRef(buffer);
	return 0;
}

int AndroidSerialPort::write(QByteArray data)
{
	QAndroidJniEnvironment env;
	jbyteArray buffer = env->NewByteArray(data.size());
	env->SetByteArrayRegion(buffer, 0, data.size(), (jbyte *)data.data());
	serialJavaObject.callMethod<void>("write", "([B)V", buffer);
	env->DeleteLocalRef(buffer);
	return 0;
}

void AndroidSerialPort::setPortName(QString s)
{
	QAndroidJniObject p = QAndroidJniObject::fromString(s);
	jstring j = p.object<jstring>();
	serialJavaObject.callMethod<void>("set_port_name", "(Ljava/lang/String;)V", j);
}

void AndroidSerialPort::setBaudRate(int br)
{
	serialJavaObject.callMethod<void>("set_baud_rate", "(I)V", br);
}

void AndroidSerialPort::setDataBits(int db)
{
	serialJavaObject.callMethod<void>("set_data_bits", "(I)V", db);
}

void AndroidSerialPort::setStopBits(int sb)
{
	serialJavaObject.callMethod<void>("set_stop_bits", "(I)V", sb);
}

void AndroidSerialPort::setParity(int p)
{
	serialJavaObject.callMethod<void>("set_parity", "(I)V", p);
}

void AndroidSerialPort::setFlowControl(int fc)
{
	serialJavaObject.callMethod<void>("set_flow_control", "(I)V", fc);
}

void AndroidSerialPort::setRequestToSend(int rts)
{
	serialJavaObject.callMethod<void>("set_rts", "(I)V", rts);
}

QByteArray AndroidSerialPort::readAll()
{
	QByteArray r;
	r.replace(0, m_received.size(), m_received);
	m_received.clear();
	return r;
}

void AndroidSerialPort::java_data_received(JNIEnv *env, jobject, jbyteArray data)
{
	QByteArray r;
	jboolean copy;
	jsize s = env->GetArrayLength(data);
	jbyte *p_data = env->GetByteArrayElements(data, &copy);
	r.append((char *)p_data, s);
	emit AndroidSerialPort::GetInstance().data_received(r);
}

void AndroidSerialPort::java_device_open(JNIEnv *, jobject)
{
    emit AndroidSerialPort::GetInstance().device_ready();

}

void AndroidSerialPort::java_device_denied(JNIEnv *, jobject)
{
    emit AndroidSerialPort::GetInstance().device_denied();

}

void AndroidSerialPort::java_devices_changed(JNIEnv *, jobject)
{
    emit AndroidSerialPort::GetInstance().devices_changed();
}
#endif
