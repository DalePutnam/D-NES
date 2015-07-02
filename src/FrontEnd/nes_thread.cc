#include "nes_thread.h"
#include "main_window.h"

wxDEFINE_EVENT(wxEVT_COMMAND_NESTHREAD_FRAME_UPDATE, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_COMMAND_NESTHREAD_FPS_UPDATE, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_COMMAND_NESTHREAD_UNEXPECTED_SHUTDOWN, wxThreadEvent);

wxThread::ExitCode NESThread::Entry()
{
    if (!TestDestroy())
    {
        nes.Start();
    }

    if (!expectedStop)
    {
        wxQueueEvent(handler, new wxThreadEvent(wxEVT_COMMAND_NESTHREAD_UNEXPECTED_SHUTDOWN));
    }

    return static_cast<wxThread::ExitCode>(0);
}

NESThread::NESThread(MainWindow* handler, std::string& filename, bool cpuLogEnabled)
    : wxThread(wxTHREAD_JOINABLE),
    handler(handler),
    nes(*new NES(filename, *this, cpuLogEnabled)),
    expectedStop(false),
    fpsCounter(0),
    currentFPS(0),
    intervalStart(boost::chrono::steady_clock::now()),
    width(256),
    height(240),
    pixelCount(0),
    pixelArray(new unsigned char[width*height * 3])
{
    for (int i = 0; i < 64; ++i)
    {
        if (i < 2) patternTable[i] = 0;
        if (i < 4) nameTable[i] = 0;

        if (i < 8)
        {
            palette[i] = 0;
            secondarySprite[i] = 0;
        }

        primarySprite[i] = 0;
    }
}

NESThread::~NESThread()
{
    delete &nes;

    for (int i = 0; i < 64; ++i)
    {
        if (i < 2)
        {
            if (patternTable[i]) delete[] patternTable[i];
        }

        if (i < 4)
        {
            if (nameTable[i]) delete[] nameTable[i];
        }

        if (i < 8)
        {
            if (palette[i]) delete[] palette[i];
            if (secondarySprite[i]) delete[] secondarySprite[i];
        }

        if (primarySprite[i]) delete[] primarySprite[i];
    }

    delete[] pixelArray;
}

std::string& NESThread::GetGameName()
{
    return nes.GetGameName();
}

void NESThread::EnableCPULog()
{
    nes.EnableCPULog();
}

void NESThread::DisableCPULog()
{
    nes.DisableCPULog();
}

void NESThread::EmulatorResume()
{
    if (nes.IsPaused()) nes.Resume();
}

void NESThread::EmulatorPause()
{
    if (!nes.IsPaused()) nes.Pause();
}

void NESThread::Stop()
{
    expectedStop = true;
    nes.Stop();
}

void NESThread::SetControllerOneState(unsigned char state)
{
    nes.SetControllerOneState(state);
}

unsigned char NESThread::GetControllerOneState()
{
    return nes.GetControllerOneState();
}

void NESThread::NextPixel(unsigned int pixel)
{
    while (frameLocked);

    unsigned char red = static_cast<unsigned char>((pixel & 0xFF0000) >> 16);
    unsigned char green = static_cast<unsigned char>((pixel & 0x00FF00) >> 8);
    unsigned char blue = static_cast<unsigned char>(pixel & 0x0000FF);

    pixelArray[pixelCount * 3] = red;
    pixelArray[(pixelCount * 3) + 1] = green;
    pixelArray[(pixelCount * 3) + 2] = blue;
    ++pixelCount;

    if (pixelCount == width * height)
    {
        frameLocked = true;
        wxQueueEvent(handler, new wxThreadEvent(wxEVT_COMMAND_NESTHREAD_FRAME_UPDATE));
        pixelCount = 0;
        fpsCounter++;

        using namespace boost::chrono;

        steady_clock::time_point now = steady_clock::now();
        microseconds time_span = duration_cast<microseconds>(now - intervalStart);
        if (time_span.count() >= 1000000)
        {
            currentFPS.store(fpsCounter);
            fpsCounter = 0;
            intervalStart = steady_clock::now();
            wxQueueEvent(handler, new wxThreadEvent(wxEVT_COMMAND_NESTHREAD_FPS_UPDATE));
        }
    }
}

unsigned char* NESThread::GetFrame()
{
    return pixelArray;
}

void NESThread::UnlockFrame()
{
    frameLocked = false;
}

int NESThread::GetCurrentFPS()
{
    return currentFPS.load();
}

unsigned char* NESThread::GetNameTable(int tableID)
{
    if (!nameTable[tableID]) nameTable[tableID] = new unsigned char[184320];
    nes.GetNameTable(tableID, nameTable[tableID]);
    return nameTable[tableID];
}

unsigned char* NESThread::GetPatternTable(int tableID, int paletteID)
{
    if (!patternTable[tableID]) patternTable[tableID] = new unsigned char[49152];
    nes.GetPatternTable(tableID, paletteID, patternTable[tableID]);
    return patternTable[tableID];
}

unsigned char* NESThread::GetPalette(int tableID)
{
    if (!palette[tableID]) palette[tableID] = new unsigned char[3072];
    nes.GetPalette(tableID, palette[tableID]);
    return palette[tableID];
}

unsigned char* NESThread::GetPrimarySprite(int sprite)
{
    if (!primarySprite[sprite]) primarySprite[sprite] = new unsigned char[192];
    nes.GetPrimaryOAM(sprite, primarySprite[sprite]);
    return primarySprite[sprite];
}

unsigned char* NESThread::GetSecondarySprite(int sprite)
{
    if (!secondarySprite[sprite]) secondarySprite[sprite] = new unsigned char[192];
    nes.GetSecondaryOAM(sprite, secondarySprite[sprite]);
    return secondarySprite[sprite];
}