# DroidStar
This software connects to M17, Fusion (YSF/FCS, DN and VW modes are supported), DMR, P25, NXDN, D-STAR (REF/XRF/DCS) reflectors and AllStar nodes (as an IAX2 client or Web Transceiver mode) over UDP.  It is compatible with all of the AMBE USB devices out there (ThumbDV, DVstick 30, DVSI, etc). It also supports MMDVM modems and can be used as a hotspot, or as a stand-alone transceiver via direct mode to the MMDVM device.  This software is open source and uses the cross platform C++ library called Qt.  It will build and run on Linux, Windows, MacOS, Android, and iOS. No USB device support for iOS though (AMBE vocoder or MMDVM). It should also build and run on any other posix platform that has Qt avilable (xxxBSD, Solaris, etc).  This software is provided *as-is* and no support is available.

This software makes use of software from a number of other open source software projects, including MMDVMHost, MMDVM_CM, mvoice, and others. Not only is software from these projects being used directly, but learning about the various network protocols and encoding/decoding of the various protocols was only possible thanks to the authors of all of these software projects.

# DudeStar, DroidStar, and Qt
The DudeStar application used the Qt Widgets UI, while DroidStar uses the Qt Quick UI.  All of the back end C/C++ source code for both projects has always been identical, but because of the different UI APIs, two repositories had to be maintained for the same project. Even though I prefer the Qt widgets UI over the Qt Quick UI for desktop applications, I have combined both projects into a single entity which is now simply called 'DroidStar'. My dudestar repo has been removed from github, but there are plenty of forks of it on github, in case anyone wishes to continue development of that version.

# M17 support 
The Codec2 vocoder library is open source and is included as a C++ implementation of the original C library taken from the mvoice project.  More info on M17 can be found here: https://m17project.org/

M17 SMS type (0x06) packet support has been added to DroidStar.  A text input and SMS send button is available in the log tab when in M17 mode.

# MMDVM support -- work in progress
DroidStar supports MMDVM and MMDVM_HS (hotspot) modems, with basic (possibly buggy) support for M17, D-STAR, Fusion, and DMR.  Support for P25 and NXDN coming soon.  When connecting to a digital mode reflector/DMR server and selecting an MMDVM device under Modems, then DroidStar acts as a hotspot/repeater.  When 'MMDVM Direct' (currently M17 only) is selected as the host, then DroidStar becomes a stand-alone transceiver.

# PTT Hotkey Support
DroidStar includes comprehensive Push-to-Talk (PTT) support via both keyboard hotkeys and MIDI devices:

## Keyboard PTT Hotkeys
- **Cross-platform support**: macOS (Carbon), Windows (RegisterHotKey), Linux (X11)
- **Global hotkeys**: Work even when DroidStar is not in focus
- **Two operation modes**: Hold-to-talk (PTT) or Toggle mode
- **Configurable key combinations**: Support for Ctrl, Shift, Alt/Option, Cmd/Win modifiers
- **Background operation**: No need to switch to DroidStar window to transmit

Default hotkeys:
- **macOS**: `Cmd+Shift+T`
- **Windows**: `Ctrl+Shift+T`
- **Linux**: `Ctrl+Shift+T`

## MIDI PTT Support
Use MIDI controllers, keyboards, foot pedals, or any MIDI device to trigger transmission. Features include:

- **Cross-platform support**: Windows (Windows MM), Linux (ALSA/JACK), macOS (CoreMIDI)
- **Flexible configuration**: Any MIDI note (0-127) on any channel (1-16)
- **Two operation modes**: Hold-to-talk (PTT) or Toggle mode
- **Velocity sensitivity**: Configurable minimum velocity threshold
- **Background operation**: Works even when DroidStar is not in focus

To enable MIDI support, install the RtMidi library:
- **Linux**: `sudo apt-get install librtmidi-dev` (Ubuntu/Debian) or equivalent
- **Windows**: Install via vcpkg or build from source
- **macOS**: `brew install rtmidi`

