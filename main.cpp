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

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QIcon>
#include <fcntl.h>
#include "droidstar.h"

int main(int argc, char *argv[])
{
	//QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

	QGuiApplication app(argc, argv);
	QQuickStyle::setStyle("Fusion");
	app.setWindowIcon(QIcon(":/images/droidstar.png"));
	qmlRegisterType<DroidStar>("org.dudetronics.droidstar", 1, 0, "DroidStar");
	QQmlApplicationEngine engine;
#ifdef USE_FLITE
	engine.rootContext()->setContextProperty("USE_FLITE", QVariant(true));
#else
	engine.rootContext()->setContextProperty("USE_FLITE", QVariant(false));
#endif
	const QUrl url(QStringLiteral("qrc:/main.qml"));
	QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
					 &app, [url](QObject *obj, const QUrl &objUrl) {
		if (!obj && url == objUrl)
			QCoreApplication::exit(-1);
	}, Qt::QueuedConnection);
	engine.load(url);
	QObject::connect(&engine, &QQmlApplicationEngine::quit, &app, &QGuiApplication::quit);
	return app.exec();
}
