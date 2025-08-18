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

#include "midihotkey.h"
#include <QDebug>

MidiHotkey::MidiHotkey(QObject *parent)
	: QObject(parent)
#ifdef ENABLE_MIDI
	, m_midiIn(nullptr)
#endif
	, m_midiDeviceOpen(false)
	, m_currentMidiDeviceIndex(-1)
	, m_midiHotkeyEnabled(false)
	, m_hotkeyNoteNumber(-1)
	, m_hotkeyChannel(-1)
	, m_velocityThreshold(64)
	, m_toggleMode(false)
	, m_transmitting(false)
	, m_settings(new QSettings(this))
{
	loadSettings();

#ifdef ENABLE_MIDI
	try {
		m_midiIn = new RtMidiIn();
		qDebug() << "MidiHotkey: RtMidi initialized successfully";
	} catch (RtMidiError &error) {
		qWarning() << "MidiHotkey: Failed to initialize RtMidi:" << QString::fromStdString(error.getMessage());
	}
#else
	qDebug() << "MidiHotkey: MIDI support not compiled in";
#endif
}

MidiHotkey::~MidiHotkey()
{
	closeMidiDevice();
#ifdef ENABLE_MIDI
	delete m_midiIn;
#endif
}

bool MidiHotkey::isMidiSupported()
{
#ifdef ENABLE_MIDI
	return true;
#else
	return false;
#endif
}

QStringList MidiHotkey::getAvailableMidiDevices()
{
	QStringList devices;
	
#ifdef ENABLE_MIDI
	if (!m_midiIn) {
		return devices;
	}
	
	try {
		unsigned int portCount = m_midiIn->getPortCount();
		for (unsigned int i = 0; i < portCount; i++) {
			std::string portName = m_midiIn->getPortName(i);
			devices.append(QString::fromStdString(portName));
		}
	} catch (RtMidiError &error) {
		qWarning() << "MidiHotkey: Error getting MIDI devices:" << QString::fromStdString(error.getMessage());
		emit midiDeviceError(QString::fromStdString(error.getMessage()));
	}
#endif
	
	return devices;
}

bool MidiHotkey::openMidiDevice(const QString &deviceName)
{
#ifdef ENABLE_MIDI
	if (!m_midiIn) {
		return false;
	}
	
	try {
		unsigned int portCount = m_midiIn->getPortCount();
		for (unsigned int i = 0; i < portCount; i++) {
			std::string portName = m_midiIn->getPortName(i);
			if (QString::fromStdString(portName) == deviceName) {
				return openMidiDevice(i);
			}
		}
	} catch (RtMidiError &error) {
		qWarning() << "MidiHotkey: Error opening MIDI device by name:" << QString::fromStdString(error.getMessage());
		emit midiDeviceError(QString::fromStdString(error.getMessage()));
	}
#endif
	
	return false;
}

bool MidiHotkey::openMidiDevice(int deviceIndex)
{
#ifdef ENABLE_MIDI
	if (!m_midiIn) {
		return false;
	}
	
	closeMidiDevice(); // Close any existing connection
	
	try {
		if (deviceIndex < 0 || deviceIndex >= static_cast<int>(m_midiIn->getPortCount())) {
			qWarning() << "MidiHotkey: Invalid device index:" << deviceIndex;
			return false;
		}
		
		m_midiIn->openPort(deviceIndex);
		m_midiIn->setCallback(&MidiHotkey::midiCallback, this);
		m_midiIn->ignoreTypes(false, false, false); // Don't ignore sysex, timing, or active sensing
		
		m_currentMidiDeviceIndex = deviceIndex;
		m_currentMidiDevice = QString::fromStdString(m_midiIn->getPortName(deviceIndex));
		m_midiDeviceOpen = true;
		
		qDebug() << "MidiHotkey: Opened MIDI device:" << m_currentMidiDevice;
		saveSettings();
		return true;
		
	} catch (RtMidiError &error) {
		qWarning() << "MidiHotkey: Error opening MIDI device:" << QString::fromStdString(error.getMessage());
		emit midiDeviceError(QString::fromStdString(error.getMessage()));
		return false;
	}
#endif
	
	return false;
}

void MidiHotkey::closeMidiDevice()
{
#ifdef ENABLE_MIDI
	if (m_midiIn && m_midiDeviceOpen) {
		try {
			m_midiIn->cancelCallback();
			m_midiIn->closePort();
		} catch (RtMidiError &error) {
			qWarning() << "MidiHotkey: Error closing MIDI device:" << QString::fromStdString(error.getMessage());
		}
	}
#endif
	
	m_midiDeviceOpen = false;
	m_currentMidiDeviceIndex = -1;
	m_currentMidiDevice.clear();
}

bool MidiHotkey::setMidiHotkey(int noteNumber, int channel)
{
	if (noteNumber < 0 || noteNumber > 127) {
		qWarning() << "MidiHotkey: Invalid note number:" << noteNumber;
		return false;
	}
	
	if (channel < -1 || channel > 15) {
		qWarning() << "MidiHotkey: Invalid channel:" << channel;
		return false;
	}
	
	m_hotkeyNoteNumber = noteNumber;
	m_hotkeyChannel = channel;
	m_midiHotkeyEnabled = true;
	
	qDebug() << "MidiHotkey: Set MIDI hotkey - Note:" << noteNumber << "Channel:" << (channel == -1 ? "Any" : QString::number(channel + 1));
	
	saveSettings();
	return true;
}

