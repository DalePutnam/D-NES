/* The main window for the application */

#pragma once

#include <mutex>
#include <vector>
#include <memory>

#include <wx/menu.h>
#include <wx/event.h>
#include <wx/image.h>
#include <wx/panel.h>
#include <wx/frame.h>
#include <wx/sizer.h>
#include <wx/listctrl.h>
#include <wx/bitmap.h>

#include "nes.h"

class GameList;
class PPUViewerWindow;
class PathSettingsWindow;
class AudioSettingsWindow;
class VideoSettingsWindow;

enum GameResolutions
{
    _256X240,
    _512X480,
    _768X720,
    _1024X960,
    NUM_RESOLUTIONS
};

class MainWindow : public wxFrame, public NESCallback
{
public:
    MainWindow();
    ~MainWindow() noexcept;

    void StopEmulator(bool showRomList = true);
    void SetGameResolution(GameResolutions resolution, bool overscan);

private:
    static std::vector<std::pair<wxSize, wxSize>> ResolutionsList;

    std::unique_ptr<NES> Nes;

    std::mutex PpuViewerMutex;
    PPUViewerWindow* PpuWindow;
    PathSettingsWindow* PathWindow;
    AudioSettingsWindow* AudioWindow;
    VideoSettingsWindow* VideoWindow;

    wxMenu* FileMenu;
    wxMenu* EmulatorMenu;
    wxMenu* SettingsMenu;
    wxMenu* AboutMenu;

    wxMenuItem* PlayPauseMenuItem;

    wxMenu* StateSaveSubMenu;
    wxMenu* StateLoadSubMenu;

    wxBoxSizer* VerticalBox;
    GameList* RomList;

    wxPanel* RenderSurface;

    wxSize GameWindowSize;
    wxSize GameMenuSize;

    void InitializeMenus();
    void InitializeLayout();
    void BindEvents();

    void StartEmulator(const std::string& filename);

    void ToggleCPULog(wxCommandEvent& event);
    void ToggleFrameLimit(wxCommandEvent& event);
    void OnROMDoubleClick(wxListEvent& event);
    void OnOpenROM(wxCommandEvent& event);
    void OnEmulatorSuspendResume(wxCommandEvent& event);
    void OnEmulatorStop(wxCommandEvent& event);
    void OnSaveState(wxCommandEvent& event);
    void OnLoadState(wxCommandEvent& event);
    void OpenPpuViewer(wxCommandEvent& event);
    void OpenPathSettings(wxCommandEvent& event);
    void OpenAudioSettings(wxCommandEvent& event);
    void OpenVideoSettings(wxCommandEvent& event);

    void OnQuit(wxCommandEvent& event);
    void OnSize(wxSizeEvent& event);

    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);

    void EmulatorErrorCallback(std::string err);
    void EmulatorFrameCallback();

    void OnUnexpectedShutdown(wxThreadEvent& event);
    void OnPpuViewerClosed(wxCommandEvent& event);
    void OnPathSettingsClosed(wxCommandEvent& event);
    void OnAudioSettingsClosed(wxCommandEvent& event);
    void OnVideoSettingsClosed(wxCommandEvent& event);

#ifdef _WIN32
    void OnRenderSurfaceFocus(wxFocusEvent& event);
#endif

    // NESCallback functions

    void OnFrameComplete() override;
    void OnError(std::exception_ptr eptr) override;
};

wxDECLARE_EVENT(EVT_NES_UNEXPECTED_SHUTDOWN, wxThreadEvent);

const int ID_OPEN_ROM = 100;
const int ID_EMULATOR_SUSPEND_RESUME = 101;
const int ID_EMULATOR_STOP = 102;
const int ID_EMULATOR_PPU_DEBUG = 103;
const int ID_CPU_LOG = 104;
const int ID_FRAME_LIMIT = 105;
const int ID_SETTINGS_AUDIO = 106;
const int ID_SETTINGS_VIDEO = 107;
const int ID_SETTINGS_PATHS = 108;
const int ID_STATE_SAVE_1 = 110;
const int ID_STATE_SAVE_2 = 111;
const int ID_STATE_SAVE_3 = 112;
const int ID_STATE_SAVE_4 = 113;
const int ID_STATE_SAVE_5 = 114;
const int ID_STATE_SAVE_6 = 115;
const int ID_STATE_SAVE_7 = 116;
const int ID_STATE_SAVE_8 = 117;
const int ID_STATE_SAVE_9 = 118;
const int ID_STATE_SAVE_10 = 119;
const int ID_STATE_LOAD_1 = 120;
const int ID_STATE_LOAD_2 = 121;
const int ID_STATE_LOAD_3 = 122;
const int ID_STATE_LOAD_4 = 123;
const int ID_STATE_LOAD_5 = 124;
const int ID_STATE_LOAD_6 = 125;
const int ID_STATE_LOAD_7 = 126;
const int ID_STATE_LOAD_8 = 127;
const int ID_STATE_LOAD_9 = 128;
const int ID_STATE_LOAD_10 = 129;
