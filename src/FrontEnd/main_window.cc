#include <sstream>
#include <iostream>
#include <cstring>

#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/dcbuffer.h>
#include <wx/dir.h>
#include <wx/wupdlock.h>

#include "nes.h"
#include "game_list.h"
#include "main_window.h"
#include "path_settings_window.h"
#include "ppu_viewer_window.h"
#include "audio_settings_window.h"
#include "video_settings_window.h"
#include "utilities/app_settings.h"

#ifdef __linux

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

static Window GetX11WindowHandle(void* handle)
{
    return gdk_x11_window_get_xid(gtk_widget_get_window(reinterpret_cast<GtkWidget*>(handle)));
}

#endif

wxDEFINE_EVENT(EVT_NES_UPDATE_FRAME, wxThreadEvent);
wxDEFINE_EVENT(EVT_NES_UNEXPECTED_SHUTDOWN, wxThreadEvent);

// The first resolution in each pair is the full resolution of the
// emulator, the second is the resolution at the same scale but with overscan
// taken into account
std::vector<std::pair<wxSize, wxSize> > MainWindow::ResolutionsList =
{
    { wxSize(256, 240), wxSize(256, 224) },
    { wxSize(512, 480), wxSize(512, 448) },
    { wxSize(768, 720), wxSize(768, 672) },
    { wxSize(1024, 960), wxSize(1024, 896) },
};

void MainWindow::EmulatorFrameCallback()
{
    std::unique_lock<std::mutex> lock(PpuViewerMutex);
    
    if (PpuWindow != nullptr)
    {
        PpuWindow->UpdatePanels();
    }
}

void MainWindow::OnPpuViewerClosed(wxCommandEvent& WXUNUSED(event))
{
    std::unique_lock<std::mutex> lock(PpuViewerMutex);

    if (PpuWindow != nullptr)
    {
        PpuWindow->Destroy();
        PpuWindow = nullptr;
    }
}

void MainWindow::OnPathSettingsClosed(wxCommandEvent& event)
{
    if (PathWindow != nullptr)
    {
        PathWindow->Destroy();
        PathWindow = nullptr;
    }

    if (RomList)
    {
        RomList->PopulateList();
    }
}

void MainWindow::OnAudioSettingsClosed(wxCommandEvent& event)
{
    if (AudioWindow != nullptr)
    {
        AudioWindow->Destroy();
        AudioWindow = nullptr;
    }
}

void MainWindow::OnVideoSettingsClosed(wxCommandEvent& event)
{
    if (VideoWindow != nullptr)
    {
        VideoWindow->Destroy();
        VideoWindow = nullptr;
    }
}

void MainWindow::EmulatorErrorCallback(std::string err)
{
    wxThreadEvent evt(EVT_NES_UNEXPECTED_SHUTDOWN);
    evt.SetString(err);

    wxQueueEvent(this, evt.Clone());
    SetFocus();
}

