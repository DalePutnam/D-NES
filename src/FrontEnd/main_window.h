/* The main window for the application */

#pragma once

#include <mutex>
#include <atomic>
#include <thread>
#include <vector>
#include <condition_variable>

#include <wx/menu.h>
#include <wx/event.h>
#include <wx/image.h>
#include <wx/panel.h>
#include <wx/frame.h>
#include <wx/sizer.h>
#include <wx/listctrl.h>
#include <wx/bitmap.h>

class NES;
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

class MainWindow : public wxFrame
{
public:
    MainWindow();
    ~MainWindow();

    void StopEmulator(bool showRomList = true);
    void SetGameResolution(GameResolutions resolution, bool overscan);
    void SetShowFpsCounter(bool enabled);

private:
    static std::vector<std::pair<wxSize, wxSize> > ResolutionsList;

    NES* Nes;

    PPUViewerWindow* PpuWindow;
    PathSettingsWindow* PathWindow;
    AudioSettingsWindow* AudioWindow;
    VideoSettingsWindow* VideoWindow;

#ifdef _WIN32
    std::mutex PpuViewerMutex;
#elif __linux
    wxPanel* Panel;
#endif

    wxMenu* FileMenu;
    wxMenu* EmulatorMenu;
    wxMenu* SettingsMenu;
    wxMenu* AboutMenu;

    wxBoxSizer* VerticalBox;
    GameList* RomList;

    bool ShowFpsCounter;

#ifdef __linux
    std::atomic<bool> StopFlag;
    std::mutex FrameMutex;
    std::condition_variable FrameCv;
    uint8_t* FrameBuffer;
#endif

    bool OverscanEnabled;
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
    void OpenPpuViewer(wxCommandEvent& event);
    void OpenPathSettings(wxCommandEvent& event);
    void OpenAudioSettings(wxCommandEvent& event);
    void OpenVideoSettings(wxCommandEvent& event);

    void OnQuit(wxCommandEvent& event);

    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);

    void EmulatorErrorCallback(std::string err);
    void EmulatorFrameCallback(uint8_t* frameBuffer);

#ifdef __linux
    void OnUpdateFrame(wxThreadEvent& event);
#endif

    void OnUnexpectedShutdown(wxThreadEvent& event);
    void OnPpuViewerClosed(wxCommandEvent& event);
    void OnPathSettingsClosed(wxCommandEvent& event);
    void OnAudioSettingsClosed(wxCommandEvent& event);
    void OnVideoSettingsClosed(wxCommandEvent& event);

    void UpdateFrame(uint8_t* frameBuffer);
};

wxDECLARE_EVENT(EVT_NES_UPDATE_FRAME, wxThreadEvent);
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
