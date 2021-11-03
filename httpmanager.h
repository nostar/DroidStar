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

#ifndef HTTPMANAGER_H
#define HTTPMANAGER_H

#include <QObject>
#include <QtNetwork>

class HttpManager : public QObject
{
	Q_OBJECT
public:
	explicit HttpManager(QString, bool u = false);
	//void start_request(QString file);

signals:
	void file_downloaded(QString);

private:
	QString m_filename;
	QString m_config_path;
	QNetworkAccessManager *m_qnam;
	bool m_url;

private slots:
	void process();
	void doRequest();
	void http_finished(QNetworkReply *reply);
};

#endif // HTTPMANAGER_H
