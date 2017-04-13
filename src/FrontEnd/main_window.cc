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

void MainWindow::EmulatorFrameCallback(uint8_t* frameBuffer)
{
    // Windows works just fine if the window is updated from the emulator
    // thread, but Linux, or at least X has trouble with it so we need
    // to queue a thread event on Linux builds to update the frame
#ifdef _WIN32
    UpdateFrame(frameBuffer);
    UpdateFps();
#else __linux
    std::unique_lock<std::mutex> lock(FrameMutex);
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

void MainWindow::UpdateFrame(uint8_t* frameBuffer)
{
    wxImage image(256, 240, frameBuffer, true);
    wxBitmap bitmap(image, 24);

    wxMemoryDC mdc(bitmap);
    wxClientDC cdc(this);
    cdc.StretchBlit(0, 0, GetVirtualSize().GetWidth(), GetVirtualSize().GetHeight(), &mdc, 0, 0, 256, 240);

#ifdef _WIN32
    if (PpuDebugWindow != nullptr)
    {
        std::lock_guard<std::mutex> lock(PpuDebugMutex);
        if (PpuDebugWindow != nullptr)
        {
            PpuDebugWindow->Update();
        }
    }
#else __linux
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

        std::ostringstream oss;
        oss << CurrentFps << " FPS - " << Nes->GetGameName();
        SetTitle(oss.str());
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
        Nes->Start();

        if (PpuDebugWindow != nullptr)
        {
            PpuDebugWindow->SetNes(Nes);
        }

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
        Nes->Stop();
        delete Nes;
        Nes = nullptr;

        if (showRomList)
        {
            SetMinClientSize(wxSize(-1, -1));
            SetMaxClientSize(wxSize(-1, -1));

            VerticalBox->Show(RomList);
            RomList->PopulateList();
            SetSize(wxSize(600, 460));
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

void MainWindow::ToggleNtscDecoding(wxCommandEvent& event)
{
    if (Nes != nullptr)
    {
        bool enabled = EmulatorMenu->FindItem(ID_EMULATOR_NTSC_DECODE)->IsChecked();
        Nes->PpuSetNtscDecoderEnabled(enabled);
    }
}

void MainWindow::OnSettings(wxCommandEvent& WXUNUSED(event))
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

void MainWindow::OnEmulatorScale(wxCommandEvent& WXUNUSED(event))
{
    if (SizeSubMenu->IsChecked(ID_EMULATOR_SCALE_1X))
    {
        GameWindowSize = wxSize(256, 240);
    }
    else if (SizeSubMenu->IsChecked(ID_EMULATOR_SCALE_2X))
    {
        GameWindowSize = wxSize(512, 480);
    }
    else if (SizeSubMenu->IsChecked(ID_EMULATOR_SCALE_3X))
    {
        GameWindowSize = wxSize(768, 720);
    }
    else if (SizeSubMenu->IsChecked(ID_EMULATOR_SCALE_4X))
    {
        GameWindowSize = wxSize(1024, 960);
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

void MainWindow::OpenAudioSettings(wxCommandEvent& event)
{
    if (AudioWindow == nullptr)
    {
        AudioWindow = new AudioSettingsWindow(this);
    }

    AudioWindow->SetNes(Nes);
    AudioWindow->Show();
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
    StopEmulator(false);
    Close(true);
}

void MainWindow::OnSize(wxSizeEvent& event)
{
    if (Nes != nullptr)
    {
        //event.Skip();
    }
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

MainWindow::MainWindow()
    : wxFrame(NULL, wxID_ANY, "D-NES", wxDefaultPosition, wxSize(600, 460))
    , Nes(nullptr)
    , PpuDebugWindow(nullptr)
    , FpsCounter(0)
    , CurrentFps(0)
    , IntervalStart(std::chrono::steady_clock::now())
    , GameWindowSize(wxSize(256, 240))
{
    FileMenu = new wxMenu;
    FileMenu->Append(ID_OPEN_ROM, wxT("&Open ROM"));
    FileMenu->AppendSeparator();
    FileMenu->Append(wxID_EXIT, wxT("&Quit"));

    SizeSubMenu = new wxMenu;
    SizeSubMenu->AppendRadioItem(ID_EMULATOR_SCALE_1X, wxT("1X"));
    SizeSubMenu->AppendRadioItem(ID_EMULATOR_SCALE_2X, wxT("2X"));
    SizeSubMenu->AppendRadioItem(ID_EMULATOR_SCALE_3X, wxT("3X"));
    SizeSubMenu->AppendRadioItem(ID_EMULATOR_SCALE_4X, wxT("4X"));

    EmulatorMenu = new wxMenu;
    EmulatorMenu->Append(ID_EMULATOR_PAUSE, wxT("&Pause"));
    EmulatorMenu->Append(ID_EMULATOR_RESUME, wxT("&Resume"));
    EmulatorMenu->Append(ID_EMULATOR_STOP, wxT("&Stop"));
    EmulatorMenu->AppendSeparator();
    EmulatorMenu->AppendSubMenu(SizeSubMenu, wxT("&Size"));
    EmulatorMenu->AppendCheckItem(ID_EMULATOR_LIMIT, wxT("&Limit To 60 FPS"));
    EmulatorMenu->FindItem(ID_EMULATOR_LIMIT)->Check();
    EmulatorMenu->AppendCheckItem(ID_EMULATOR_NTSC_DECODE, wxT("&Enable NTSC Decoding"));
    EmulatorMenu->AppendSeparator();
    EmulatorMenu->Append(ID_EMULATOR_PPU_DEBUG, wxT("&PPU Debugger"));

    SettingsMenu = new wxMenu;
    SettingsMenu->AppendCheckItem(ID_CPU_LOG, wxT("&Enable CPU Log"));
    SettingsMenu->Append(ID_SETTINGS_AUDIO, wxT("&Audio Settings"));
    SettingsMenu->AppendSeparator();
    SettingsMenu->Append(ID_SETTINGS, wxT("&All Settings"));

    AboutMenu = new wxMenu;
    AboutMenu->Append(wxID_ANY, wxT("&About"));

    MenuBar = new wxMenuBar;
    MenuBar->Append(FileMenu, wxT("&File"));
    MenuBar->Append(EmulatorMenu, wxT("&Emulator"));
    MenuBar->Append(SettingsMenu, wxT("&Settings"));
    MenuBar->Append(AboutMenu, wxT("&About"));

    SetMenuBar(MenuBar);

    Bind(wxEVT_LIST_ITEM_ACTIVATED, wxListEventHandler(MainWindow::OnROMDoubleClick), this, wxID_ANY);

    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnQuit), this, wxID_EXIT);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::ToggleCPULog), this, ID_CPU_LOG);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnSettings), this, ID_SETTINGS);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OpenAudioSettings), this, ID_SETTINGS_AUDIO);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnOpenROM), this, ID_OPEN_ROM);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnEmulatorResume), this, ID_EMULATOR_RESUME);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnEmulatorStop), this, ID_EMULATOR_STOP);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnEmulatorPause), this, ID_EMULATOR_PAUSE);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnEmulatorScale), this, ID_EMULATOR_SCALE_1X);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnEmulatorScale), this, ID_EMULATOR_SCALE_2X);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnEmulatorScale), this, ID_EMULATOR_SCALE_3X);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnEmulatorScale), this, ID_EMULATOR_SCALE_4X);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnPPUDebug), this, ID_EMULATOR_PPU_DEBUG);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::ToggleFrameLimit), this, ID_EMULATOR_LIMIT);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::ToggleNtscDecoding), this, ID_EMULATOR_NTSC_DECODE);

    Bind(EVT_NES_UNEXPECTED_SHUTDOWN, wxThreadEventHandler(MainWindow::OnUnexpectedShutdown), this, wxID_ANY);
