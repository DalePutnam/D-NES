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

	unsigned char* nameTable[4];
	unsigned char* patternTable[2];
	unsigned char* palette[8];
	unsigned char* primarySprite[64];
	unsigned char* secondarySprite[8];

	int width;
	int height;
	int pixelCount;
	unsigned char* pixelArray;

	volatile bool frameLocked;

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
	unsigned char* GetPrimarySprite(int sprite);
	unsigned char* GetSecondarySprite(int sprite);
};

#endif