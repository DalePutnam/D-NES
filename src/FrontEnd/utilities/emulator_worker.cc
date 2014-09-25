/*
 * emulator_worker.cc
 *
 *  Created on: Sep 15, 2014
 *      Author: Dale
 */

#include "emulator_worker.h"
#include "../main_window.h"


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


EmulatorWorker::EmulatorWorker(std::string filename, MainWindow& caller, bool enableLogging) :
	nes(*new NES(filename)),
	caller(caller),
	logRetrieved(true),
	logbuf(0),
	stream(0)
{
	// if logging is on then attach log stream to the log window
	if (enableLogging)
	{
		logbuf = new LogBuffer(*this);
		stream = new std::ostream(logbuf);
		nes.setLogStream(*stream);
	}
}

EmulatorWorker::~EmulatorWorker()
{
	delete &nes;
	delete logbuf;
	delete stream;
}

void EmulatorWorker::startWorker(MainWindow* caller)
{
	// Start emulator and notify dispatcher
	nes.Start();
	this->caller.notify();
}

void EmulatorWorker::stopWorker()
{
	// Stop emulator
	nes.Stop();
}

bool EmulatorWorker::hasStopped()
{
	return nes.isStopped();
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
		caller.notify(); // notify dispatcher
	}
}

