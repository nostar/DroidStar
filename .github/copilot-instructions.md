# Copilot Instructions for DroidStar

## Project Overview

DroidStar is a cross-platform amateur radio digital voice client that connects to M17, D-STAR (REF/XRF/DCS), DMR, YSF/FCS, P25, NXDN reflectors and AllStar nodes (IAX2). Built with C++/Qt6 Quick (QML) and targets Linux, Windows, macOS, Android, and iOS.

Licensed under GPL v3.

## Build

Requires **Qt6 >= 6.5** with modules: Core, Gui, Multimedia, Network, Quick, QuickControls2, SerialPort (except iOS).

```bash
mkdir build && cd build
cmake ..
make
```

### Compile-time feature flags (in CMakeLists.txt)

- `VOCODER_PLUGIN` — build without internal AMBE vocoder (plugin-only mode)
- `USE_FLITE` — enable Flite text-to-speech for TX
- `USE_EXTERNAL_CODEC2` — link system codec2 instead of bundled C++ port
- `USE_MD380_VOCODER` — enable md380 vocoder (ARM platforms, requires external lib)

Toggle these by changing `if(TRUE)` / `if(FALSE)` blocks or uncommenting `DEFINES+=` lines in CMakeLists.txt.

## Architecture

### Core layers

1. **QML UI** (`Main.qml`, `MainTab.qml`, `SettingsTab.qml`, `LogTab.qml`, `HostsTab.qml`, `AboutTab.qml`) — Qt Quick tabbed interface. The `DroidStar` C++ type is registered as `org.dudetronics.droidstar` and instantiated directly in QML.

2. **DroidStar** (`droidstar.h/.cpp`) — Application controller (QObject). Manages settings, host lists, device discovery, and user interactions. Creates protocol `Mode` instances and moves them to a worker `QThread`.

3. **Mode** (`mode.h/.cpp`) — Abstract base class for all digital voice protocols. Owns the `QUdpSocket`, audio engine, vocoder state, and MMDVM modem connection. Each protocol subclass implements `process_udp()`, `transmit()`, `send_ping()`, `send_disconnect()`, and `hostname_lookup()`.

4. **Protocol subclasses** — One per protocol, all inheriting `Mode`:
   - `REF`, `XRF`, `DCS` — D-STAR reflector protocols
   - `DMR` — DMR with Brandmeister/TGIF/DMR+ auth
   - `YSF` — Yaesu System Fusion (also used for FCS)
   - `P25`, `NXDN` — P25 and NXDN reflectors
   - `M17` — M17 protocol with Codec2 vocoder
   - `IAX` — AllStar IAX2 client

5. **AudioEngine** (`audioengine.h/.cpp`) — Wraps Qt audio I/O with AGC, capture/playback queues, and 8kHz PCM processing.

### Factory pattern

`Mode::create_mode(QString)` in `mode.cpp` is the factory that instantiates protocol subclasses by name string (e.g., `"DMR"`, `"M17"`). The `DroidStar` controller calls this, then moves the `Mode` to its own thread and wires up Qt signal/slot connections.

### Vocoder system

Three vocoder paths exist:
- **Hardware**: USB AMBE devices via `SerialAMBE` (not available on iOS)
- **Software plugin**: Dynamic loading (`dlopen`/`LoadLibrary`) via the `Vocoder` interface in `vocoder_plugin.h`
- **Bundled**: `codec2/` (C++ port for M17), `imbe_vocoder/` (P25), `mbe/` (DMR/D-STAR/YSF/NXDN)

### Threading model

Each active `Mode` runs on its own `QThread`. Communication between the QML UI ↔ `DroidStar` controller ↔ `Mode` worker is entirely via Qt signals and slots.

## Key Conventions

- **Flat source layout**: All protocol implementations and helpers are in the project root (no `src/` directory).
- **Platform conditionals**: Heavy use of `#ifdef Q_OS_ANDROID`, `Q_OS_IOS`, `Q_OS_WIN` and CMake `if(IOS)` / `if(ANDROID)` / `if(WIN32)` blocks. iOS excludes serial port support entirely.
- **No test suite**: There are no automated tests in this project.
- **Settings persistence**: `QSettings` stores all user config; `save_settings()` is called from nearly every setter in `DroidStar`.
- **Host files**: Reflector/server lists (DMRHosts.txt, DMRIDs.data, etc.) are downloaded at runtime and stored in the platform's config directory (`~/.config/dudetronics` on Linux).
- **Signal/slot wiring**: `DroidStar::process_connect()` contains extensive signal/slot connections between the controller and the active `Mode`. When adding new protocol features, connections must be added here.
- **QML registration**: The `DroidStar` type is registered via `qmlRegisterType` in `main.cpp` and accessed from QML as a singleton-like instance.
