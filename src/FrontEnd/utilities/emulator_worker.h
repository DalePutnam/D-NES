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

#include "../../Emulator/nes.h"
#include "gtk_display.h"

#ifdef DEBUG
#include "log_buffer.h"
#endif

class MainWindow;

class EmulatorWorker
{
	GtkDisplay& display;
	NES& nes; // NES Emulator
	MainWindow& caller; // MainWindow that created this instance
	bool frame;

	Glib::Threads::Mutex sinkMutex; // Log Mutual Exclusion
	Glib::Threads::Mutex frameMutex; // Frame Mutual Exclusion
	Glib::Threads::Mutex retrievedMutex; // Log Retrieved Mutual Exclusion

#ifdef DEBUG
	bool log;
	bool logRetrieved; // Has the current log chunk been retrieved
	std::string sink; // The current log chunk

	// Output stream objects for NES to write to
	LogBuffer* logbuf;
	std::ostream* stream;
#endif

public:
#ifdef DEBUG
	EmulatorWorker(std::string filename, MainWindow& caller, bool enableLogging = false);
#else
	EmulatorWorker(std::string filename, MainWindow& caller);
#endif
	~EmulatorWorker();

	void startWorker(MainWindow* caller); // Start NES
	void resumeWorker(); // Resume paused NES
	void pauseWorker(); // Pause NES
	void stopWorker(); // Stop NES
	bool hasStopped(); // Is NES stopped
	void updateFrame();
	Glib::RefPtr<Gdk::Pixbuf>& getFrame();
	bool isFrameUpdated();
	void getNameTable(int table, unsigned char* pixels);
	void getPatternTable(int table, unsigned char* pixels);
	void getPalette(int palette, unsigned char* pixels);
#ifdef DEBUG
	bool isLogUpdated();
	bool isLogRetrieved(); // Has the current log chunk been retrieved
	void setLogRetrieved(bool retrieved); // Set if the the log chunk has been retrieved
	std::string getLogChunk(); // Get current log chunk
	void writeOutLog(char* text_begin, char* text_end); // Write log to sink
#endif
};



#endif /* EMULATOR_WORKER_H_ */