Configure PTT settings in the Settings tab under "PTT Hotkeys" and "MIDI PTT". See [MIDI_SUPPORT.md](MIDI_SUPPORT.md) for detailed setup instructions.

# Software vocoder plugin API
There is a vocoder plugin API available for loading of vocoder software.  Any vocoder plugin used with DroidStar should be properly licensed by the user if any copyright patents apply.  Do not use any patented vocoder plugin that you are not licensed to use.  I have no information regarding aquiring a software vocoder.

# Loading a vocoder plugin
A vocoder plugin can be downloaded from a URL, which will place the vocoder into the correct location for the platform.  Add a full http url, including the filename, to the Vocoder URL option on the settings tab, and click the Download vocoder button.

For Android and iOS devices, the vocoder URL download method is the only way to load a vocoder plugin.  For Linux/MacOS/Windows, a vocoder can also be manually placed.  Copy the plugin to the system settings file location.  On Linux, this is ~/.config/dudetronics.  It is the same place where the host and id files are located (DMRHosts.txt, DMRIDs.data, etc).

The vocoder plugin filename must be named vocoder_plugin.platform.arch where platform and arch can be any of the following:
platform: linux, darwin, winnt, android, ios
arch: x86_64, arm, arm64

The platform and arch for your device are identified on the About tab.

There are no software vocoder plugins available in this repository, and I have no information on obtaining one.  DONT ASK!

# Optional FLite Text-to-speech build
I added Flite TTS TX capability so I didn't have to talk to myself all of the time during development and testing.  To build DroidStar with Flite TTS support, uncomment the line 'DEFINES += USE_FLITE' from the top of DroidStar.pro (and run/re-run qmake). You will need the Flite library and development header files installed on your system.  When built with Flite support, 3 TTS options and a Mic in option will be available at the bottom of the window.  TTS1-TTS3 are 3 voice choices, and Mic in turns off TTS and uses the microphone for input.  The text to be converted to speech and transmitted goes in the text box under the TTS options.

# Usage
Linux users with USB AMBE and/or MMDVM dongles will need to make sure they have permission to use the USB serial device, and disable the archaic ModeManager service that still exists on many Linux systems. On most systems this means adding your user to the 'dialout' group, and running 'sudo systemctl disable ModemManager.service' and rebooting.  This is a requirement for any serial device to be accessed.

Host/Mod: Select the desired host and module (for D-STAR and M17) from the selections.

Callsign:  Enter your amateur radio callsign.  A valid license is required to use this software.

DMRID: A valid DMR ID is required to connect to DMR servers.
Latitude/Longitude/Location/Description:  These are DMR config options, sent to the DMR server during connect.   Some servers require specific values here, some do not.  This is specific to the server you are connecting to, so please dont ask what these values should be.

DMR+ IPSC2 hosts:  The format for the DMR+ options string is the complete string including 'Options='.  Create your options string and check 'Send DMR+ options on connect' before connecting.  A description of the DMR+ options string can be found here: https://github.com/g4klx/MMDVMHost/blob/master/DMRplus_startup_options.md .

Talkgroup:  For DMR, enter the talkgroup ID number.  A very active TG for testing functionality on Brandmeister is 91 (Brandmeister Worldwide).  You must TX with a talkgroup entered to link to that talkgroup, just like a real radio.  Any statics you have defined in BM selfcare will work the same way they do if you were using a hotspot/radio.

MYCALL/URCALL/RPTR1/RPTR2 are for Dstar modes REF/DCS/XRF.  These fields need to be entered correctly before attempting to TX on any DSTAR reflector.  All fields are populated with suggested values upon connect, but can still be modified for advanced users.  RPT2 is always overwritten with the current reflector upon connected.

# IAX Client for AllStar
DroidStar can connect to an AllStar node as an IAX(2) client.  See the AllStar wiki and other AllStar, Asterisk, and IAX2 protocal related websites for the technical details of IAX2 for AllStar.  This is a basic client and currently only uLaw audio codec is supported.  This is the default codec on most AllStar nodes.

