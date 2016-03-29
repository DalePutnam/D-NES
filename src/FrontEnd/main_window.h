/* The main window for the application */

#ifndef MAIN_WINDOW_H_
#define MAIN_WINDOW_H_

#include <mutex>
#include <atomic>
#include <thread>

#include <boost/chrono/chrono.hpp>

#include "wx/menu.h"
#include "wx/event.h"
#include "wx/image.h"
#include "wx/panel.h"
#include "wx/frame.h"
#include "wx/sizer.h"
#include "wx/statusbr.h"
#include "wx/treelist.h"
#include "wx/listctrl.h"

#include "game_list.h"
#include "game_window.h"
#include "ppu_debug_window.h"

class NES;

wxDECLARE_EVENT(wxEVT_NES_UNEXPECTED_SHUTDOWN, wxThreadEvent);

class MainWindow : public wxFrame
{
    NES* nes;
    std::thread* thread;
    std::mutex sizeMutex;

    PPUDebugWindow* ppuDebugWindow;
    
    wxPanel* panel;
    wxMenuBar* menuBar;
    wxMenu* file;
    wxMenu* size;
    wxMenu* emulator;
    wxMenu* settings;
    wxMenu* about;

    wxBoxSizer* vbox;
    GameList* romList;

    int fpsCounter;
    std::atomic<int> currentFPS;
    boost::chrono::steady_clock::time_point intervalStart;
    int pixelCount;
    uint8_t frameBuffer[256 * 240 * 3];
    wxImage frame;
    std::atomic<wxSize> gameSize;
    std::atomic<bool> frameSizeDirty;

    void StartEmulator(std::string& filename);
    void UpdateImage(unsigned char* data);

    void ToggleCPULog(wxCommandEvent& event);
	void ToggleFrameLimit(wxCommandEvent& event);
    void OnSettings(wxCommandEvent& event);
    void OnROMDoubleClick(wxListEvent& event);
    void OnOpenROM(wxCommandEvent& event);
    void OnThreadUpdate(wxThreadEvent& event);
    void OnEmulatorResume(wxCommandEvent& event);
    void OnEmulatorStop(wxCommandEvent& event);
    void OnEmulatorPause(wxCommandEvent& event);
    void OnEmulatorScale(wxCommandEvent& event);
    void OnPPUDebug(wxCommandEvent& event);
    void OnUnexpectedShutdown(wxThreadEvent& event);
    void OnFPSUpdate(wxThreadEvent& event);
    void OnQuit(wxCommandEvent& event);
    void OnSize(wxSizeEvent& event);

    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);

public:
    MainWindow();
    ~MainWindow();

    void NextPixel(uint32_t pixel);
    void DrawFrame(uint8_t* frameBuffer);
    void StopEmulator(bool showRomList = true);
    void PPUDebugClose();
};

const int ID_OPEN_ROM = 100;
const int ID_EMULATOR_RESUME = 101;
const int ID_EMULATOR_PAUSE = 102;
const int ID_EMULATOR_STOP = 103;
const int ID_EMULATOR_PPU_DEBUG = 104;
const int ID_SETTINGS = 105;
const int ID_CPU_LOG = 106;
const int ID_EMULATOR_SCALE_1X = 107;
const int ID_EMULATOR_SCALE_2X = 108;
const int ID_EMULATOR_SCALE_3X = 109;
const int ID_EMULATOR_SCALE_4X = 110;
const int ID_EMULATOR_LIMIT = 111;

#endif