void MainWindow::StartEmulator(const std::string& filename)
{
    if (Nes == nullptr)
    {
        AppSettings* appSettings = AppSettings::GetInstance();

        NesParams params;
        params.RomPath = filename;
        params.CpuLogEnabled = SettingsMenu->FindItem(ID_CPU_LOG)->IsChecked();
		params.TurboModeEnabled = SettingsMenu->FindItem(ID_FRAME_LIMIT)->IsChecked();

        bool audioEnabled, filtersEnabled;
        appSettings->Read("/Audio/Enabled", &audioEnabled);
        appSettings->Read("/Audio/FiltersEnabled", &filtersEnabled);

        params.AudioEnabled = audioEnabled;
        params.FiltersEnabled = filtersEnabled;

        int master, pulseOne, pulseTwo, triangle, noise, dmc;
        appSettings->Read("/Audio/MasterVolume", &master);
        appSettings->Read("/Audio/PulseOneVolume", &pulseOne);
        appSettings->Read("/Audio/PulseTwoVolume", &pulseTwo);
        appSettings->Read("/Audio/TriangleVolume", &triangle);
        appSettings->Read("/Audio/NoiseVolume", &noise);
        appSettings->Read("/Audio/DmcVolume", &dmc);

        params.MasterVolume = master / 100.0f;
        params.PulseOneVolume = pulseOne / 100.0f;
        params.PulseTwoVolume = pulseTwo / 100.0f;
        params.TriangleVolume = triangle / 100.0f;
        params.NoiseVolume = noise / 100.0f;
        params.DmcVolume = dmc / 100.0f;

        RenderSurface->Show();

#ifdef _WIN32
		params.WindowHandle = RenderSurface->GetHandle();
#endif

#ifdef __linux
        params.WindowHandle = reinterpret_cast<void*>(GetX11WindowHandle(RenderSurface->GetHandle()));
#endif
        
        appSettings->Read("/Video/ShowFps", &params.FpsDisplayEnabled);
        appSettings->Read("/Video/Overscan", &params.OverscanEnabled);
        appSettings->Read("/Video/NtscDecoding", &params.NtscDecoderEnabled);
        appSettings->Read("/Paths/NativeSavePath", &params.SavePath);

        if (!wxDir::Exists(params.SavePath))
        {
            wxDir::Make(params.SavePath, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
        }

        try
        {
            Nes = new NES(params);
            Nes->BindFrameCallback(&MainWindow::EmulatorFrameCallback, this);
            Nes->BindErrorCallback(&MainWindow::EmulatorErrorCallback, this);
        }
        catch (std::exception &e)
        {
            wxMessageDialog message(nullptr, e.what(), "ERROR", wxOK | wxICON_ERROR);
            message.ShowModal();

            delete Nes;
            Nes = nullptr;

            RenderSurface->Hide();
            RomList->SetFocus();

            return;
        }

        if (PpuWindow != nullptr)
        {
            PpuWindow->SetNes(Nes);
        }

        if (AudioWindow != nullptr)
        {
            AudioWindow->SetNes(Nes);
        }

        if (VideoWindow != nullptr)
        {
            VideoWindow->SetNes(Nes);
        }

        if (PathWindow != nullptr)
        {
            PathWindow->SetNes(Nes);
        }

        GameMenuSize.SetWidth(GetSize().GetWidth());
        GameMenuSize.SetHeight(GetSize().GetHeight());

        VerticalBox->Hide(RomList);
        SetTitle(Nes->GetGameName());
        SetClientSize(GameWindowSize);

        SetMinClientSize(GameWindowSize);
        SetMaxClientSize(GameWindowSize);

        RenderSurface->SetSize(GetClientSize());

        RenderSurface->SetFocus();

        Nes->Start();
    }
    else
    {
        wxMessageDialog message(NULL, "Emulator Instance Already Running", "Cannot Complete Action", wxOK);
        message.ShowModal();
    }
}

void MainWindow::StopEmulator(bool showRomList)
{
    if (Nes != nullptr)
    {
        Nes->Stop();

        delete Nes;
        Nes = nullptr;

        if (showRomList)
        {
			RenderSurface->Hide();

            SetMinClientSize(wxSize(-1, -1));
            SetMaxClientSize(wxSize(-1, -1));

            VerticalBox->Show(RomList);
            RomList->PopulateList();
            SetSize(GameMenuSize);
            SetTitle("D-NES");
        }

        // No need for the lock here. NES thread stopped
        if (PpuWindow != nullptr)
        {
            PpuWindow->ClearAll();
            PpuWindow->SetNes(nullptr);
        }
        
        if (AudioWindow != nullptr)
        {
            AudioWindow->SetNes(nullptr);
        }

        if (VideoWindow != nullptr)
        {
            VideoWindow->SetNes(nullptr);
        }

        if (PathWindow != nullptr)
        {
            PathWindow->SetNes(nullptr);
        }
    }
}

void MainWindow::ToggleCPULog(wxCommandEvent& WXUNUSED(event))
{
    if (Nes != nullptr)
    {
        bool logEnabled = SettingsMenu->FindItem(ID_CPU_LOG)->IsChecked();
        Nes->SetCpuLogEnabled(logEnabled);
    }
}

void MainWindow::ToggleFrameLimit(wxCommandEvent& event)
{
    if (Nes != nullptr)
    {
        bool enabled = SettingsMenu->FindItem(ID_FRAME_LIMIT)->IsChecked();
        //Nes->SetFrameLimitEnabled(enabled);
		Nes->SetTurboModeEnabled(enabled);
    }
}

void MainWindow::OnROMDoubleClick(wxListEvent& event)
{
    AppSettings* settings = AppSettings::GetInstance();
    wxString filename = RomList->GetItemText(event.GetIndex(), 0);

    wxString romName;
    settings->Read("/Paths/RomPath", &romName);
    romName += "/" + filename;
    StartEmulator(romName.ToStdString());
}

void MainWindow::OnOpenROM(wxCommandEvent& WXUNUSED(event))
{
    AppSettings* settings = AppSettings::GetInstance();

    wxString romPath;
    settings->Read("/Paths/RomPath", &romPath);

    wxFileDialog dialog(NULL, "Open ROM", romPath, wxEmptyString, "NES Files (*.nes)|*.nes|All Files (*.*)|*.*");

    if (dialog.ShowModal() == wxID_OK)
    {
        StartEmulator(dialog.GetPath().ToStdString());
    }
}

void MainWindow::OnEmulatorStop(wxCommandEvent& WXUNUSED(event))
{
    StopEmulator();
}

void MainWindow::OnEmulatorSuspendResume(wxCommandEvent& WXUNUSED(event))
{
    if (Nes != nullptr)
    {
        if (Nes->IsPaused())
        {
            Nes->Resume();
        }
        else
        {
            Nes->Pause();
        }
    }
}

void MainWindow::OnSaveState(wxCommandEvent& event)
{
    if (Nes != nullptr)
    {
        AppSettings* settings = AppSettings::GetInstance();

        std::string stateSavePath;
        settings->Read("/Paths/StateSavePath", &stateSavePath);

        if (!wxDir::Exists(stateSavePath))
        {
            wxDir::Make(stateSavePath, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
        }

        try
        {
            int slot = event.GetId() % 10;
            Nes->SaveState(slot + 1, stateSavePath);
        }
        catch (std::exception& e)
        {
            wxMessageDialog message(nullptr, e.what(), "ERROR", wxOK | wxICON_ERROR);
            message.ShowModal();
        }

    }
}

void MainWindow::OnLoadState(wxCommandEvent& event)
{
    if (Nes != nullptr)
    {
        AppSettings* settings = AppSettings::GetInstance();

        std::string stateSavePath;
        settings->Read("/Paths/StateSavePath", &stateSavePath);

        try
        {
            int slot = event.GetId() % 10;
            Nes->LoadState(slot + 1, stateSavePath);
        }
        catch (std::exception&)
        {
            // Just quietly discard the exception
        }
    }
}

void MainWindow::SetGameResolution(GameResolutions resolution, bool overscan)
{
    if (!overscan)
    {
        GameWindowSize = ResolutionsList[resolution].first;
    }
    else
    {
        GameWindowSize = ResolutionsList[resolution].second;
    }

    if (Nes != nullptr)
    {
        Nes->SetOverscanEnabled(overscan);

        // To avoid setting the min size to larger than the max size
        // or vice versa, first set the min and max to allow any size
        SetMaxClientSize(wxSize(-1, -1));
        SetMinClientSize(wxSize(-1, -1));

        SetMaxClientSize(GameWindowSize);
        SetMinClientSize(GameWindowSize);

        SetClientSize(GameWindowSize);
    }
}

void MainWindow::OpenPpuViewer(wxCommandEvent& WXUNUSED(event))
{
    std::lock_guard<std::mutex> lock(PpuViewerMutex);

    if (PpuWindow == nullptr)
    {
        PpuWindow = new PPUViewerWindow(this, Nes);
        PpuWindow->Show();
    }
    else
    {
        PpuWindow->Raise();
    }
}

void MainWindow::OpenPathSettings(wxCommandEvent& WXUNUSED(event))
{
    if (PathWindow == nullptr)
    {
        PathWindow = new PathSettingsWindow(this);
    }

    PathWindow->SetNes(Nes);
    PathWindow->Show();
}

void MainWindow::OpenAudioSettings(wxCommandEvent& event)
{
    if (AudioWindow == nullptr)
    {
        AudioWindow = new AudioSettingsWindow(this);
    }

    AudioWindow->SetNes(Nes);
    AudioWindow->Show();
}

void MainWindow::OpenVideoSettings(wxCommandEvent& event)
{
    if (VideoWindow == nullptr)
    {
        VideoWindow = new VideoSettingsWindow(this);
    }

    VideoWindow->SetNes(Nes);
    VideoWindow->Show();
}

void MainWindow::OnUnexpectedShutdown(wxThreadEvent& event)
{
    wxMessageDialog message(nullptr, event.GetString(), "ERROR", wxOK | wxICON_ERROR);
    message.ShowModal();
    StopEmulator();
}

void MainWindow::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    Close();
}

void MainWindow::OnKeyDown(wxKeyEvent& event)
{
    if (Nes != nullptr)
    {
        unsigned char currentState = Nes->GetControllerOneState();

        switch (event.GetKeyCode())
        {
        case 'Z':
            Nes->SetControllerOneState(currentState | 0x1);
            break;
        case 'X':
            Nes->SetControllerOneState(currentState | 0x2);
            break;
        case WXK_CONTROL:
            Nes->SetControllerOneState(currentState | 0x4);
            break;
        case WXK_RETURN:
            Nes->SetControllerOneState(currentState | 0x8);
            break;
        case WXK_UP:
            Nes->SetControllerOneState(currentState | 0x10);
            break;
        case WXK_DOWN:
            Nes->SetControllerOneState(currentState | 0x20);
            break;
        case WXK_LEFT:
            Nes->SetControllerOneState(currentState | 0x40);
            break;
        case WXK_RIGHT:
            Nes->SetControllerOneState(currentState | 0x80);
            break;
        default:
            break;
        }
    }
}

void MainWindow::OnKeyUp(wxKeyEvent& event)
{
    if (Nes != nullptr)
    {
        unsigned char currentState = Nes->GetControllerOneState();

        switch (event.GetKeyCode())
        {
        case 'Z':
            Nes->SetControllerOneState(currentState & ~0x1);
            break;
        case 'X':
            Nes->SetControllerOneState(currentState & ~0x2);
            break;
        case WXK_CONTROL:
            Nes->SetControllerOneState(currentState & ~0x4);
            break;
        case WXK_RETURN:
            Nes->SetControllerOneState(currentState & ~0x8);
            break;
        case WXK_UP:
            Nes->SetControllerOneState(currentState & ~0x10);
            break;
        case WXK_DOWN:
            Nes->SetControllerOneState(currentState & ~0x20);
            break;
        case WXK_LEFT:
            Nes->SetControllerOneState(currentState & ~0x40);
            break;
        case WXK_RIGHT:
            Nes->SetControllerOneState(currentState & ~0x80);
            break;
        default:
            break;
        }
    }
}

void MainWindow::OnSize(wxSizeEvent& event)
{
    RenderSurface->SetSize(GetClientSize());
    event.Skip();
}

void MainWindow::InitializeMenus()
{
    FileMenu = new wxMenu;
    FileMenu->Append(ID_OPEN_ROM, wxT("&Open ROM"));
    FileMenu->AppendSeparator();
    FileMenu->Append(wxID_EXIT, wxT("&Quit"));

    StateSaveSubMenu = new wxMenu;
    StateSaveSubMenu->Append(ID_STATE_SAVE_1, wxT("&Slot 1"));
    StateSaveSubMenu->Append(ID_STATE_SAVE_2, wxT("&Slot 2"));
    StateSaveSubMenu->Append(ID_STATE_SAVE_3, wxT("&Slot 3"));
    StateSaveSubMenu->Append(ID_STATE_SAVE_4, wxT("&Slot 4"));
    StateSaveSubMenu->Append(ID_STATE_SAVE_5, wxT("&Slot 5"));
    StateSaveSubMenu->Append(ID_STATE_SAVE_6, wxT("&Slot 6"));
    StateSaveSubMenu->Append(ID_STATE_SAVE_7, wxT("&Slot 7"));
    StateSaveSubMenu->Append(ID_STATE_SAVE_8, wxT("&Slot 8"));
    StateSaveSubMenu->Append(ID_STATE_SAVE_9, wxT("&Slot 9"));
    StateSaveSubMenu->Append(ID_STATE_SAVE_10, wxT("&Slot 10"));

    StateLoadSubMenu = new wxMenu;
    StateLoadSubMenu->Append(ID_STATE_LOAD_1, wxT("&Slot 1"));
    StateLoadSubMenu->Append(ID_STATE_LOAD_2, wxT("&Slot 2"));
    StateLoadSubMenu->Append(ID_STATE_LOAD_3, wxT("&Slot 3"));
    StateLoadSubMenu->Append(ID_STATE_LOAD_4, wxT("&Slot 4"));
    StateLoadSubMenu->Append(ID_STATE_LOAD_5, wxT("&Slot 5"));
    StateLoadSubMenu->Append(ID_STATE_LOAD_6, wxT("&Slot 6"));
    StateLoadSubMenu->Append(ID_STATE_LOAD_7, wxT("&Slot 7"));
    StateLoadSubMenu->Append(ID_STATE_LOAD_8, wxT("&Slot 8"));
    StateLoadSubMenu->Append(ID_STATE_LOAD_9, wxT("&Slot 9"));
    StateLoadSubMenu->Append(ID_STATE_LOAD_10, wxT("&Slot 10"));

    EmulatorMenu = new wxMenu;
    EmulatorMenu->Append(ID_EMULATOR_SUSPEND_RESUME, wxT("&Suspend/Resume"));
    EmulatorMenu->Append(ID_EMULATOR_STOP, wxT("&Stop"));
    EmulatorMenu->AppendSeparator();
    EmulatorMenu->AppendSubMenu(StateSaveSubMenu, wxT("&Save State"));
    EmulatorMenu->AppendSubMenu(StateLoadSubMenu, wxT("&Load State"));
    EmulatorMenu->AppendSeparator();
    EmulatorMenu->Append(ID_EMULATOR_PPU_DEBUG, wxT("&PPU Viewer"));

    SettingsMenu = new wxMenu;
    SettingsMenu->AppendCheckItem(ID_CPU_LOG, wxT("&Enable CPU Log"));
    //SettingsMenu->AppendCheckItem(ID_FRAME_LIMIT, wxT("&Limit To 60 FPS"));
	SettingsMenu->AppendCheckItem(ID_FRAME_LIMIT, wxT("&Enable Turbo Mode"));
    SettingsMenu->AppendSeparator();
    SettingsMenu->Append(ID_SETTINGS_AUDIO, wxT("&Audio Settings"));
    SettingsMenu->Append(ID_SETTINGS_VIDEO, wxT("&Video Settings"));
    SettingsMenu->Append(ID_SETTINGS_PATHS, wxT("&Path Settings"));

    //SettingsMenu->FindItem(ID_FRAME_LIMIT)->Check();

    AboutMenu = new wxMenu;
    AboutMenu->Append(wxID_ANY, wxT("&About"));

    wxMenuBar* MenuBar = new wxMenuBar;
    MenuBar->Append(FileMenu, wxT("&File"));
    MenuBar->Append(EmulatorMenu, wxT("&Emulator"));
    MenuBar->Append(SettingsMenu, wxT("&Settings"));
    MenuBar->Append(AboutMenu, wxT("&About"));

    SetMenuBar(MenuBar);
}

void MainWindow::InitializeLayout()
{
    RomList = new GameList(this);
    RomList->PopulateList();

    VerticalBox = new wxBoxSizer(wxVERTICAL);
    VerticalBox->Add(RomList, 1, wxEXPAND | wxALL);
    SetSizer(VerticalBox);

    AppSettings* settings = AppSettings::GetInstance();

    int gameResolutionIndex;
    settings->Read("/Video/Resolution", &gameResolutionIndex);

    bool overscan;
    settings->Read("/Video/Overscan", &overscan);

    SetGameResolution(static_cast<GameResolutions>(gameResolutionIndex), overscan);

    int menuWidth, menuHeight;
    settings->Read("/Menu/Width", &menuWidth);
    settings->Read("/Menu/Height", &menuHeight);

    GameMenuSize = wxSize(menuWidth, menuHeight);
    SetSize(GameMenuSize);

    RenderSurface = new wxPanel(this, wxID_ANY, wxDefaultPosition, GetClientSize());
    RenderSurface->Hide();

    Centre();
}

void MainWindow::BindEvents()
{
    // Game Selected Event
    Bind(wxEVT_LIST_ITEM_ACTIVATED, wxListEventHandler(MainWindow::OnROMDoubleClick), this, wxID_ANY);

    // Menu Events
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnQuit), this, wxID_EXIT);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::ToggleCPULog), this, ID_CPU_LOG);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OpenPathSettings), this, ID_SETTINGS_PATHS);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OpenAudioSettings), this, ID_SETTINGS_AUDIO);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OpenVideoSettings), this, ID_SETTINGS_VIDEO);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnOpenROM), this, ID_OPEN_ROM);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnEmulatorStop), this, ID_EMULATOR_STOP);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnEmulatorSuspendResume), this, ID_EMULATOR_SUSPEND_RESUME);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OpenPpuViewer), this, ID_EMULATOR_PPU_DEBUG);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::ToggleFrameLimit), this, ID_FRAME_LIMIT);

    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnSaveState), this, ID_STATE_SAVE_1);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnSaveState), this, ID_STATE_SAVE_2);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnSaveState), this, ID_STATE_SAVE_3);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnSaveState), this, ID_STATE_SAVE_4);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnSaveState), this, ID_STATE_SAVE_5);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnSaveState), this, ID_STATE_SAVE_6);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnSaveState), this, ID_STATE_SAVE_7);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnSaveState), this, ID_STATE_SAVE_8);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnSaveState), this, ID_STATE_SAVE_9);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnSaveState), this, ID_STATE_SAVE_10);

    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnLoadState), this, ID_STATE_LOAD_1);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnLoadState), this, ID_STATE_LOAD_2);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnLoadState), this, ID_STATE_LOAD_3);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnLoadState), this, ID_STATE_LOAD_4);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnLoadState), this, ID_STATE_LOAD_5);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnLoadState), this, ID_STATE_LOAD_6);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnLoadState), this, ID_STATE_LOAD_7);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnLoadState), this, ID_STATE_LOAD_8);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnLoadState), this, ID_STATE_LOAD_9);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnLoadState), this, ID_STATE_LOAD_10);

    // Emulator Error Event
    Bind(EVT_NES_UNEXPECTED_SHUTDOWN, wxThreadEventHandler(MainWindow::OnUnexpectedShutdown), this, wxID_ANY);

    // Settings Windows Close Events
    Bind(EVT_PPU_VIEWER_CLOSED, wxCommandEventHandler(MainWindow::OnPpuViewerClosed), this);
    Bind(EVT_PATH_WINDOW_CLOSED, wxCommandEventHandler(MainWindow::OnPathSettingsClosed), this);
    Bind(EVT_AUDIO_WINDOW_CLOSED, wxCommandEventHandler(MainWindow::OnAudioSettingsClosed), this);
    Bind(EVT_VIDEO_WINDOW_CLOSED, wxCommandEventHandler(MainWindow::OnVideoSettingsClosed), this);

    Bind(wxEVT_SIZE, wxSizeEventHandler(MainWindow::OnSize), this, wxID_ANY);