All IAX nodes are now defined on the Hosts tab.  The example shows the format.  Once there are one or more IAX nodes defined on the Hosts tab, they will be available to select when IAX mode is selected from the Main tab.  This change allows multiple IAX nodes to be defined.

Add DTMF commands like \*3node, \*1node, \*70, etc in the IAX DTMF box and hit send to send the DTMF string. Details on various commands can be found at the AllStar wiki and others.

# AllStar web transceiver support
In order to connect to an AllStar node via the ASL WT portal, add a line in the Host tab as follows:
```
IAX 12345 wt 4569 allstar-public allstar
```
When 'wt' is used instead of an IP address, then wt will be replaced by XXXXX.nodes.allstarlink.org, where XXXXX is the specified none number.  Then you must add you ASL web portal password to ASL password under settings.  This is *NOT* the password for your node, this is the password you made to login to the ASL website.

# General building instructions
This software is written primarily in C++ and requires Qt6 >= Qt6.5, and naturally the development packages to build.  Java, QML (Javascript based), and C# code is also used where necessary.  The preferred way to obtain Qt is to use the Qt open source online installer from the Qt website.  Run this installer as a user (not root) to keep the Qt installation separate from your system libs.  Select the option as shown in this pic https://imgur.com/i0WuFCY which will install everything under ~/Qt.

## Building on macOS

### Prerequisites
1. **Xcode Command Line Tools** (required for compiler and system frameworks):
   ```bash
   xcode-select --install
   ```

2. **Homebrew** (package manager):
   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   ```

### Required Dependencies
Install the essential build dependencies:

```bash
# Core build tools and Qt6
brew install cmake qt

# MIDI support (optional but recommended)
brew install rtmidi

# Git (if not already installed)
brew install git
```

### Building DroidStar
1. **Clone the repository**:
   ```bash
   git clone https://github.com/nostar/DroidStar.git
   cd DroidStar
   ```

2. **Create build directory and configure**:
   ```bash
   mkdir build
   cd build
   cmake ..
   ```

3. **Build the application**:
   ```bash
   make -j$(sysctl -n hw.ncpu)
   ```

4. **Run DroidStar**:
   ```bash
   # Run directly from build directory
   ./DroidStar.app/Contents/MacOS/DroidStar
   
   # Or copy to Applications folder (optional)
   cp -r DroidStar.app /Applications/
   open /Applications/DroidStar.app
   ```

### Optional Features
- **MIDI PTT Support**: Requires `rtmidi` (install with `brew install rtmidi`)
- **Additional Audio Codecs**: May require additional development libraries

### Troubleshooting macOS Build Issues
- **Qt not found**: Ensure Qt6 is properly installed via Homebrew. You can check with `brew list qt`
- **CMake errors**: Make sure you have the latest CMake version with `brew upgrade cmake`
- **Missing frameworks**: Xcode Command Line Tools must be installed for Carbon framework support
- **Permission issues**: DroidStar requires accessibility permissions for global hotkeys. You'll be prompted to grant these when first using PTT hotkeys.

### macOS-Specific Features
- **Global Hotkeys**: Uses Carbon framework for system-wide PTT key capture
- **Background Operation**: Works even when DroidStar is not the active application
- **Native Integration**: Follows macOS UI conventions and supports native features

## Creating Portable macOS Distribution

To create a portable version of DroidStar that can be distributed to other macOS users without requiring them to install dependencies:

### Automated Deployment (Recommended)
Use the provided deployment script:

```bash
# Basic portable app bundle
./deploy_macos.sh

# Create portable app with code signing (requires Developer ID)
./deploy_macos.sh --codesign "Developer ID Application: Your Name"

# Create notarized app for distribution (requires Apple Developer account)
./deploy_macos.sh --notarize "Developer ID Application: Your Name"

