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

#include "globalhotkey.h"
#include <QDebug>
#include <QCoreApplication>
#include <QTimer>

#ifdef Q_OS_WIN
#include <QGuiApplication>
#include <qpa/qplatformnativeinterface.h>
#endif

#ifdef Q_OS_LINUX
#include <QGuiApplication>
#include <qpa/qplatformnativeinterface.h>
#include <X11/XKBlib.h>
#endif

#ifdef Q_OS_MACOS
const EventTypeSpec GlobalHotkey::s_hotkeyEvents[2] = {
	{ kEventClassKeyboard, kEventHotKeyPressed },
	{ kEventClassKeyboard, kEventHotKeyReleased }
};
#endif

GlobalHotkey::GlobalHotkey(QObject *parent)
	: QObject(parent)
	, m_registered(false)
	, m_toggleMode(false)
	, m_transmitting(false)
	, m_eventHandlerInstalled(false)
#ifdef Q_OS_MACOS
	, m_hotkeyRef(nullptr)
#endif
#ifdef Q_OS_WIN
	, m_hotkeyId(1)
#endif
#ifdef Q_OS_LINUX
	, m_display(nullptr)
	, m_keyCode(0)
	, m_modifiers(0)
#endif
{
	m_settings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "dudetronics", "droidstar", this);
	loadSettings();

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
	// Install native event filter for Windows and Linux
	QCoreApplication::instance()->installNativeEventFilter(this);
#endif

#ifdef Q_OS_LINUX
	// Initialize X11 display
	m_display = XOpenDisplay(nullptr);
	if (!m_display) {
		qWarning() << "GlobalHotkey: Failed to open X11 display";
	}
#endif
	
	// Note: Carbon event handler now calls handleHotkeyPressed/Released directly
	// No need for signal connections
	
	// Test timer to verify signal connections
	QTimer *testTimer = new QTimer(this);
	connect(testTimer, &QTimer::timeout, [this]() {
		static bool pressed = false;
		if (pressed) {
			qDebug() << "Test: Emitting hotkeyReleased";
			emit hotkeyReleased();
		} else {
			qDebug() << "Test: Emitting hotkeyPressed";
			emit hotkeyPressed();
		}
		pressed = !pressed;
	});
	// Uncomment next line to test signal connections every 5 seconds
	// testTimer->start(5000);
}

GlobalHotkey::~GlobalHotkey()
{
	unregisterHotkey();

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
	// Remove native event filter
	if (QCoreApplication::instance()) {
		QCoreApplication::instance()->removeNativeEventFilter(this);
	}
#endif

#ifdef Q_OS_LINUX
	// Close X11 display
	if (m_display) {
		XCloseDisplay(m_display);
	}
#endif
}

QString GlobalHotkey::getDefaultHotkey()
{
#ifdef Q_OS_MACOS
	return "Cmd+Shift+T";
#elif defined(Q_OS_WIN)
	return "Ctrl+Shift+T";
#else
	return "Ctrl+Shift+T";
#endif
}

bool GlobalHotkey::registerHotkey(const QString &keySequence)
{
	if (m_registered) {
		unregisterHotkey();
	}

	if (keySequence.isEmpty()) {
		return false;
	}

	uint32_t keyCode, modifiers;
	if (!parseKeySequence(keySequence, keyCode, modifiers)) {
		qWarning() << "Failed to parse key sequence:" << keySequence;
		return false;
	}

#ifdef Q_OS_MACOS
	qDebug() << "Calling registerMacHotkey...";
	bool result = registerMacHotkey(keyCode, modifiers);
	qDebug() << "registerMacHotkey returned:" << result;
	
	if (result) {
		m_registered = true;
		m_currentHotkey = keySequence;
		saveSettings();
		qDebug() << "Registered global hotkey:" << keySequence;
		return true;
	}
#elif defined(Q_OS_WIN)
	qDebug() << "Calling registerWindowsHotkey...";
	bool result = registerWindowsHotkey(keyCode, modifiers);
	qDebug() << "registerWindowsHotkey returned:" << result;
	
	if (result) {
		m_registered = true;
		m_currentHotkey = keySequence;
		saveSettings();
		qDebug() << "Registered global hotkey:" << keySequence;
		return true;
	}
#elif defined(Q_OS_LINUX)
	qDebug() << "Calling registerLinuxHotkey...";
	bool result = registerLinuxHotkey(keyCode, modifiers);
	qDebug() << "registerLinuxHotkey returned:" << result;
	
	if (result) {
		m_registered = true;
		m_currentHotkey = keySequence;
		saveSettings();
		qDebug() << "Registered global hotkey:" << keySequence;
		return true;
	}
#else
	qWarning() << "Global hotkeys not supported on this platform";
#endif

	qWarning() << "Failed to register global hotkey:" << keySequence;
	return false;
}

