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

#ifdef MIDI_ENABLED
#include "RtMidi.h"
#include <vector>

// Private implementation class - contains all MIDI-specific code
class MidiHotkeyImpl
{
public:
	MidiHotkeyImpl(MidiHotkey *parent);
	~MidiHotkeyImpl();
	
	QStringList getAvailableMidiDevices();
	bool openMidiDevice(const QString &deviceName);
	bool openMidiDevice(int deviceIndex);
	void closeMidiDevice();
	bool isMidiDeviceOpen() const { return m_midiDeviceOpen; }
	QString currentMidiDevice() const { return m_currentMidiDevice; }
	
	bool setMidiHotkey(int noteNumber, int channel = -1);
	void clearMidiHotkey();
	bool hasMidiHotkey() const { return m_midiHotkeyEnabled; }
	
	void setToggleMode(bool enabled);
	bool isToggleMode() const { return m_toggleMode; }
	
	void setVelocityThreshold(int threshold);
	int velocityThreshold() const { return m_velocityThreshold; }
	
private:
	MidiHotkey *q_ptr;
	RtMidiIn *m_midiIn;
	
	bool m_midiDeviceOpen;
	QString m_currentMidiDevice;
	int m_currentMidiDeviceIndex;
	
	// Hotkey configuration
	bool m_midiHotkeyEnabled;
	int m_hotkeyNoteNumber;
	int m_hotkeyChannel; // -1 = any channel
	int m_velocityThreshold;
	
	// Toggle mode state
	bool m_toggleMode;
	bool m_transmitting;
	
	// Settings
	QSettings *m_settings;
	
	void saveSettings();
	void loadSettings();
	void handleMidiMessage(const std::vector<unsigned char> &message);
	
	static void midiCallback(double timeStamp, std::vector<unsigned char> *message, void *userData);
};

// Static callback function for RtMidi
void MidiHotkeyImpl::midiCallback(double timeStamp, std::vector<unsigned char> *message, void *userData)
{
	Q_UNUSED(timeStamp);
	
	MidiHotkeyImpl *impl = static_cast<MidiHotkeyImpl*>(userData);
	if (impl && message && !message->empty()) {
		impl->handleMidiMessage(*message);
	}
}

MidiHotkeyImpl::MidiHotkeyImpl(MidiHotkey *parent)
	: q_ptr(parent)
	, m_midiIn(nullptr)
	, m_midiDeviceOpen(false)
	, m_currentMidiDeviceIndex(-1)
	, m_midiHotkeyEnabled(false)
	, m_hotkeyNoteNumber(-1)
	, m_hotkeyChannel(-1)
	, m_velocityThreshold(64)
	, m_toggleMode(false)
	, m_transmitting(false)
	, m_settings(new QSettings(parent))
{
	try {
		m_midiIn = new RtMidiIn();
	} catch (RtMidiError &error) {
		qWarning() << "Failed to create RtMidiIn:" << error.getMessage().c_str();
		m_midiIn = nullptr;
	}
	
	loadSettings();
}

MidiHotkeyImpl::~MidiHotkeyImpl()
{
	closeMidiDevice();
	delete m_midiIn;
}

QStringList MidiHotkeyImpl::getAvailableMidiDevices()
{
	QStringList devices;
	if (!m_midiIn) return devices;
	
	try {
		unsigned int portCount = m_midiIn->getPortCount();
		for (unsigned int i = 0; i < portCount; i++) {
			try {
				devices << QString::fromStdString(m_midiIn->getPortName(i));
			} catch (RtMidiError &error) {
				qWarning() << "Failed to get MIDI port name for index" << i << ":" << error.getMessage().c_str();
			}
		}
	} catch (RtMidiError &error) {
		qWarning() << "Failed to get MIDI port count:" << error.getMessage().c_str();
	}
	
	return devices;
}

