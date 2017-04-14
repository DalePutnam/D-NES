/* The main window for the application */

#pragma once

#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <condition_variable>

#include <wx/menu.h>
#include <wx/event.h>
#include <wx/image.h>
#include <wx/panel.h>
#include <wx/frame.h>
#include <wx/sizer.h>
#include <wx/statusbr.h>
#include <wx/treelist.h>
#include <wx/listctrl.h>
#include <wx/bitmap.h>

#include "game_list.h"
#include "ppu_debug_window.h"
#include "audio_settings_window.h"

class NES;

class MainWindow : public wxFrame
{
public:
    MainWindow();
    ~MainWindow();

    void StopEmulator(bool showRomList = true);
    void PPUDebugClose();

private:
    NES* Nes;

#ifdef _WIN32
    std::mutex PpuDebugMutex;
#endif
    PPUDebugWindow* PpuDebugWindow;
    AudioSettingsWindow* AudioWindow;

#ifdef __linux
    wxPanel* Panel;
#endif

    wxMenuBar* MenuBar;
    wxMenu* FileMenu;
    wxMenu* SizeSubMenu;
    wxMenu* EmulatorMenu;
    wxMenu* SettingsMenu;
    wxMenu* AboutMenu;

    wxBoxSizer* VerticalBox;
    GameList* RomList;

    int FpsCounter;
    std::atomic<int> CurrentFps;
    std::chrono::steady_clock::time_point IntervalStart;

    std::atomic<wxSize> GameWindowSize;

#ifdef __linux
    std::atomic<bool> StopFlag;
    std::mutex FrameMutex;
    std::condition_variable FrameCv;
    uint8_t* FrameBuffer;
#endif

    void StartEmulator(const std::string& filename);

    void ToggleCPULog(wxCommandEvent& event);
    void ToggleFrameLimit(wxCommandEvent& event);
    void ToggleNtscDecoding(wxCommandEvent& event);
    void OnSettings(wxCommandEvent& event);
    void OnROMDoubleClick(wxListEvent& event);
    void OnOpenROM(wxCommandEvent& event);
    void OnEmulatorResume(wxCommandEvent& event);
    void OnEmulatorStop(wxCommandEvent& event);
    void OnEmulatorPause(wxCommandEvent& event);
    void OnEmulatorScale(wxCommandEvent& event);
    void OnPPUDebug(wxCommandEvent& event);
    void OpenAudioSettings(wxCommandEvent& event);

    void OnQuit(wxCommandEvent& event);
    void OnSize(wxSizeEvent& event);

    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);

    void EmulatorErrorCallback(std::string err);
    void EmulatorFrameCallback(uint8_t* frameBuffer);

#ifdef __linux
    void OnUpdateFrame(wxThreadEvent& event);
#endif
    void OnUnexpectedShutdown(wxThreadEvent& event);

    void OnAudioSettingsClosed(wxCommandEvent& event);

    void UpdateFrame(uint8_t* frameBuffer);
    void UpdateFps();
};

wxDECLARE_EVENT(EVT_NES_UPDATE_FRAME, wxThreadEvent);
wxDECLARE_EVENT(EVT_NES_UNEXPECTED_SHUTDOWN, wxThreadEvent);

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
const int ID_EMULATOR_NTSC_DECODE = 114;
const int ID_SETTINGS_AUDIO = 115;