#ifdef __linux
    Bind(EVT_NES_UPDATE_FRAME, wxThreadEventHandler(MainWindow::OnUpdateFrame), this, wxID_ANY);
#endif

    Bind(wxEVT_SIZING, wxSizeEventHandler(MainWindow::OnSize), this, wxID_ANY);

    Bind(EVT_AUDIO_WINDOW_CLOSED, wxCommandEventHandler(MainWindow::OnAudioSettingsClosed), this);

    // On Linux we need a panel to intercept keyboard events for some reason
#ifdef __linux
    Panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS);
    Panel->Bind(wxEVT_KEY_DOWN, &MainWindow::OnKeyDown, this);
    Panel->Bind(wxEVT_KEY_UP, &MainWindow::OnKeyUp, this);
#elif _WIN32
    Bind(wxEVT_KEY_DOWN, &MainWindow::OnKeyDown, this);
    Bind(wxEVT_KEY_UP, &MainWindow::OnKeyUp, this);
#endif

    RomList = new GameList(this);
    RomList->PopulateList();

    VerticalBox = new wxBoxSizer(wxVERTICAL);
    VerticalBox->Add(RomList, 1, wxEXPAND | wxALL);
    SetSizer(VerticalBox);

    SetMinClientSize(wxSize(256, 240));
    Centre();
}

MainWindow::~MainWindow()
{
    StopEmulator(false);
    PPUDebugClose();
}
