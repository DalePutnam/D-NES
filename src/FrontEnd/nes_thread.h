#ifndef NES_THREAD_H_
#define NES_THREAD_H_

#include "wx/thread.h"
#include "wx/event.h"

#include "nes.h"
#include "Interfaces/idisplay.h"

wxDECLARE_EVENT(wxEVT_COMMAND_NESTHREAD_UPDATE, wxThreadEvent);
//wxDECLARE_EVENT(wxEVT_COMMAND_NESTHREAD_COMPLETED, wxThreadEvent);

class MainWindow;

class NESThread : public wxThread, public IDisplay
{
	MainWindow* handler;
	NES& nes;

	unsigned char* nameTable0;
	unsigned char* nameTable1;
	unsigned char* nameTable2;
	unsigned char* nameTable3;
	unsigned char* patternTable0;
	unsigned char* patternTable1;
	unsigned char* palette0;
	unsigned char* palette1;
	unsigned char* palette2;
	unsigned char* palette3;
	unsigned char* palette4;
	unsigned char* palette5;
	unsigned char* palette6;
	unsigned char* palette7;

	int width;
	int height;
	int pixelCount;
	unsigned char* pixelArray;

	bool frameLocked;

	virtual wxThread::ExitCode Entry();

public:
	NESThread(MainWindow* handler, std::string& filename);
	~NESThread();

	void EmulatorResume();
	void EmulatorPause();
	void Stop();

	virtual void NextPixel(unsigned int pixel);
	unsigned char* GetFrame();
	void UnlockFrame();

	unsigned char* GetNameTable(int tableID);
	unsigned char* GetPatternTable(int tableID, int paletteID);
	unsigned char* GetPalette(int tableID);
};

#endif