bool MidiHotkeyImpl::openMidiDevice(const QString &deviceName)
{
	if (!m_midiIn) return false;
	
	QStringList devices = getAvailableMidiDevices();
	int index = devices.indexOf(deviceName);
	if (index < 0) return false;
	
	return openMidiDevice(index);
}

bool MidiHotkeyImpl::openMidiDevice(int deviceIndex)
{
	if (!m_midiIn) return false;
	
	closeMidiDevice();
	
	try {
		m_midiIn->openPort(deviceIndex);
		m_midiIn->setCallback(&MidiHotkeyImpl::midiCallback, this);
		m_midiIn->ignoreTypes(false, false, false);
		
		m_midiDeviceOpen = true;
		m_currentMidiDeviceIndex = deviceIndex;
		m_currentMidiDevice = QString::fromStdString(m_midiIn->getPortName(deviceIndex));
		
		qDebug() << "MIDI device opened:" << m_currentMidiDevice;
		return true;
	} catch (RtMidiError &error) {
		qWarning() << "Failed to open MIDI device at index" << deviceIndex << ":" << error.getMessage().c_str();
		return false;
	}
}

void MidiHotkeyImpl::closeMidiDevice()
{
	if (m_midiIn && m_midiDeviceOpen) {
		try {
			m_midiIn->closePort();
		} catch (RtMidiError &error) {
			qWarning() << "Error closing MIDI port:" << error.getMessage().c_str();
		}
	}
	
	m_midiDeviceOpen = false;
	m_currentMidiDevice.clear();
	m_currentMidiDeviceIndex = -1;
}

bool MidiHotkeyImpl::setMidiHotkey(int noteNumber, int channel)
{
	if (noteNumber < 0 || noteNumber > 127) return false;
	if (channel < -1 || channel > 15) return false;
	
	m_hotkeyNoteNumber = noteNumber;
	m_hotkeyChannel = channel;
	m_midiHotkeyEnabled = true;
	
	saveSettings();
	return true;
}

void MidiHotkeyImpl::clearMidiHotkey()
{
	m_midiHotkeyEnabled = false;
	m_hotkeyNoteNumber = -1;
	m_hotkeyChannel = -1;
	saveSettings();
}

void MidiHotkeyImpl::setToggleMode(bool enabled)
{
	m_toggleMode = enabled;
	saveSettings();
}

void MidiHotkeyImpl::setVelocityThreshold(int threshold)
{
	m_velocityThreshold = qBound(1, threshold, 127);
	saveSettings();
}

void MidiHotkeyImpl::saveSettings()
{
	m_settings->setValue("midiHotkeyEnabled", m_midiHotkeyEnabled);
	m_settings->setValue("hotkeyNoteNumber", m_hotkeyNoteNumber);
	m_settings->setValue("hotkeyChannel", m_hotkeyChannel);
	m_settings->setValue("velocityThreshold", m_velocityThreshold);
	m_settings->setValue("toggleMode", m_toggleMode);
	m_settings->setValue("currentMidiDevice", m_currentMidiDevice);
}

void MidiHotkeyImpl::loadSettings()
{
	m_midiHotkeyEnabled = m_settings->value("midiHotkeyEnabled", false).toBool();
	m_hotkeyNoteNumber = m_settings->value("hotkeyNoteNumber", -1).toInt();
	m_hotkeyChannel = m_settings->value("hotkeyChannel", -1).toInt();
	m_velocityThreshold = m_settings->value("velocityThreshold", 64).toInt();
	m_toggleMode = m_settings->value("toggleMode", false).toBool();
	m_currentMidiDevice = m_settings->value("currentMidiDevice", "").toString();
}

