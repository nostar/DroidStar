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

#ifndef MIDIHOTKEY_H
#define MIDIHOTKEY_H

#include <QObject>
#include <QSettings>
#include <QStringList>
#include <QTimer>

#ifdef ENABLE_MIDI
#include "RtMidi.h"
#endif

class MidiHotkey : public QObject
{
	Q_OBJECT

public:
	explicit MidiHotkey(QObject *parent = nullptr);
	~MidiHotkey();

	// MIDI device management
	QStringList getAvailableMidiDevices();
	bool openMidiDevice(const QString &deviceName);
	bool openMidiDevice(int deviceIndex);
	void closeMidiDevice();
	bool isMidiDeviceOpen() const { return m_midiDeviceOpen; }
	QString currentMidiDevice() const { return m_currentMidiDevice; }

	// MIDI hotkey configuration
	bool setMidiHotkey(int noteNumber, int channel = -1); // -1 = any channel
	void clearMidiHotkey();
	bool hasMidiHotkey() const { return m_midiHotkeyEnabled; }
	
	// Toggle mode support
	void setToggleMode(bool enabled) { m_toggleMode = enabled; saveSettings(); }
	bool isToggleMode() const { return m_toggleMode; }
	
	// Velocity sensitivity
	void setVelocityThreshold(int threshold) { m_velocityThreshold = threshold; saveSettings(); }
	int velocityThreshold() const { return m_velocityThreshold; }

	// Static method to check if MIDI support is compiled in
	static bool isMidiSupported();

signals:
	void midiHotkeyPressed(int velocity);
	void midiHotkeyReleased();
	void toggleStateChanged(bool transmitting);
	void midiDeviceError(const QString &error);

private:
#ifdef ENABLE_MIDI
	RtMidiIn *m_midiIn;
	static void midiCallback(double deltaTime, std::vector<unsigned char> *message, void *userData);
#endif
	
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
	
	// Methods
	void handleMidiMessage(const std::vector<unsigned char> &message);
	void saveSettings();
	void loadSettings();
	
	// MIDI message parsing
	bool isNoteOn(const std::vector<unsigned char> &message) const;
	bool isNoteOff(const std::vector<unsigned char> &message) const;
	int getNoteNumber(const std::vector<unsigned char> &message) const;
	int getChannel(const std::vector<unsigned char> &message) const;
	int getVelocity(const std::vector<unsigned char> &message) const;

private slots:
	void emitToggleStateChanged(bool transmitting);
};

#endif // MIDIHOTKEY_H