void MidiHotkey::clearMidiHotkey()
{
	m_midiHotkeyEnabled = false;
	m_hotkeyNoteNumber = -1;
	m_hotkeyChannel = -1;
	saveSettings();
	
	qDebug() << "MidiHotkey: Cleared MIDI hotkey";
}

#ifdef ENABLE_MIDI
void MidiHotkey::midiCallback(double deltaTime, std::vector<unsigned char> *message, void *userData)
{
	Q_UNUSED(deltaTime)
	
	MidiHotkey *instance = static_cast<MidiHotkey*>(userData);
	if (instance && message && !message->empty()) {
		// Use Qt's queued connection to safely handle the MIDI message on the main thread
		QMetaObject::invokeMethod(instance, [instance, msg = *message]() {
			instance->handleMidiMessage(msg);
		}, Qt::QueuedConnection);
	}
}
#endif

void MidiHotkey::handleMidiMessage(const std::vector<unsigned char> &message)
{
	if (!m_midiHotkeyEnabled || message.empty()) {
		return;
	}
	
	// Check if this is the configured hotkey note
	int noteNumber = getNoteNumber(message);
	int channel = getChannel(message);
	
	// Check if this matches our hotkey configuration
	if (noteNumber != m_hotkeyNoteNumber) {
		return;
	}
	
	if (m_hotkeyChannel != -1 && channel != m_hotkeyChannel) {
		return;
	}
	
	if (isNoteOn(message)) {
		int velocity = getVelocity(message);
		
		// Check velocity threshold
		if (velocity < m_velocityThreshold) {
			return;
		}
		
		qDebug() << "MidiHotkey: Note ON - Note:" << noteNumber << "Channel:" << (channel + 1) << "Velocity:" << velocity;
		
		if (m_toggleMode) {
			// Toggle mode: switch transmit state
			m_transmitting = !m_transmitting;
			emitToggleStateChanged(m_transmitting);
		} else {
			// PTT mode: start transmitting
			emit midiHotkeyPressed(velocity);
			emit toggleStateChanged(true);
		}
		
	} else if (isNoteOff(message)) {
		qDebug() << "MidiHotkey: Note OFF - Note:" << noteNumber << "Channel:" << (channel + 1);
		
		if (!m_toggleMode) {
			// PTT mode: stop transmitting
			emit midiHotkeyReleased();
			emit toggleStateChanged(false);
		}
		// In toggle mode, note off doesn't change the transmit state
	}
}

bool MidiHotkey::isNoteOn(const std::vector<unsigned char> &message) const
{
	if (message.size() < 3) return false;
	
	unsigned char status = message[0] & 0xF0;
	unsigned char velocity = message[2];
	
	// Note on with velocity > 0, or some keyboards send note on with velocity 0 as note off
	return (status == 0x90 && velocity > 0);
}

bool MidiHotkey::isNoteOff(const std::vector<unsigned char> &message) const
{
	if (message.size() < 3) return false;
	
	unsigned char status = message[0] & 0xF0;
	unsigned char velocity = message[2];
	
	// Note off, or note on with velocity 0
	return (status == 0x80) || (status == 0x90 && velocity == 0);
}

int MidiHotkey::getNoteNumber(const std::vector<unsigned char> &message) const
{
	if (message.size() < 2) return -1;
	return message[1];
}

int MidiHotkey::getChannel(const std::vector<unsigned char> &message) const
{
	if (message.empty()) return -1;
	return message[0] & 0x0F; // Channel is in the lower 4 bits
}

int MidiHotkey::getVelocity(const std::vector<unsigned char> &message) const
{
	if (message.size() < 3) return 0;
	return message[2];
}

void MidiHotkey::emitToggleStateChanged(bool transmitting)
{
	emit toggleStateChanged(transmitting);
}

void MidiHotkey::saveSettings()
{
	m_settings->setValue("midi_device", m_currentMidiDevice);
	m_settings->setValue("midi_device_index", m_currentMidiDeviceIndex);
	m_settings->setValue("midi_hotkey_enabled", m_midiHotkeyEnabled);
	m_settings->setValue("midi_hotkey_note", m_hotkeyNoteNumber);
	m_settings->setValue("midi_hotkey_channel", m_hotkeyChannel);
	m_settings->setValue("midi_velocity_threshold", m_velocityThreshold);
	m_settings->setValue("midi_toggle_mode", m_toggleMode);
}

void MidiHotkey::loadSettings()
{
	m_currentMidiDevice = m_settings->value("midi_device", "").toString();
	m_currentMidiDeviceIndex = m_settings->value("midi_device_index", -1).toInt();
	m_midiHotkeyEnabled = m_settings->value("midi_hotkey_enabled", false).toBool();
	m_hotkeyNoteNumber = m_settings->value("midi_hotkey_note", -1).toInt();
	m_hotkeyChannel = m_settings->value("midi_hotkey_channel", -1).toInt();
	m_velocityThreshold = m_settings->value("midi_velocity_threshold", 64).toInt();
	m_toggleMode = m_settings->value("midi_toggle_mode", false).toBool();
}