void MidiHotkeyImpl::handleMidiMessage(const std::vector<unsigned char> &message)
{
	if (!m_midiHotkeyEnabled || message.size() < 3) return;
	
	unsigned char status = message[0];
	unsigned char note = message[1];
	unsigned char velocity = message[2];
	
	// Extract MIDI channel (0-15) and message type
	int channel = status & 0x0F;
	int messageType = status & 0xF0;
	
	// Check if this matches our hotkey
	if (note != m_hotkeyNoteNumber) return;
	if (m_hotkeyChannel != -1 && channel != m_hotkeyChannel) return;
	if (velocity < m_velocityThreshold) return;
	
	if (messageType == 0x90) { // Note On
		if (m_toggleMode) {
			m_transmitting = !m_transmitting;
			emit q_ptr->toggleStateChanged(m_transmitting);
		} else {
			emit q_ptr->midiHotkeyPressed(velocity);
		}
	} else if (messageType == 0x80) { // Note Off
		if (!m_toggleMode) {
			emit q_ptr->midiHotkeyReleased();
		}
	}
}

#else // MIDI_ENABLED

// Stub implementation when MIDI is disabled
class MidiHotkeyImpl
{
public:
	MidiHotkeyImpl(MidiHotkey *parent) { Q_UNUSED(parent); }
	~MidiHotkeyImpl() {}
	
	QStringList getAvailableMidiDevices() { return QStringList(); }
	bool openMidiDevice(const QString &deviceName) { Q_UNUSED(deviceName); return false; }
	bool openMidiDevice(int deviceIndex) { Q_UNUSED(deviceIndex); return false; }
	void closeMidiDevice() {}
	bool isMidiDeviceOpen() const { return false; }
	QString currentMidiDevice() const { return QString(); }
	
	bool setMidiHotkey(int noteNumber, int channel = -1) { Q_UNUSED(noteNumber); Q_UNUSED(channel); return false; }
	void clearMidiHotkey() {}
	bool hasMidiHotkey() const { return false; }
	
	void setToggleMode(bool enabled) { Q_UNUSED(enabled); }
	bool isToggleMode() const { return false; }
	
	void setVelocityThreshold(int threshold) { Q_UNUSED(threshold); }
	int velocityThreshold() const { return 64; }
};

#endif // MIDI_ENABLED

// MidiHotkey public interface - delegates to implementation
MidiHotkey::MidiHotkey(QObject *parent)
	: QObject(parent)
	, m_impl(new MidiHotkeyImpl(this))
{
}

MidiHotkey::~MidiHotkey()
{
	delete m_impl;
}

QStringList MidiHotkey::getAvailableMidiDevices()
{
	return m_impl->getAvailableMidiDevices();
}

bool MidiHotkey::openMidiDevice(const QString &deviceName)
{
	return m_impl->openMidiDevice(deviceName);
}

bool MidiHotkey::openMidiDevice(int deviceIndex)
{
	return m_impl->openMidiDevice(deviceIndex);
}

void MidiHotkey::closeMidiDevice()
{
	m_impl->closeMidiDevice();
}

bool MidiHotkey::isMidiDeviceOpen() const
{
	return m_impl->isMidiDeviceOpen();
}

QString MidiHotkey::currentMidiDevice() const
{
	return m_impl->currentMidiDevice();
}

bool MidiHotkey::setMidiHotkey(int noteNumber, int channel)
{
	return m_impl->setMidiHotkey(noteNumber, channel);
}

void MidiHotkey::clearMidiHotkey()
{
	m_impl->clearMidiHotkey();
}

bool MidiHotkey::hasMidiHotkey() const
{
	return m_impl->hasMidiHotkey();
}

void MidiHotkey::setToggleMode(bool enabled)
{
	m_impl->setToggleMode(enabled);
}

bool MidiHotkey::isToggleMode() const
{
	return m_impl->isToggleMode();
}

void MidiHotkey::setVelocityThreshold(int threshold)
{
	m_impl->setVelocityThreshold(threshold);
}

int MidiHotkey::velocityThreshold() const
{
	return m_impl->velocityThreshold();
}

bool MidiHotkey::isMidiSupported()
{
#ifdef MIDI_ENABLED
	return true;
#else
	return false;
#endif
}

void MidiHotkey::emitToggleStateChanged(bool transmitting)
{
	emit toggleStateChanged(transmitting);
}
