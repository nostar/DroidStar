# DroidStar
This software connects to M17, Fusion (YSF/FCS, DN and VW modes are supported), DMR, P25, NXDN, D-STAR (REF/XRF/DCS) reflectors and AllStar nodes (as an IAX2 client) over UDP.  It is compatible with all of the AMBE USB devices out there (ThumbDV, DVstick 30, DVSI, etc). It also supports MMDVM modems and can be used as a hotspot, or as a stand-alone transceiver via direct mode to the MMDVM device.  This software is open source and uses the cross platform C++ library called Qt.  It will build and run on Linux, Windows, MacOS, Android, and iOS. No USB device support for iOS though (AMBE vocoder or MMDVM). It should also build and run on any other posix platform that has Qt avilable (xxxBSD, Solaris, etc).  This software is provided *as-is* and no support is available.

This software makes use of software from a number of other open source software projects, including MMDVMHost, MMDVM_CM, mvoice, and others. Not only is software from these projects being used directly, but learning about the various network protocols and encoding/decoding of the various protocols was only possible thanks to the authors of all of these software projects.

# DudeStar, DroidStar, and Qt
The DudeStar application used the Qt Widgets UI, while DroidStar uses the Qt Quick UI.  All of the back end C/C++ source code for both projects has always been identical, but because of the different UI APIs, two repositories had to be maintained for the same project. Even though I prefer the Qt widgets UI over the Qt Quick UI for desktop applications, I have combined both projects into a single entity which is now simply called 'DroidStar'. My dudestar repo has been removed from github, but there are plenty of forks of it on github, in case anyone wishes to continue development of that version.

# M17 support 
The Codec2 vocoder library is open source and is included as a C++ implementation of the original C library taken from the mvoice project.  More info on M17 can be found here: https://m17project.org/

# MMDVM support -- work in progress
DroidStar supports MMDVM and MMDVM_HS (hotspot) modems, with basic (possibly buggy) support for M17, D-STAR, Fusion, and DMR.  Support for P25 and NXDN coming soon.  When connecting to a digital mode reflector/DMR server and selecting an MMDVM device under Modems, then DroidStar acts as a hotspot/repeater.  When 'MMDVM Direct' (currently M17 only) is selected as the host, then DroidStar becomes a stand-alone transceiver.

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

Talkgroup:  For DMR, enter the talkgroup ID number.  A very active TG for testing functionality on Brandmeister is 91 (Brandmeister Worldwide).  You must TX with a talkgroup entered to link to that talkgroup, just like a real radio.  Any ststics you have defined in BM selfcare will work the same way they do if you were using a hotspot/radio.

MYCALL/URCALL/RPTR1/RPTR2 are for Dstar modes REF/DCS/XRF.  These fields need to be entered correctly before attempting to TX on any DSTAR reflector.  All fields are populated with suggested values upon connect, but can still be modified for advanced users.  RPT2 is always overwritten with the current reflector upon connected.

# IAX Client for AllStar
Dudestar can connect to an AllStar node as an IAX(2) client.  See the AllStar wiki and other AllStar, Asterisk, and IAX2 protocal related websites for the technical details of IAX2 for AllStar.  This is a basic client and currently only uLaw audio codec is supported.  This is the default codec on most AllStar nodes.

Username: Defined in your nodes iax.conf file, usually iaxclient

Password: Defined as 'secret' in your iax.conf

Node[@Context]: ID and context of your AllStar node. The context is optional and if not specified, defaults to iax-client.

Host: hostname or IP address of node.

Port: UDP port of node, usually 4569.

Add DTMF commands like \*3node, \*1node, \*70, etc in the IAX DTMF box and hit send to send the DTMF string. Details on various commands can be found at the AllStar wiki and others.

# General building instructions
This software is written primarily in C++ on Linux and requires Qt5 >= Qt5.15, and natually the devel packages to build.  The imbe_vocoder library is also required.  Java, QML (Javascript based), and C# code is also used where necessary.  The preferred way to obtain Qt 5.15 is to use the Qt open source online installer from the Qt website.  Run this installer as a user (not root) to keep the Qt installation separate from your system libs.  Select the option as shown in this pic https://imgur.com/i0WuFCY which will install everything under ~/Qt.

The imbe_vocoder library is a prerequisite: https://github.com/nostar/imbe_vocoder

## Note for building on Raspbian/RaspiOS
The Qt online installer does not support RPi, but fortunately there is a great Qt 5.15.2 installer for RaspiOS:

https://github.com/koendv/qt5-opengl-raspberrypi

This installer puts everything in the same place as the Qt installer, so build instructions are the same for all:

```
cd DroidStar
mkdir build
cd build
/usr/lib/qt5.15.2/bin/qmake ..
make
```

And if pulseaudio is not currently installed:
```
sudo apt-get install pulseaudio
```
My primary development platform is Fedora Linux.  With a proper build environment, the build instructions apply to all other platforms/distributions, including Windows and macOS.

All of the gradle build files are provided to create an APK file ready to be installed on an Android device.  A proper Android build system including the Android NDK is required and beyond the scope of this document.

# No builds are available on Github
No builds for any platform are available on this Github site.  This is and always will be an open source project, to be used for educational and development purposes only. I am currently providing a Windows build which is *not* to be confused as any sort of official release of any kind.  No support will be provided for any build at any time.

http://www.dudetronics.com/


