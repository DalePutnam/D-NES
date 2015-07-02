#ifndef NES_THREAD_H_
#define NES_THREAD_H_

#include <boost\chrono\chrono.hpp>
#include <atomic>
#include "wx/thread.h"
#include "wx/event.h"

#include "nes.h"
#include "Interfaces/idisplay.h"

wxDECLARE_EVENT(wxEVT_COMMAND_NESTHREAD_FRAME_UPDATE, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_COMMAND_NESTHREAD_FPS_UPDATE, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_COMMAND_NESTHREAD_UNEXPECTED_SHUTDOWN, wxThreadEvent);

class MainWindow;

class NESThread : public wxThread, public IDisplay
{
    MainWindow* handler;
    NES& nes;

    bool expectedStop;
    int fpsCounter;
    std::atomic<int> currentFPS;
    boost::chrono::steady_clock::time_point intervalStart;

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
    NESThread(MainWindow* handler, std::string& filename, bool cpuLogEnabled = false);
    ~NESThread();

    std::string& GetGameName();

    void EmulatorResume();
    void EmulatorPause();
    void Stop();

    void SetControllerOneState(unsigned char state);
    unsigned char GetControllerOneState();

    virtual void NextPixel(unsigned int pixel);
    unsigned char* GetFrame();
    void UnlockFrame();

    void EnableCPULog();
    void DisableCPULog();

    int GetCurrentFPS();

    unsigned char* GetNameTable(int tableID);
    unsigned char* GetPatternTable(int tableID, int paletteID);
    unsigned char* GetPalette(int tableID);
    unsigned char* GetPrimarySprite(int sprite);
    unsigned char* GetSecondarySprite(int sprite);
};

#endif