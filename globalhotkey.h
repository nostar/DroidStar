/*
	Copyright (C) 2024 DroidStar Contributors

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

#ifndef GLOBALHOTKEY_H
#define GLOBALHOTKEY_H

#include <QObject>
#include <QSettings>

#ifdef Q_OS_MACOS
#include <Carbon/Carbon.h>
#include <ApplicationServices/ApplicationServices.h>
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#include <QAbstractNativeEventFilter>
#endif

#ifdef Q_OS_LINUX
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <QAbstractNativeEventFilter>
#include <xcb/xcb.h>
#endif

class GlobalHotkey : public QObject
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
	, public QAbstractNativeEventFilter
#endif
{
	Q_OBJECT

public:
	explicit GlobalHotkey(QObject *parent = nullptr);
	~GlobalHotkey();

	bool registerHotkey(const QString &keySequence);
	void unregisterHotkey();
	bool isRegistered() const { return m_registered; }
	QString currentHotkey() const { return m_currentHotkey; }
	
	void setToggleMode(bool enabled) { 
		qDebug() << "GlobalHotkey::setToggleMode called with enabled:" << enabled;
		qDebug() << "Before: m_toggleMode was:" << m_toggleMode;
		m_toggleMode = enabled; 
		qDebug() << "After: m_toggleMode is now:" << m_toggleMode;
		qDebug() << "Calling saveSettings()...";
		saveSettings(); 
		qDebug() << "saveSettings() completed";
	}
	
	// Debug method to force PTT mode
	void forcePttMode() {
		qDebug() << "*** FORCING PTT MODE ***";
		m_toggleMode = false;
		qDebug() << "m_toggleMode forcibly set to:" << m_toggleMode;
	}
	bool isToggleMode() const { return m_toggleMode; }
	
	static QString getDefaultHotkey();

signals:
	void hotkeyPressed();
	void hotkeyReleased();
	void toggleStateChanged(bool transmitting);

private:
	bool m_registered;
	QString m_currentHotkey;
	QSettings *m_settings;
	bool m_toggleMode;
	bool m_transmitting;
	bool m_eventHandlerInstalled;

#ifdef Q_OS_MACOS
	EventHotKeyRef m_hotkeyRef;
	static const EventTypeSpec s_hotkeyEvents[2];
	static OSStatus hotkeyHandler(EventHandlerCallRef nextHandler, EventRef event, void *userData);
	bool registerMacHotkey(uint32_t keyCode, uint32_t modifiers);
	void unregisterMacHotkey();
#endif

#ifdef Q_OS_WIN
	int m_hotkeyId;
	bool registerWindowsHotkey(uint32_t keyCode, uint32_t modifiers);
	void unregisterWindowsHotkey();
	bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;
#endif

#ifdef Q_OS_LINUX
	Display *m_display;
	int m_keyCode;
	int m_modifiers;
	bool registerLinuxHotkey(uint32_t keyCode, uint32_t modifiers);
	void unregisterLinuxHotkey();
	bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;
#endif

	bool parseKeySequence(const QString &keySequence, uint32_t &keyCode, uint32_t &modifiers);
	void saveSettings();
	void loadSettings();

private slots:
	void handleHotkeyPressed();
	void handleHotkeyReleased();
};

#endif // GLOBALHOTKEY_H