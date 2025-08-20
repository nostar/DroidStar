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

// Forward declaration to avoid including MIDI headers
class MidiHotkeyImpl;

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
	bool isMidiDeviceOpen() const;
	QString currentMidiDevice() const;

	// MIDI hotkey configuration
	bool setMidiHotkey(int noteNumber, int channel = -1); // -1 = any channel
	void clearMidiHotkey();
	bool hasMidiHotkey() const;
	
	// Toggle mode support
	void setToggleMode(bool enabled);
	bool isToggleMode() const;
	
	// Velocity sensitivity
	void setVelocityThreshold(int threshold);
	int velocityThreshold() const;

	// Static method to check if MIDI support is compiled in
	static bool isMidiSupported();

signals:
	void midiHotkeyPressed(int velocity);
	void midiHotkeyReleased();
	void toggleStateChanged(bool transmitting);
	void midiDeviceError(const QString &error);

private slots:
	void emitToggleStateChanged(bool transmitting);

private:
	// PIMPL pointer - all MIDI implementation is hidden
	MidiHotkeyImpl *m_impl;
};

#endif // MIDIHOTKEY_H