void GlobalHotkey::unregisterHotkey()
{
	if (!m_registered) {
		return;
	}

#ifdef Q_OS_MACOS
	unregisterMacHotkey();
#elif defined(Q_OS_WIN)
	unregisterWindowsHotkey();
#elif defined(Q_OS_LINUX)
	unregisterLinuxHotkey();
#endif

	m_registered = false;
	qDebug() << "Unregistered global hotkey:" << m_currentHotkey;
}

#ifdef Q_OS_MACOS
bool GlobalHotkey::registerMacHotkey(uint32_t keyCode, uint32_t modifiers)
{
	// Check if we have accessibility permissions
	qDebug() << "Checking accessibility permissions...";
	
	// First try to request permissions with prompt
	CFStringRef keys[] = { kAXTrustedCheckOptionPrompt };
	CFBooleanRef values[] = { kCFBooleanTrue };
	CFDictionaryRef options = CFDictionaryCreate(nullptr, (const void**)keys, (const void**)values, 1, nullptr, nullptr);
	
	bool hasPermissions = AXIsProcessTrustedWithOptions(options);
	CFRelease(options);
	
	if (!hasPermissions) {
		qWarning() << "DroidStar does not have accessibility permissions.";
		qWarning() << "A system dialog should have appeared requesting permissions.";
		qWarning() << "If no dialog appeared, please manually:";
		qWarning() << "1. Open System Settings > Privacy & Security > Accessibility";
		qWarning() << "2. Add DroidStar to the allowed applications";
		qWarning() << "3. Restart DroidStar";
		qWarning() << "Current app path:" << QCoreApplication::applicationFilePath();
		return false;
	}
	qDebug() << "Accessibility permissions granted";
	
	// Install event handler only once per instance
	if (!m_eventHandlerInstalled) {
		OSStatus handlerStatus = InstallApplicationEventHandler(&GlobalHotkey::hotkeyHandler, 
			GetEventTypeCount(s_hotkeyEvents), s_hotkeyEvents, this, nullptr);
		if (handlerStatus != noErr) {
			qWarning() << "Failed to install event handler, error:" << handlerStatus;
			return false;
		}
		m_eventHandlerInstalled = true;
		qDebug() << "Event handler installed successfully for instance" << this;
	}

	EventHotKeyID hotkeyId;
	hotkeyId.signature = 'DRDS';
	hotkeyId.id = 1;

	qDebug() << "Attempting to register hotkey with keyCode:" << keyCode << "modifiers:" << modifiers;
	
	OSStatus status = RegisterEventHotKey(keyCode, modifiers, hotkeyId, 
		GetApplicationEventTarget(), 0, &m_hotkeyRef);
	
	if (status != noErr) {
		qWarning() << "Failed to register hotkey, error:" << status;
		return false;
	}

	qDebug() << "Hotkey registered successfully";
	return true;
}

void GlobalHotkey::unregisterMacHotkey()
{
	if (m_hotkeyRef) {
		UnregisterEventHotKey(m_hotkeyRef);
		m_hotkeyRef = nullptr;
	}
}

