#include <sstream>
#include <iostream>

#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/dir.h>
#include <wx/wupdlock.h>

#include "nes.h"
#include "main_window.h"
#include "settings_window.h"
#include "utilities/app_settings.h"

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

void MainWindow::EmulatorFrameCallback(uint8_t* frameBuffer)
{
    // Windows works just fine if the window is updated from the emulator
    // thread, but Linux, or at least X has trouble with it so we need
    // to queue a thread event on Linux builds to update the frame
#ifdef _WIN32
    UpdateFrame(frameBuffer);
    UpdateFps();
#elif __linux
    std::unique_lock<std::mutex> lock(FrameMutex);
    if (StopFlag)
    {
        return;
    }

    FrameBuffer = frameBuffer;

    wxThreadEvent evt(EVT_NES_UPDATE_FRAME);
    wxQueueEvent(this, evt.Clone());

    FrameCv.wait(lock);
#endif
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

void MainWindow::UpdateFrame(uint8_t* frameBuffer)
{
    wxImage image(256, 240, frameBuffer, true);
    wxBitmap bitmap(image, 24);

    wxMemoryDC mdc(bitmap);
    wxClientDC cdc(this);

    if (!OverscanEnabled)
    {
        cdc.StretchBlit(0, 0, GetVirtualSize().GetWidth(), GetVirtualSize().GetHeight(), &mdc, 0, 0, 256, 240);
    }
    else
    {
        cdc.StretchBlit(0, 0, GetVirtualSize().GetWidth(), GetVirtualSize().GetHeight(), &mdc, 0, 8, 256, 224);
    }

    if (ShowFpsCounter)
    {
        cdc.SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        cdc.SetTextForeground(*wxWHITE);

        std::string fpsText = std::to_string(CurrentFps);
        wxSize textSize = cdc.GetTextExtent(fpsText);

        cdc.SetPen(wxPen(*wxBLACK, 0));
        cdc.SetBrush(wxBrush(*wxBLACK));
        cdc.DrawRoundedRectangle(GetVirtualSize().GetWidth() - textSize.GetWidth() - 12, 5, textSize.GetWidth() + 7, textSize.GetHeight() + 1, 6.0);
        cdc.DrawText(fpsText, GetVirtualSize().GetWidth() - textSize.GetWidth() - 8, 6);
    }

#ifdef _WIN32
    if (PpuDebugWindow != nullptr)
    {
        std::lock_guard<std::mutex> lock(PpuDebugMutex);
        if (PpuDebugWindow != nullptr)
        {
            PpuDebugWindow->Update();
        }
    }
#elif __linux
    if (PpuDebugWindow != nullptr)
    {
        PpuDebugWindow->Update();
    }
#endif
}

void MainWindow::UpdateFps()
{
    using namespace std::chrono;

    steady_clock::time_point now = steady_clock::now();
    microseconds time_span = duration_cast<microseconds>(now - IntervalStart);
    if (time_span.count() >= 1000000)
    {
        CurrentFps = FpsCounter;
        FpsCounter = 0;
        IntervalStart = steady_clock::now();
    }
    else
    {
        FpsCounter++;
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
        params.FrameLimitEnabled = EmulatorMenu->FindItem(ID_EMULATOR_LIMIT)->IsChecked();

        bool audioEnabled, filtersEnabled;
        appSettings->Read("/Audio/Enabled", &audioEnabled);
        appSettings->Read("/Audio/FiltersEnabled", &filtersEnabled);

        params.SoundMuted = !audioEnabled;
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

        appSettings->Read("/Paths/RomSavePath", &params.SavePath);

        if (!wxDir::Exists(params.SavePath))
        {
            wxDir::Make(params.SavePath);
        }

        try
        {
            Nes = new NES(params);
            Nes->BindFrameCompleteCallback(&MainWindow::EmulatorFrameCallback, this);
            Nes->BindErrorCallback(&MainWindow::EmulatorErrorCallback, this);

            if (AudioWindow != nullptr)
            {
                AudioWindow->SetNes(Nes);
            }
        }
        catch (std::exception &e)
        {
            wxMessageDialog message(nullptr, e.what(), "ERROR", wxOK | wxICON_ERROR);
            message.ShowModal();

            delete Nes;
            Nes = nullptr;
            return;
        }

        FpsCounter = 0;
        IntervalStart = std::chrono::steady_clock::now();

#ifdef __linux
        StopFlag = false;
#endif

        Nes->Start();

        if (PpuDebugWindow != nullptr)
        {
            PpuDebugWindow->SetNes(Nes);
        }

        GameMenuSize.SetWidth(GetSize().GetWidth());
        GameMenuSize.SetHeight(GetSize().GetHeight());

        VerticalBox->Hide(RomList);
        SetTitle(Nes->GetGameName());
        SetClientSize(GameWindowSize);

        SetMinClientSize(GameWindowSize);
        SetMaxClientSize(GameWindowSize);

#ifdef __linux
        Panel->SetFocus();
#endif
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
#ifdef __linux
        FrameMutex.lock();
        StopFlag = true;
        FrameCv.notify_all();
        FrameMutex.unlock();
#endif

        Nes->Stop();
        delete Nes;
        Nes = nullptr;

        if (showRomList)
        {
            SetMinClientSize(wxSize(-1, -1));
            SetMaxClientSize(wxSize(-1, -1));

            VerticalBox->Show(RomList);
            RomList->PopulateList();
            SetSize(GameMenuSize);
            SetTitle("D-NES");
        }

        if (PpuDebugWindow != nullptr)
        {
            PpuDebugWindow->ClearAll();
            PpuDebugWindow->SetNes(nullptr);
        }

        if (AudioWindow != nullptr)
        {
            AudioWindow->SetNes(nullptr);
        }
    }
}

void MainWindow::ToggleCPULog(wxCommandEvent& WXUNUSED(event))
{
    if (Nes != nullptr)
    {
        bool logEnabled = SettingsMenu->FindItem(ID_CPU_LOG)->IsChecked();
        Nes->CpuSetLogEnabled(logEnabled);
    }
}

void MainWindow::ToggleFrameLimit(wxCommandEvent& event)
{
    if (Nes != nullptr)
    {
        bool enabled = EmulatorMenu->FindItem(ID_EMULATOR_LIMIT)->IsChecked();
        Nes->PpuSetFrameLimitEnabled(enabled);
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

void MainWindow::OnEmulatorResume(wxCommandEvent& WXUNUSED(event))
{
    if (Nes != nullptr)
    {
        Nes->Resume();
    }
}

void MainWindow::OnEmulatorStop(wxCommandEvent& WXUNUSED(event))
{
    StopEmulator();
}

void MainWindow::OnEmulatorPause(wxCommandEvent& WXUNUSED(event))
{
    if (Nes != nullptr)
    {
        Nes->Pause();
    }
}

void MainWindow::SetGameResolution(GameResolutions resolution, bool overscan)
{
    OverscanEnabled = overscan;

    if (!OverscanEnabled)
    {
        GameWindowSize = ResolutionsList[resolution].first;
    }
    else
    {
        GameWindowSize = ResolutionsList[resolution].second;
    }

    if (Nes != nullptr)
    {
        // To avoid setting the min size to larger than the max size
        // or vice versa, first set the min and max to allow any size
        SetMaxClientSize(wxSize(-1, -1));
        SetMinClientSize(wxSize(-1, -1));

        SetMaxClientSize(GameWindowSize);
        SetMinClientSize(GameWindowSize);

        SetClientSize(GameWindowSize);
    }
}

void MainWindow::SetShowFpsCounter(bool enabled)
{
    ShowFpsCounter = enabled;
}

void MainWindow::OnPPUDebug(wxCommandEvent& WXUNUSED(event))
{
#ifdef _WIN32
    std::lock_guard<std::mutex> lock(PpuDebugMutex);
#endif

    if (PpuDebugWindow == nullptr)
    {
        PpuDebugWindow = new PPUDebugWindow(this, Nes);
        PpuDebugWindow->Show();
    }
}

void MainWindow::OpenPathSettings(wxCommandEvent& WXUNUSED(event))
{
    SettingsWindow settings;
    if (settings.ShowModal() == wxID_OK)
    {
        settings.SaveSettings();
    }

    if (RomList)
    {
        RomList->PopulateList();
    }
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


void MainWindow::PPUDebugClose()
{
#ifdef _WIN32
    std::lock_guard<std::mutex> lock(PpuDebugMutex);
#endif

    if (PpuDebugWindow != nullptr)
    {
        PpuDebugWindow->Destroy();
        PpuDebugWindow = nullptr;
    }
}

#ifdef __linux
void MainWindow::OnUpdateFrame(wxThreadEvent& event)
{
    std::unique_lock<std::mutex> lock(FrameMutex);

    if (Nes == nullptr)
    {
        FrameCv.notify_all();
        return;
    }

    UpdateFrame(FrameBuffer);
    UpdateFps();

    FrameCv.notify_all();
}
#endif

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

void MainWindow::InitializeMenus()
{
    FileMenu = new wxMenu;
    FileMenu->Append(ID_OPEN_ROM, wxT("&Open ROM"));
    FileMenu->AppendSeparator();
    FileMenu->Append(wxID_EXIT, wxT("&Quit"));

    EmulatorMenu = new wxMenu;
    EmulatorMenu->Append(ID_EMULATOR_PAUSE, wxT("&Pause"));
    EmulatorMenu->Append(ID_EMULATOR_RESUME, wxT("&Resume"));
    EmulatorMenu->Append(ID_EMULATOR_STOP, wxT("&Stop"));
    EmulatorMenu->AppendSeparator();
    EmulatorMenu->AppendCheckItem(ID_EMULATOR_LIMIT, wxT("&Limit To 60 FPS"));
    EmulatorMenu->FindItem(ID_EMULATOR_LIMIT)->Check();
    EmulatorMenu->AppendSeparator();
    EmulatorMenu->Append(ID_EMULATOR_PPU_DEBUG, wxT("&PPU Debugger"));

    SettingsMenu = new wxMenu;
    SettingsMenu->AppendCheckItem(ID_CPU_LOG, wxT("&Enable CPU Log"));
    SettingsMenu->Append(ID_SETTINGS_AUDIO, wxT("&Audio Settings"));
    SettingsMenu->Append(ID_SETTINGS_VIDEO, wxT("&Video Settings"));
    SettingsMenu->Append(ID_SETTINGS_PATHS, wxT("&Path Settings"));

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

    settings->Read("/Video/ShowFps", &ShowFpsCounter);

    int menuWidth, menuHeight;
    settings->Read("/Menu/Width", &menuWidth);
    settings->Read("/Menu/Height", &menuHeight);

    GameMenuSize = wxSize(menuWidth, menuHeight);
    SetSize(GameMenuSize);

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
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnEmulatorResume), this, ID_EMULATOR_RESUME);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnEmulatorStop), this, ID_EMULATOR_STOP);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnEmulatorPause), this, ID_EMULATOR_PAUSE);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnPPUDebug), this, ID_EMULATOR_PPU_DEBUG);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::ToggleFrameLimit), this, ID_EMULATOR_LIMIT);

    // Emulator Error Event
    Bind(EVT_NES_UNEXPECTED_SHUTDOWN, wxThreadEventHandler(MainWindow::OnUnexpectedShutdown), this, wxID_ANY);

    // Settings Windows Close Events
    Bind(EVT_AUDIO_WINDOW_CLOSED, wxCommandEventHandler(MainWindow::OnAudioSettingsClosed), this);
    Bind(EVT_VIDEO_WINDOW_CLOSED, wxCommandEventHandler(MainWindow::OnVideoSettingsClosed), this);

#ifdef __linux
    // On Linux we need a panel to intercept keyboard events for some reason
    // Only used for this, so I'm leaving it here rather than putting it in InitializeLayout
    Panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS);
    Panel->Bind(wxEVT_KEY_DOWN, &MainWindow::OnKeyDown, this);
    Panel->Bind(wxEVT_KEY_UP, &MainWindow::OnKeyUp, this);

    Bind(EVT_NES_UPDATE_FRAME, wxThreadEventHandler(MainWindow::OnUpdateFrame), this, wxID_ANY);
#elif _WIN32
    Bind(wxEVT_KEY_DOWN, &MainWindow::OnKeyDown, this);
    Bind(wxEVT_KEY_UP, &MainWindow::OnKeyUp, this);
#endif
}

MainWindow::MainWindow()
    : wxFrame(NULL, wxID_ANY, "D-NES")
    , Nes(nullptr)
    , PpuDebugWindow(nullptr)
    , AudioWindow(nullptr)
    , VideoWindow(nullptr)
    , FpsCounter(0)
    , CurrentFps(0)
    , IntervalStart(std::chrono::steady_clock::now())
#ifdef __linux
    , StopFlag(false)
#endif
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
    PPUDebugClose();
}