#ifdef __linux
    // On Linux the RenderSurface needs to intercept keyboard events
    RenderSurface->Bind(wxEVT_KEY_DOWN, &MainWindow::OnKeyDown, this);
    RenderSurface->Bind(wxEVT_KEY_UP, &MainWindow::OnKeyUp, this);
#elif _WIN32
	// On windows the MainWindow need to intercept keyboard events
    Bind(wxEVT_KEY_DOWN, &MainWindow::OnKeyDown, this);
    Bind(wxEVT_KEY_UP, &MainWindow::OnKeyUp, this);
	RenderSurface->Bind(wxEVT_SET_FOCUS, &MainWindow::OnRenderSurfaceFocus, this);
#endif
}

#ifdef _WIN32
// On windows if the render surface is focused, kick focus back up to the main window
void MainWindow::OnRenderSurfaceFocus(wxFocusEvent& event)
{
	SetFocus();
}
#endif

MainWindow::MainWindow()
    : wxFrame(NULL, wxID_ANY, "D-NES")
    , Nes(nullptr)
    , PpuWindow(nullptr)
    , PathWindow(nullptr)
    , AudioWindow(nullptr)
    , VideoWindow(nullptr)
{
    InitializeMenus();
    InitializeLayout();
    BindEvents();
}

MainWindow::~MainWindow()
{
    AppSettings* settings = AppSettings::GetInstance();

    if (Nes != nullptr)
    {
        settings->Write("/Menu/Width", GameMenuSize.GetWidth());
        settings->Write("/Menu/Height", GameMenuSize.GetHeight());
    }
    else
    {
        settings->Write("/Menu/Width", GetSize().GetWidth());
        settings->Write("/Menu/Height", GetSize().GetHeight());
    }

    StopEmulator(false);
}