OSStatus GlobalHotkey::hotkeyHandler(EventHandlerCallRef nextHandler, EventRef event, void *userData)
{
	GlobalHotkey *hotkey = static_cast<GlobalHotkey*>(userData);
	if (!hotkey) {
		qDebug() << "Hotkey handler called with null userData";
		return eventNotHandledErr;
	}

	UInt32 eventKind = GetEventKind(event);
	qDebug() << "Hotkey event received, kind:" << eventKind;
	
	switch (eventKind) {
	case kEventHotKeyPressed:
		qDebug() << "Hotkey pressed, calling handleHotkeyPressed directly";
		QMetaObject::invokeMethod(hotkey, "handleHotkeyPressed", Qt::QueuedConnection);
		break;
	case kEventHotKeyReleased:
		qDebug() << "Hotkey released, calling handleHotkeyReleased directly";
		QMetaObject::invokeMethod(hotkey, "handleHotkeyReleased", Qt::QueuedConnection);
		break;
	default:
		qDebug() << "Unknown hotkey event kind:" << eventKind;
		return eventNotHandledErr;
	}

	return noErr;
}
#endif

bool GlobalHotkey::parseKeySequence(const QString &keySequence, uint32_t &keyCode, uint32_t &modifiers)
{
	qDebug() << "Parsing key sequence:" << keySequence;
	QStringList parts = keySequence.split('+', Qt::SkipEmptyParts);
	if (parts.isEmpty()) {
		qDebug() << "Empty key sequence";
		return false;
	}

	modifiers = 0;
	keyCode = 0;

	// Parse modifiers and key
	for (int i = 0; i < parts.size(); ++i) {
		QString part = parts[i].trimmed().toLower();
		
		if (i == parts.size() - 1) {
			// Last part should be the key
#ifdef Q_OS_MACOS
			if (part == "t") keyCode = kVK_ANSI_T;
			else if (part == "tab") keyCode = kVK_Tab;
			else if (part == "space") keyCode = kVK_Space;
			else if (part == "enter" || part == "return") keyCode = kVK_Return;
			else if (part == "esc" || part == "escape") keyCode = kVK_Escape;
			else if (part == "f1") keyCode = kVK_F1;
			else if (part == "f2") keyCode = kVK_F2;
			else if (part == "f3") keyCode = kVK_F3;
			else if (part == "f4") keyCode = kVK_F4;
			else if (part == "f5") keyCode = kVK_F5;
			else if (part == "f6") keyCode = kVK_F6;
			else if (part == "f7") keyCode = kVK_F7;
			else if (part == "f8") keyCode = kVK_F8;
			else if (part == "f9") keyCode = kVK_F9;
			else if (part == "f10") keyCode = kVK_F10;
			else if (part == "f11") keyCode = kVK_F11;
			else if (part == "f12") keyCode = kVK_F12;
			else if (part.length() == 1) {
				char c = part.at(0).toLatin1();
				if (c >= 'a' && c <= 'z') {
					keyCode = kVK_ANSI_A + (c - 'a');
				} else if (c >= '0' && c <= '9') {
					keyCode = kVK_ANSI_0 + (c - '0');
				}
			}
#endif
		} else {
			// Parse modifiers
#ifdef Q_OS_MACOS
			if (part == "cmd" || part == "command") {
				modifiers |= cmdKey;
			} else if (part == "ctrl" || part == "control") {
				modifiers |= controlKey;
			} else if (part == "alt" || part == "option") {
				modifiers |= optionKey;
			} else if (part == "shift") {
				modifiers |= shiftKey;
			}
#elif defined(Q_OS_WIN)
			// Windows modifier mapping would go here
#else
			// Linux modifier mapping would go here
#endif
		}
	}

	qDebug() << "Parsed keyCode:" << keyCode << "modifiers:" << modifiers;
	return keyCode != 0;
}

void GlobalHotkey::saveSettings()
{
	m_settings->setValue("ptt_hotkey", m_currentHotkey);
	m_settings->setValue("ptt_toggle_mode", m_toggleMode);
	m_settings->sync();
}

void GlobalHotkey::loadSettings()
{
	m_currentHotkey = m_settings->value("ptt_hotkey", getDefaultHotkey()).toString();
	m_toggleMode = m_settings->value("ptt_toggle_mode", false).toBool();
	if (!m_currentHotkey.isEmpty()) {
		registerHotkey(m_currentHotkey);
	}
}

