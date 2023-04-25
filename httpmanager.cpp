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

#include "httpmanager.h"

HttpManager::HttpManager(QString f, bool u) : QObject(nullptr)
{
	m_qnam = new QNetworkAccessManager(this);
	QObject::connect(m_qnam, SIGNAL(finished(QNetworkReply*)), this, SLOT(http_finished(QNetworkReply*)));
	m_config_path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
	m_url = u;
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_WIN)
	m_config_path += "/dudetronics";
#endif
	m_filename = f;
}

void HttpManager::process()
{
	QMetaObject::invokeMethod(this,"doRequest");
}

void HttpManager::doRequest()
{
	if(m_url){
		m_qnam->get(QNetworkRequest(QUrl(m_filename)));
	}
	else{
		m_qnam->get(QNetworkRequest(QUrl("http://www.dudetronics.com/ar-dns" + m_filename)));
	}
}

void HttpManager::http_finished(QNetworkReply *reply)
{
    //qDebug() << "http_finished() called";
	if (reply->error()) {
		reply->deleteLater();
		reply = nullptr;
		qDebug() << "http_finished() error()";
		return;
	}
	else{
		if(m_url){
			QStringList l = m_filename.split('/');
			m_filename = "/" + l.at(l.size() - 1);
		}
        //qDebug() << "m_config_path + m_filename = " << m_config_path + m_filename;
        QStringList l = m_filename.split('_');
        if((l.at(0) != "/vocoder") && (l.size() > 1)) return;
		QFile *hosts_file = new QFile(m_config_path + m_filename);
		hosts_file->open(QIODevice::WriteOnly);
		QFileInfo fileInfo(hosts_file->fileName());
		QString filename(fileInfo.fileName());
		hosts_file->write(reply->readAll());
		hosts_file->flush();
		hosts_file->close();
		delete hosts_file;
		emit file_downloaded(filename);
	}
	QThread::currentThread()->quit();
}