# Clean build and create portable version
./deploy_macos.sh --clean
```

### Manual Deployment Steps
If you prefer to do the steps manually:

1. **Build the application**:
   ```bash
   mkdir build && cd build
   cmake ..
   make -j$(sysctl -n hw.ncpu)
   ```

2. **Create portable app bundle**:
   ```bash
   cp -r DroidStar.app DroidStar_Portable.app
   macdeployqt DroidStar_Portable.app
   ```

3. **Create DMG for distribution** (optional):
   ```bash
   macdeployqt DroidStar_Portable.app -dmg
   ```

### Distribution Package Details
- **App Bundle**: Self-contained `.app` with all dependencies (~74MB)
- **DMG Image**: Compressed disk image for easy distribution (~37MB)
- **Compatibility**: ARM64 architecture for Apple Silicon Macs (M1, M2, M3+)
- **Dependencies**: All Qt frameworks and MIDI libraries bundled internally

### Code Signing and Notarization (Optional)
For wider distribution, especially outside the Mac App Store:

```bash
# Sign with Developer ID (removes "Unknown Developer" warning)
./deploy_macos.sh --codesign "Developer ID Application: Your Name"

# Sign and prepare for notarization (required for Gatekeeper compatibility)
./deploy_macos.sh --notarize "Developer ID Application: Your Name"
```

**Requirements for code signing:**
- Apple Developer ID certificate
- Keychain with valid certificates
- For notarization: Apple Developer account and app-specific password

### Testing Portability
To verify the app is truly portable:

```bash
# Check dependencies are bundled
otool -L DroidStar_Portable.app/Contents/MacOS/DroidStar

# Test on system without Homebrew dependencies
# All paths should be @executable_path/... or system frameworks
```

### Distribution Best Practices
1. **DMG Distribution**: Provide the `.dmg` file for easy installation
2. **Instructions**: Include setup instructions for first-time users
3. **Accessibility**: Inform users about accessibility permissions for global hotkeys
4. **MIDI Setup**: Provide MIDI device configuration guidance if relevant

In an effort to encourage others to build from source on multiple platforms, there are no longer any external build dependencies.  In order to build DroidStar with no internal AMBE vocoder, uncomment the the following line in the CMakeLists.txt file:
```
DEFINES+=VOCODER_PLUGIN
```
Building DroidStar with this line commented out will build with internal AMBE support.  If you choose to do this, it is your responsibility to determine if you will violate any patents in your area.

## Note for building on RaspiOS (Also applies to debian based Linux desktops)
The following commands should install everything necessary to build and run DroidStar:
```
sudo apt install libqt6*
sudo apt install qml6*
sudo apt install qt6-*-dev
```
Then to build:

```
git clone https://github.com/nostar/DroidStar.git
cd DroidStar
mkdir build
cd build
cmake ..
make
```
If building an an arm based platform like rpi or using dynarmic on x64/arm64, the md380 vocoder can be used.  In order to build with this, uncomment the following line in DroidStar.pro:
```
#DEFINES += USE_MD380_VOCODER
```
This requires the md380_vocoder library to be installed: https://github.com/nostar/md380_vocoder
You must make sure that you are not in violation of any patent laws in your area if you decide to use this.

My primary development platform is Fedora Linux.  With a proper build environment, the build instructions apply to all other platforms/distributions, including Windows and macOS.

All of the gradle build files are provided to create an APK file ready to be installed on an Android device.  A proper Android build system including the Android NDK is required and beyond the scope of this document.

# No builds are available on Github
No builds for any platform are available on this Github site.  This is and always will be an open source project, to be used for educational and development purposes only. I am currently providing a Windows build which is *not* to be confused as any sort of official release of any kind.  No support will be provided for any build at any time.

# DroidStar for iOS
DroidStar used to be available for iOS via the TestFlight program.  I got fed up with Apple and its developer policies (not to mention the cost) so I cancelled my paid developer account.  DroidStar can now be installed to an iOS device by using a marvelous program called Sideloadly:

https://sideloadly.io/

The instructions here are pretty thorough.  It is important to read the FAQ section of the website to understand how sideloading works.  I suggest enabling Wifi development on the iOS device, as explained on the website. This allows you to load/re-load DroidStar without connecting to USB.

There are static builds for all platforms out there on a few 3rd party sites such as this one:

http://pizzanbeer.net/

The ipa file is the iOS package.