void GlobalHotkey::handleHotkeyPressed()
{
	qDebug() << "*** INSIDE GlobalHotkey::handleHotkeyPressed() *** - Toggle mode:" << m_toggleMode << "Currently transmitting:" << m_transmitting;
	
	if (m_toggleMode) {
		// Toggle mode: press once to start TX, press again to stop TX
		if (!m_transmitting) {
			m_transmitting = true;
			qDebug() << "Toggle mode: Starting transmission";
			emit toggleStateChanged(true);
		} else {
			m_transmitting = false;
			qDebug() << "Toggle mode: Stopping transmission";
			emit toggleStateChanged(false);
		}
	} else {
		// Hold mode: start TX on press
		qDebug() << "Hold mode: Starting transmission";
		emit toggleStateChanged(true);
	}
}

void GlobalHotkey::handleHotkeyReleased()
{
	qDebug() << "GlobalHotkey::handleHotkeyReleased() - Toggle mode:" << m_toggleMode;
	
	if (!m_toggleMode) {
		// Hold mode: stop TX on release
		qDebug() << "Hold mode: Stopping transmission";
		emit toggleStateChanged(false);
	}
	// In toggle mode, release does nothing
}

#ifdef Q_OS_WIN
bool GlobalHotkey::registerWindowsHotkey(uint32_t keyCode, uint32_t modifiers)
{
	// Get the main window handle
	HWND hwnd = nullptr;
	
	// Get any available window handle from QGuiApplication
	auto app = qobject_cast<QGuiApplication*>(QCoreApplication::instance());
	if (app) {
		auto windows = app->allWindows();
		if (!windows.isEmpty()) {
			hwnd = (HWND)windows.first()->winId();
		}
	}
	
	if (!hwnd) {
		qWarning() << "GlobalHotkey: No window handle available for Windows hotkey registration";
		return false;
	}
	
	// Convert Qt modifiers to Windows modifiers
	UINT winModifiers = 0;
	if (modifiers & 0x100) winModifiers |= MOD_SHIFT;   // Qt::ShiftModifier
	if (modifiers & 0x200) winModifiers |= MOD_CONTROL; // Qt::ControlModifier
	if (modifiers & 0x400) winModifiers |= MOD_ALT;     // Qt::AltModifier
	if (modifiers & 0x800) winModifiers |= MOD_WIN;     // Qt::MetaModifier
	
	bool result = RegisterHotKey(hwnd, m_hotkeyId, winModifiers, keyCode);
	if (result) {
		qDebug() << "Windows hotkey registered with ID:" << m_hotkeyId << "keyCode:" << keyCode << "modifiers:" << winModifiers;
	} else {
		DWORD error = GetLastError();
		qWarning() << "Failed to register Windows hotkey. Error:" << error;
	}
	
	return result;
}

void GlobalHotkey::unregisterWindowsHotkey()
{
	HWND hwnd = nullptr;
	
	// Get any available window handle from QGuiApplication
	auto app = qobject_cast<QGuiApplication*>(QCoreApplication::instance());
	if (app) {
		auto windows = app->allWindows();
		if (!windows.isEmpty()) {
			hwnd = (HWND)windows.first()->winId();
		}
	}
	
	if (hwnd) {
		UnregisterHotKey(hwnd, m_hotkeyId);
		qDebug() << "Windows hotkey unregistered with ID:" << m_hotkeyId;
	}
}

bool GlobalHotkey::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
	if (eventType == "windows_generic_MSG") {
		MSG *msg = static_cast<MSG*>(message);
		if (msg->message == WM_HOTKEY) {
			if (msg->wParam == m_hotkeyId) {
				qDebug() << "Windows hotkey pressed!";
				handleHotkeyPressed();
				// Note: Windows doesn't provide hotkey release events
				// We'll simulate a release after a short delay for hold mode
				if (!m_toggleMode) {
					QTimer::singleShot(100, this, [this]() {
						handleHotkeyReleased();
					});
				}
				return true;
			}
		}
	}
	return false;
}
#endif // Q_OS_WIN

