/*
 * emulator_worker.h
 *
 *  Created on: Sep 15, 2014
 *      Author: Dale
 */

#ifndef EMULATOR_WORKER_H_
#define EMULATOR_WORKER_H_

#include <iostream>
#include <string>
#include <gtkmm/textbuffer.h>
#include <glibmm/threads.h>

#include "log_buffer.h"
#include "../../Emulator/nes.h"

class MainWindow;

class EmulatorWorker
{
	NES& nes; // NES Emulator
	MainWindow& caller; // MainWindow that created this instance

	Glib::Threads::Mutex sinkMutex; // Log Mutual Exclusion
	Glib::Threads::Mutex retrievedMutex; // Log Retrieved Mutual Exclusion

	bool logRetrieved; // Has the current log chunk been retrieved
	std::string sink; // The current log chunk

	// Output stream objects for NES to write to
	LogBuffer* logbuf;
	std::ostream* stream;

public:
	EmulatorWorker(std::string filename, MainWindow& caller, bool enableLogging = false);
	~EmulatorWorker();

	void startWorker(MainWindow* caller); // Start NES
	void stopWorker(); // Stop NES

	bool hasStopped(); // Is NES stopped

	bool isLogRetrieved(); // Has the current log chunk been retrieved
	void setLogRetrieved(bool retrieved); // Set if the the log chunk has been retrieved

	std::string getLogChunk(); // Get current log chunk
	void writeOutLog(char* text_begin, char* text_end); // Write log to sink
};



#endif /* EMULATOR_WORKER_H_ */
