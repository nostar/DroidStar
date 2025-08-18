# MIDI PTT Support in DroidStar

DroidStar includes comprehensive MIDI support for Push-to-Talk (PTT) functionality, allowing you to use MIDI controllers, keyboards, foot pedals, and other MIDI devices to trigger transmission.

## Features

- **Cross-Platform Support**: Windows, Linux, and macOS
- **Flexible Input**: Any MIDI note (0-127) on any channel (1-16 or any)
- **Two Modes**: Hold-to-talk (PTT) or Toggle mode
- **Velocity Sensitivity**: Configurable minimum velocity threshold
- **Multiple Backends**: ALSA, JACK (Linux), Windows MM (Windows), CoreMIDI (macOS)
- **Thread-Safe**: Proper real-time MIDI processing with Qt integration

## Installation

### Prerequisites

DroidStar's MIDI support requires the RtMidi library. Install it using your platform's package manager:

#### Windows

**Option 1: Using vcpkg (Recommended)**
```bash
vcpkg install rtmidi
```

**Option 2: Build from source**
1. Download RtMidi from https://github.com/thestk/rtmidi
2. Build with CMake
3. Set `RTMIDI_DIR` environment variable to the install path

#### Linux

**Ubuntu/Debian:**
```bash
sudo apt-get install librtmidi-dev libasound2-dev
```

**Fedora/RHEL:**
```bash
sudo dnf install rtmidi-devel alsa-lib-devel
```

**Arch Linux:**
```bash
sudo pacman -S rtmidi alsa-lib
```

**For JACK support (optional):**
```bash
# Ubuntu/Debian
sudo apt-get install libjack-jackd2-dev

# Fedora
sudo dnf install jack-audio-connection-kit-devel

# Arch
sudo pacman -S jack2
```

#### macOS

```bash
brew install rtmidi
```

### Building DroidStar with MIDI Support

MIDI support is enabled by default. To build:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

To disable MIDI support:
```bash
cmake -DENABLE_MIDI=OFF ..
```

## Configuration

### Settings Tab

In DroidStar's Settings tab, scroll down to the "MIDI PTT" section:

1. **MIDI Device**: Select your MIDI device from the dropdown
2. **MIDI Note**: Set the note number (0-127, where 60 = Middle C)
3. **MIDI Channel**: Set the channel (0 = Any, 1-16 for specific channels)
4. **Min Velocity**: Set minimum velocity threshold (1-127)
5. **MIDI Mode**: Choose between Toggle and Hold modes

### Example Configurations

**Foot Pedal Setup:**
- Device: Your USB foot pedal
- Note: 36 (Kick drum note)
- Channel: 0 (Any)
- Velocity: 64 (Medium sensitivity)
- Mode: Hold (PTT style)

**Keyboard Setup:**
- Device: Your MIDI keyboard
- Note: 48 (C3) or any comfortable key
- Channel: 1 (Specific channel)
- Velocity: 80 (Higher sensitivity)
- Mode: Toggle (Press once to start/stop)

## MIDI Backends

DroidStar automatically detects and uses the best available MIDI backend for your platform:

### Windows
- **Windows Multimedia (winmm)**: Standard Windows MIDI support
- Works with all USB MIDI devices and built-in MIDI ports

### Linux
- **ALSA**: Advanced Linux Sound Architecture (most common)
- **JACK**: Professional audio system (optional, for low-latency)
- Both can be used simultaneously

### macOS
- **CoreMIDI**: Apple's professional MIDI framework
- Supports all USB MIDI devices and virtual MIDI ports

## Troubleshooting

### No MIDI Devices Detected

1. **Check device connection**: Ensure your MIDI device is connected and recognized by the OS
2. **Check drivers**: Some devices need specific drivers (Windows)
3. **Check permissions**: Some Linux systems require user to be in `audio` group:
   ```bash
   sudo usermod -a -G audio $USER
   ```
4. **Restart application**: After connecting new devices

### MIDI Not Working

1. **Verify RtMidi installation**: Check if library is properly installed
2. **Check compilation**: Look for "Found RtMidi" in cmake output
3. **Test with other software**: Verify device works with other MIDI applications
4. **Check logs**: Enable debug output in DroidStar settings

### Linux-Specific Issues

**ALSA permissions:**
```bash
# Add user to audio group
sudo usermod -a -G audio $USER

# Check ALSA MIDI devices
aconnect -l
```

**JACK setup:**
```bash
# Start JACK daemon
jackd -d alsa

# Check JACK MIDI ports
jack_lsp -t
```

### Windows-Specific Issues

**Driver issues:**
- Ensure MIDI device drivers are installed
- Check Device Manager for any issues
- Some devices may need manufacturer-specific drivers

### macOS-Specific Issues

**Security permissions:**
- macOS may ask for permissions to access MIDI devices
- Grant access when prompted

## API Reference

### QML Methods

Available in `droidstar` object:

```qml
// Check if MIDI is supported
bool is_midi_supported()

// Get list of available MIDI devices
QStringList get_midi_devices()

// Set MIDI device
bool set_midi_device(string deviceName)

// Get current MIDI device
string get_current_midi_device()

// Configure MIDI hotkey
bool set_midi_hotkey(int noteNumber, int channel = -1)

// Clear MIDI hotkey
void clear_midi_hotkey()

// Check if MIDI hotkey is configured
bool has_midi_hotkey()

// Set MIDI toggle mode
void set_midi_toggle_mode(bool enabled)

// Get MIDI toggle mode
bool get_midi_toggle_mode()

// Set velocity threshold
void set_midi_velocity_threshold(int threshold)

// Get velocity threshold
int get_midi_velocity_threshold()
```

## Technical Details

### MIDI Message Handling

DroidStar responds to MIDI Note On/Off messages:
- **Note On** (velocity > 0): Trigger PTT or toggle transmission
- **Note Off** (or Note On with velocity 0): Release PTT (Hold mode only)

### Thread Safety

MIDI callbacks run on a separate thread. DroidStar uses Qt's queued connections to safely handle MIDI messages on the main thread.

### Settings Persistence

All MIDI settings are automatically saved and restored:
- Device selection
- Note and channel configuration
- Velocity threshold
- Toggle mode preference

## Contributing

When contributing to MIDI functionality:

1. Test on multiple platforms when possible
2. Follow the existing Qt signal/slot patterns
3. Ensure thread-safe operation
4. Update this documentation for new features

## License

MIDI support is part of DroidStar and follows the same GPL license terms.