#ifdef Q_OS_LINUX
bool GlobalHotkey::registerLinuxHotkey(uint32_t keyCode, uint32_t modifiers)
{
	if (!m_display) {
		qWarning() << "GlobalHotkey: X11 display not available";
		return false;
	}
	
	// Convert Qt key code to X11 KeySym and then to KeyCode
	KeySym keysym = keyCode; // For now, assume direct mapping
	m_keyCode = XKeysymToKeycode(m_display, keysym);
	
	if (m_keyCode == 0) {
		qWarning() << "GlobalHotkey: Failed to convert keycode" << keyCode << "to X11 keycode";
		return false;
	}
	
	// Convert Qt modifiers to X11 modifiers
	m_modifiers = 0;
	if (modifiers & 0x100) m_modifiers |= ShiftMask;   // Qt::ShiftModifier
	if (modifiers & 0x200) m_modifiers |= ControlMask; // Qt::ControlModifier
	if (modifiers & 0x400) m_modifiers |= Mod1Mask;    // Qt::AltModifier
	if (modifiers & 0x800) m_modifiers |= Mod4Mask;    // Qt::MetaModifier (Super/Windows key)
	
	Window root = DefaultRootWindow(m_display);
	
	// Grab the key with and without Num Lock and Caps Lock
	// This ensures the hotkey works regardless of these lock states
	int lockMasks[] = { 0, LockMask, Mod2Mask, LockMask | Mod2Mask };
	
	for (int i = 0; i < 4; i++) {
		int grabResult = XGrabKey(m_display, m_keyCode, m_modifiers | lockMasks[i], 
								  root, False, GrabModeAsync, GrabModeAsync);
		if (grabResult == BadAccess) {
			qWarning() << "GlobalHotkey: Key combination already grabbed by another application";
			// Try to ungrab what we already grabbed
			for (int j = 0; j < i; j++) {
				XUngrabKey(m_display, m_keyCode, m_modifiers | lockMasks[j], root);
			}
			return false;
		}
	}
	
	XSync(m_display, False);
	qDebug() << "Linux hotkey registered - keyCode:" << m_keyCode << "modifiers:" << m_modifiers;
	return true;
}

void GlobalHotkey::unregisterLinuxHotkey()
{
	if (!m_display || m_keyCode == 0) {
		return;
	}
	
	Window root = DefaultRootWindow(m_display);
	
	// Ungrab all the key combinations we grabbed
	int lockMasks[] = { 0, LockMask, Mod2Mask, LockMask | Mod2Mask };
	for (int i = 0; i < 4; i++) {
		XUngrabKey(m_display, m_keyCode, m_modifiers | lockMasks[i], root);
	}
	
	XSync(m_display, False);
	qDebug() << "Linux hotkey unregistered - keyCode:" << m_keyCode;
	
	m_keyCode = 0;
	m_modifiers = 0;
}

bool GlobalHotkey::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
	if (eventType == "xcb_generic_event_t") {
		xcb_generic_event_t *event = static_cast<xcb_generic_event_t*>(message);
		
		if ((event->response_type & ~0x80) == XCB_KEY_PRESS) {
			xcb_key_press_event_t *keyEvent = (xcb_key_press_event_t*)event;
			
			// Check if this is our registered hotkey
			if (keyEvent->detail == m_keyCode) {
				// Mask out lock keys for comparison
				uint16_t cleanMods = keyEvent->state & ~(LockMask | Mod2Mask);
				if (cleanMods == m_modifiers) {
					qDebug() << "Linux hotkey pressed!";
					handleHotkeyPressed();
					return true;
				}
			}
		} else if ((event->response_type & ~0x80) == XCB_KEY_RELEASE) {
			xcb_key_release_event_t *keyEvent = (xcb_key_release_event_t*)event;
			
			// Check if this is our registered hotkey
			if (keyEvent->detail == m_keyCode) {
				uint16_t cleanMods = keyEvent->state & ~(LockMask | Mod2Mask);
				if (cleanMods == m_modifiers) {
					qDebug() << "Linux hotkey released!";
					handleHotkeyReleased();
					return true;
				}
			}
		}
	}
	return false;
}
#endif // Q_OS_LINUX