/* The main window for the application */

#pragma once

#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>

#include <wx/menu.h>
#include <wx/event.h>
#include <wx/image.h>
#include <wx/panel.h>
#include <wx/frame.h>
#include <wx/sizer.h>
#include <wx/statusbr.h>
#include <wx/treelist.h>
#include <wx/listctrl.h>

#include "game_list.h"
#include "game_window.h"
#include "ppu_debug_window.h"

class NES;

wxDECLARE_EVENT(wxEVT_NES_UNEXPECTED_SHUTDOWN, wxThreadEvent);

class MainWindow : public wxFrame
{
    NES* nes;
    std::mutex PpuDebugMutex;

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
    std::chrono::steady_clock::time_point intervalStart;
    std::atomic<wxSize> gameSize;

    void StartEmulator(const std::string& filename);

    void ToggleCPULog(wxCommandEvent& event);
    void ToggleFrameLimit(wxCommandEvent& event);
    void ToggleMute(wxCommandEvent& event);
    void ToggleFilters(wxCommandEvent& event);
    void ToggleNtscDecoding(wxCommandEvent& event);
    void OnSettings(wxCommandEvent& event);
    void OnROMDoubleClick(wxListEvent& event);
    void OnOpenROM(wxCommandEvent& event);
    void OnEmulatorResume(wxCommandEvent& event);
    void OnEmulatorStop(wxCommandEvent& event);
    void OnEmulatorPause(wxCommandEvent& event);
    void OnEmulatorScale(wxCommandEvent& event);
    void OnPPUDebug(wxCommandEvent& event);
    void OnUnexpectedShutdown(wxThreadEvent& event);
    void OnQuit(wxCommandEvent& event);
    void OnSize(wxSizeEvent& event);

    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);

    void OnEmulatorError(std::string err);
    void OnEmulatorFrameComplete(uint8_t* frameBuffer);

public:
    MainWindow();
    ~MainWindow();

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
const int ID_EMULATOR_MUTE = 112;
const int ID_EMULATOR_FILTER = 113;
const int ID_EMULATOR_NTSC_DECODE = 114;
