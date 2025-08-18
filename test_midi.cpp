/*
	Simple MIDI test program for DroidStar development
	
	This program demonstrates basic RtMidi functionality and can be used
	to test MIDI devices before using them with DroidStar.
	
	Compile with:
	g++ -o test_midi test_midi.cpp -lrtmidi -lpthread
	
	Or on Windows:
	g++ -o test_midi test_midi.cpp -lrtmidi -lwinmm
*/

#ifdef ENABLE_MIDI
#include "RtMidi.h"
#include <iostream>
#include <cstdlib>
#include <signal.h>

bool done = false;

static void finish(int ignore) { 
	done = true; 
}

void midiCallback(double deltatime, std::vector<unsigned char> *message, void *userData) {
	unsigned int nBytes = message->size();
	if (nBytes == 0) return;
	
	for (unsigned int i = 0; i < nBytes; i++) {
		std::cout << "Byte " << i << " = " << (int)message->at(i) << ", ";
	}
	
	if (nBytes >= 3) {
		unsigned char status = message->at(0);
		unsigned char note = message->at(1);
		unsigned char velocity = message->at(2);
		
		if ((status & 0xF0) == 0x90 && velocity > 0) {
			std::cout << " -> Note ON: " << (int)note << " velocity: " << (int)velocity;
		} else if ((status & 0xF0) == 0x80 || ((status & 0xF0) == 0x90 && velocity == 0)) {
			std::cout << " -> Note OFF: " << (int)note;
		}
	}
	
	std::cout << std::endl;
}

int main() {
	RtMidiIn *midiin = 0;
	
	try {
		midiin = new RtMidiIn();
	} catch (RtMidiError &error) {
		std::cout << "RtMidi initialization failed: " << error.getMessage() << std::endl;
		exit(EXIT_FAILURE);
	}
	
	// Check available ports
	unsigned int nPorts = midiin->getPortCount();
	if (nPorts == 0) {
		std::cout << "No MIDI input ports available!" << std::endl;
		delete midiin;
		return 0;
	}
	
	std::cout << "Available MIDI input ports:" << std::endl;
	for (unsigned int i = 0; i < nPorts; i++) {
		try {
			std::string portName = midiin->getPortName(i);
			std::cout << "  Port " << i << ": " << portName << std::endl;
		} catch (RtMidiError &error) {
			std::cout << "  Port " << i << ": Error getting name - " << error.getMessage() << std::endl;
		}
	}
	
	// Open first available port
	std::cout << "\nOpening port 0..." << std::endl;
	try {
		midiin->openPort(0);
	} catch (RtMidiError &error) {
		std::cout << "Failed to open port: " << error.getMessage() << std::endl;
		delete midiin;
		return 1;
	}
	
	// Set callback
	midiin->setCallback(&midiCallback);
	
	// Don't ignore sysex, timing, or active sensing messages
	midiin->ignoreTypes(false, false, false);
	
	std::cout << "Listening for MIDI input... Press Ctrl-C to quit." << std::endl;
	std::cout << "Play some notes or press buttons on your MIDI device." << std::endl;
	
	// Install interrupt handler
	(void) signal(SIGINT, finish);
	
	// Periodic sleep until we get interrupt
	while (!done) {
		usleep(50000); // sleep for 50ms
	}
	
	std::cout << "\nCleaning up..." << std::endl;
	delete midiin;
	return 0;
}

#else

#include <iostream>

int main() {
	std::cout << "MIDI support not compiled in. Define ENABLE_MIDI and link with RtMidi." << std::endl;
	return 1;
}

#endif