/*
 * emulator_worker.cc
 *
 *  Created on: Sep 15, 2014
 *      Author: Dale
 */

#include <fstream>
#include "emulator_worker.h"
#include "../main_window.h"

#ifdef DEBUG
EmulatorWorker::EmulatorWorker(std::string filename, MainWindow& caller, bool enableLogging)
	: display(*new GtkDisplay(256, 240, *this)),
	  nes(*new NES(filename, display)),
	  caller(caller),
	  frame(false),
	  log(false),
	  logRetrieved(true),
	  logbuf(0),
	  stream(0)
{
	// if logging is on then attach log stream to the log window
	if (enableLogging)
	{
		//logbuf = new LogBuffer(*this);
		//stream = new std::ostream(logbuf);
		logbuf = 0;
		stream = new std::ofstream("Log.txt");
		nes.setLogStream(*stream);
	}
}

#else

EmulatorWorker::EmulatorWorker(std::string filename, MainWindow& caller)
	: nes(*new NES(filename, *new GtkDisplay(256, 240, *this))),
	  caller(caller)
{}
#endif

EmulatorWorker::~EmulatorWorker()
{
	delete &nes;
	delete &display;
#ifdef DEBUG
	delete logbuf;
	delete stream;
#endif
}

void EmulatorWorker::startWorker(MainWindow* caller)
{
	// Start emulator and notify dispatcher
	nes.Start();
	this->caller.notify();
}

void EmulatorWorker::resumeWorker()
{
	nes.Resume();
}

void EmulatorWorker::pauseWorker()
{
	nes.Pause();
}

void EmulatorWorker::stopWorker()
{
	// Stop emulator
	nes.Stop();
}

bool EmulatorWorker::hasStopped()
{
	return nes.IsStopped();
}

void EmulatorWorker::updateFrame()
{
	frame = true;
	caller.notify();
}

Glib::RefPtr<Gdk::Pixbuf>& EmulatorWorker::getFrame()
{
	return display.GetFrame();
}

bool EmulatorWorker::isFrameUpdated()
{
	bool value = frame;
	frame = false;
	return value;
}

void EmulatorWorker::getNameTable(int table, unsigned char* pixels)
{
	nes.GetNameTable(table, pixels);
}

void EmulatorWorker::getPatternTable(int table, unsigned char* pixels)
{
	nes.GetPatternTable(table, pixels);
}

void EmulatorWorker::getPalette(int palette, unsigned char* pixels)
{
	nes.GetPalette(palette, pixels);
}

#ifdef DEBUG
bool EmulatorWorker::isLogUpdated()
{
	bool value = log;
	log = false;
	return value;
}

bool EmulatorWorker::isLogRetrieved()
{
	// Sync access to logRetrieved
	Glib::Threads::Mutex::Lock lock(retrievedMutex);
	return logRetrieved;
}

void EmulatorWorker::setLogRetrieved(bool retrieved)
{
	// Sync access to logRetrieved
	Glib::Threads::Mutex::Lock lock(retrievedMutex);
	logRetrieved = retrieved;
}

std::string EmulatorWorker::getLogChunk()
{
	// Sync access to log chunk
	Glib::Threads::Mutex::Lock lock(sinkMutex);
	return sink;
}

void EmulatorWorker::writeOutLog(char* text_begin, char* text_end)
{
	// Prevent overwriting the log chunk before the log window retrieves it
	while (!isLogRetrieved() && !hasStopped());

	if (!hasStopped()) // If emulator is running
	{
		// Sync access to log chunk
		Glib::Threads::Mutex::Lock lock(sinkMutex);
		sink = std::string(text_begin, text_end - text_begin);
		setLogRetrieved(false); // New log chunk has not been retrieved
		log = true;
		caller.notify(); // notify dispatcher
	}
}
#endif
