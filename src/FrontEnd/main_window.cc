#include <sstream>
#include <iostream>

#include "wx/msgdlg.h"
#include "wx/filedlg.h"
#include "wx/dcclient.h"
#include "wx/dcmemory.h"
#include "boost/filesystem.hpp"

#include "nes.h"
#include "main_window.h"
#include "settings_window.h"
#include "utilities/app_settings.h"

wxDEFINE_EVENT(wxEVT_NES_UNEXPECTED_SHUTDOWN, wxThreadEvent);

void MainWindow::OnEmulatorFrameComplete(uint8_t* frameBuffer)
{
	using namespace std::chrono;

    wxImage image(256, 240, frameBuffer, true);
    wxBitmap bitmap(image, 24);
    wxMemoryDC mdc(bitmap);
    wxClientDC cdc(this);
    cdc.StretchBlit(0, 0, GetVirtualSize().GetWidth(), GetVirtualSize().GetHeight(), &mdc, 0, 0, 256, 240);

    if (ppuDebugWindow != nullptr)
    {
        std::lock_guard<std::mutex> lock(PpuDebugMutex);

        if (ppuDebugWindow != nullptr)
        {
            for (int i = 0; i < 64; ++i)
            {
                if (i < 2)
                {
                    uint8_t patternTable[256 * 128 * 3];
                    nes->GetPatternTable(i, ppuDebugWindow->GetCurrentPalette(), patternTable);
                    ppuDebugWindow->UpdatePatternTable(i, patternTable);
                }

                if (i < 4)
                {
                    uint8_t nameTable[256 * 240 * 3];
                    nes->GetNameTable(i, nameTable);
                    ppuDebugWindow->UpdateNameTable(i, nameTable);
                }

                if (i < 8)
                {
                    uint8_t palette[64 * 16 * 3];
                    nes->GetPalette(i, palette);
                    ppuDebugWindow->UpdatePalette(i, palette);
                }

                uint8_t sprite[8 * 8 * 3];
                nes->GetPrimarySprite(i, sprite);
                ppuDebugWindow->UpdatePrimarySprite(i, sprite);
            }
        }
    }

    fpsCounter++;

    steady_clock::time_point now = steady_clock::now();
    microseconds time_span = duration_cast<microseconds>(now - intervalStart);
    if (time_span.count() >= 1000000)
    {
        currentFPS = fpsCounter;
        fpsCounter = 0;
        intervalStart = steady_clock::now();

        std::ostringstream oss;
        oss << currentFPS << " FPS - " << nes->GetGameName();
        SetTitle(oss.str());
    }
}

void MainWindow::OnEmulatorError(std::string err)
{
    wxThreadEvent evt(wxEVT_NES_UNEXPECTED_SHUTDOWN);
    evt.SetString(err);

    wxQueueEvent(this, evt.Clone());
    SetFocus();
}

void MainWindow::StartEmulator(std::string& filename)
{
    if (nes == nullptr)
    {
        NesParams params;
        params.RomPath = filename;
        params.CpuLogEnabled = settings->FindItem(ID_CPU_LOG)->IsChecked();
        params.FrameLimitEnabled = emulator->FindItem(ID_EMULATOR_LIMIT)->IsChecked();
        params.SoundMuted = emulator->FindItem(ID_EMULATOR_MUTE)->IsChecked();
        params.FiltersEnabled = emulator->FindItem(ID_EMULATOR_FILTER)->IsChecked();

        try
        {
            nes = new NES(params);
            nes->BindFrameCompleteCallback(&MainWindow::OnEmulatorFrameComplete, this);
            nes->BindErrorCallback(&MainWindow::OnEmulatorError, this);
        }
        catch (std::exception &e)
        {
            wxMessageDialog message(nullptr, e.what(), "ERROR", wxOK | wxICON_ERROR);
            message.ShowModal();

            delete nes;
            nes = nullptr;
            return;
        }

        fpsCounter = 0;
        intervalStart = std::chrono::steady_clock::now();
        nes->Start();

        romList = 0;
        vbox->Clear(true);
        SetTitle(nes->GetGameName());
        SetClientSize(gameSize);

#ifndef _WINDOWS
        panel->SetFocus();
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
    if (nes != nullptr)
    {
        nes->Stop();
        delete nes;
        nes = nullptr;

        if (showRomList)
        {
            romList = new GameList(this);
            vbox->Add(romList, 1, wxEXPAND | wxALL);
            romList->PopulateList();
            SetSize(wxSize(600, 460));
            SetTitle("D-NES");
        }

        if (ppuDebugWindow != nullptr)
        {
            ppuDebugWindow->ClearAll();
        }
    }
}

void MainWindow::ToggleCPULog(wxCommandEvent& WXUNUSED(event))
{
    if (nes != nullptr)
    {
        if (settings->FindItem(ID_CPU_LOG)->IsChecked())
        {
            nes->EnableCPULog();
            nes->ApuSetMuted(true);
        }
        else
        {
            nes->DisableCPULog();

            if (!emulator->FindItem(ID_EMULATOR_MUTE)->IsChecked())
            {
                nes->ApuSetMuted(false);
            }
        }
    }
}

void MainWindow::ToggleFrameLimit(wxCommandEvent& event)
{
    if (nes != nullptr)
    {
        bool enabled = emulator->FindItem(ID_EMULATOR_LIMIT)->IsChecked();
        nes->PpuSetFrameLimitEnabled(enabled);
    }
}

void MainWindow::ToggleMute(wxCommandEvent& event)
{
    if (nes != nullptr)
    {
        bool muted = emulator->FindItem(ID_EMULATOR_MUTE)->IsChecked();
        nes->ApuSetMuted(muted);
    }
}

void MainWindow::ToggleFilters(wxCommandEvent& event)
{
    if (nes != nullptr)
    {
        bool enabled = emulator->FindItem(ID_EMULATOR_FILTER)->IsChecked();
        nes->ApuSetFiltersEnabled(enabled);
    }
}

void MainWindow::ToggleNtscDecoding(wxCommandEvent& event)
{
    if (nes != nullptr) 
    {
        bool enabled = emulator->FindItem(ID_EMULATOR_NTSC_DECODE)->IsChecked();
        nes->PpuSetNtscDecoderEnabled(enabled);
    }
}

void MainWindow::OnSettings(wxCommandEvent& WXUNUSED(event))
{
    SettingsWindow settings;
    if (settings.ShowModal() == wxID_OK)
    {
        settings.SaveSettings();
    }

    if (romList)
    {
        romList->PopulateList();
    }
}

void MainWindow::OnROMDoubleClick(wxListEvent& event)
{
    AppSettings* settings = AppSettings::getInstance();
    wxString filename = romList->GetItemText(event.GetIndex(), 0);

    std::string romName = settings->get<std::string>("frontend.rompath") + "/" + filename.ToStdString();
    StartEmulator(romName);
}

void MainWindow::OnOpenROM(wxCommandEvent& WXUNUSED(event))
{
    AppSettings* settings = AppSettings::getInstance();
    wxFileDialog dialog(NULL, "Open ROM", settings->get<std::string>("frontend.rompath"), wxEmptyString, "NES Files (*.nes)|*.nes|All Files (*.*)|*.*");

    if (dialog.ShowModal() == wxID_OK)
    {
        wxString filename = dialog.GetPath();

        std::string romName = filename.ToStdString();
        StartEmulator(romName);
    }
}

void MainWindow::OnEmulatorResume(wxCommandEvent& WXUNUSED(event))
{
    if (nes != nullptr)
    {
        nes->Resume();
    }
}

void MainWindow::OnEmulatorStop(wxCommandEvent& WXUNUSED(event))
{
    StopEmulator();
}

void MainWindow::OnEmulatorPause(wxCommandEvent& WXUNUSED(event))
{
    if (nes != nullptr)
    {
        nes->Pause();
    }
}

void MainWindow::OnEmulatorScale(wxCommandEvent& WXUNUSED(event))
{
    if (size->IsChecked(ID_EMULATOR_SCALE_1X))
    {
        gameSize = wxSize(256, 240);
    }
    else if (size->IsChecked(ID_EMULATOR_SCALE_2X))
    {
        gameSize = wxSize(512, 480);
    }
    else if (size->IsChecked(ID_EMULATOR_SCALE_3X))
    {
        gameSize = wxSize(768, 720);
    }
    else if (size->IsChecked(ID_EMULATOR_SCALE_4X))
    {
        gameSize = wxSize(1024, 960);
    }

    if (nes != nullptr)
    {
        SetClientSize(gameSize);
    }
}


void MainWindow::OnPPUDebug(wxCommandEvent& WXUNUSED(event))
{
    if (ppuDebugWindow == nullptr)
    {
        ppuDebugWindow = new PPUDebugWindow(this);
        ppuDebugWindow->Show();
    }
}

void MainWindow::PPUDebugClose()
{
    if (ppuDebugWindow != nullptr)
    {
        std::lock_guard<std::mutex> lock(PpuDebugMutex);

        ppuDebugWindow->Destroy();
        ppuDebugWindow = nullptr;
    }
}

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
    if (nes != nullptr)
    {
        //event.Skip();
    }
}

void MainWindow::OnKeyDown(wxKeyEvent& event)
{
    if (nes != nullptr)
    {
        unsigned char currentState = nes->GetControllerOneState();

        switch (event.GetKeyCode())
        {
        case 'Z':
            nes->SetControllerOneState(currentState | 0x1);
            break;
        case 'X':
            nes->SetControllerOneState(currentState | 0x2);
            break;
        case WXK_CONTROL:
            nes->SetControllerOneState(currentState | 0x4);
            break;
        case WXK_RETURN:
            nes->SetControllerOneState(currentState | 0x8);
            break;
        case WXK_UP:
            nes->SetControllerOneState(currentState | 0x10);
            break;
        case WXK_DOWN:
            nes->SetControllerOneState(currentState | 0x20);
            break;
        case WXK_LEFT:
            nes->SetControllerOneState(currentState | 0x40);
            break;
        case WXK_RIGHT:
            nes->SetControllerOneState(currentState | 0x80);
            break;
        default:
            break;
        }
    }
}

void MainWindow::OnKeyUp(wxKeyEvent& event)
{
    if (nes != nullptr)
    {
        unsigned char currentState = nes->GetControllerOneState();

        switch (event.GetKeyCode())
        {
        case 'Z':
            nes->SetControllerOneState(currentState & ~0x1);
            break;
        case 'X':
            nes->SetControllerOneState(currentState & ~0x2);
            break;
        case WXK_CONTROL:
            nes->SetControllerOneState(currentState & ~0x4);
            break;
        case WXK_RETURN:
            nes->SetControllerOneState(currentState & ~0x8);
            break;
        case WXK_UP:
            nes->SetControllerOneState(currentState & ~0x10);
            break;
        case WXK_DOWN:
            nes->SetControllerOneState(currentState & ~0x20);
            break;
        case WXK_LEFT:
            nes->SetControllerOneState(currentState & ~0x40);
            break;
        case WXK_RIGHT:
            nes->SetControllerOneState(currentState & ~0x80);
            break;
        default:
            break;
        }
    }
}

MainWindow::MainWindow() :
    wxFrame(NULL, wxID_ANY, "D-NES", wxDefaultPosition, wxSize(600, 460)),
    nes(nullptr),
    fpsCounter(0),
    currentFPS(0),
	intervalStart(std::chrono::steady_clock::now()),
    ppuDebugWindow(nullptr),
    gameSize(wxSize(256, 240))
{
    file = new wxMenu;
    file->Append(ID_OPEN_ROM, wxT("&Open ROM"));
    file->AppendSeparator();
    file->Append(wxID_EXIT, wxT("&Quit"));

    size = new wxMenu;
    size->AppendRadioItem(ID_EMULATOR_SCALE_1X, wxT("1X"));
    size->AppendRadioItem(ID_EMULATOR_SCALE_2X, wxT("2X"));
    size->AppendRadioItem(ID_EMULATOR_SCALE_3X, wxT("3X"));
    size->AppendRadioItem(ID_EMULATOR_SCALE_4X, wxT("4X"));

    emulator = new wxMenu;
    emulator->Append(ID_EMULATOR_PAUSE, wxT("&Pause"));
    emulator->Append(ID_EMULATOR_RESUME, wxT("&Resume"));
    emulator->Append(ID_EMULATOR_STOP, wxT("&Stop"));
    emulator->AppendSeparator();
    emulator->AppendSubMenu(size, wxT("&Size"));
    emulator->AppendCheckItem(ID_EMULATOR_LIMIT, wxT("&Limit To 60 FPS"));
    emulator->FindItem(ID_EMULATOR_LIMIT)->Check();
    emulator->AppendCheckItem(ID_EMULATOR_NTSC_DECODE, wxT("&Enable NTSC Decoding"));
    emulator->AppendCheckItem(ID_EMULATOR_MUTE, wxT("&Mute"));
    emulator->AppendCheckItem(ID_EMULATOR_FILTER, wxT("&Filters Enabled"));
    emulator->AppendSeparator();
    emulator->Append(ID_EMULATOR_PPU_DEBUG, wxT("&PPU Debugger"));

    settings = new wxMenu;
    settings->AppendCheckItem(ID_CPU_LOG, wxT("&Enable CPU Log"));
    settings->AppendSeparator();
    settings->Append(ID_SETTINGS, wxT("&All Settings"));

    about = new wxMenu;
    about->Append(wxID_ANY, wxT("&About"));

    menuBar = new wxMenuBar;
    menuBar->Append(file, wxT("&File"));
    menuBar->Append(emulator, wxT("&Emulator"));
    menuBar->Append(settings, wxT("&Settings"));
    menuBar->Append(about, wxT("&About"));

    SetMenuBar(menuBar);

    Bind(wxEVT_LIST_ITEM_ACTIVATED, wxListEventHandler(MainWindow::OnROMDoubleClick), this, wxID_ANY);

    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnQuit), this, wxID_EXIT);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::ToggleCPULog), this, ID_CPU_LOG);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnSettings), this, ID_SETTINGS);
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
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::ToggleMute), this, ID_EMULATOR_MUTE);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::ToggleFilters), this, ID_EMULATOR_FILTER);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::ToggleNtscDecoding), this, ID_EMULATOR_NTSC_DECODE);

    Bind(wxEVT_NES_UNEXPECTED_SHUTDOWN, wxThreadEventHandler(MainWindow::OnUnexpectedShutdown), this, wxID_ANY);

    Bind(wxEVT_SIZING, wxSizeEventHandler(MainWindow::OnSize), this, wxID_ANY);

#ifdef _WINDOWS
    Bind(wxEVT_KEY_DOWN, &MainWindow::OnKeyDown, this);
    Bind(wxEVT_KEY_UP, &MainWindow::OnKeyUp, this);
#else
    panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS);
    panel->Bind(wxEVT_KEY_DOWN, &MainWindow::OnKeyDown, this);
    panel->Bind(wxEVT_KEY_UP, &MainWindow::OnKeyUp, this);
#endif

    romList = new GameList(this);
    romList->PopulateList();

    vbox = new wxBoxSizer(wxVERTICAL);
    vbox->Add(romList, 1, wxEXPAND | wxALL);
    SetSizer(vbox);

    SetMinClientSize(wxSize(256, 240));
    Centre();
}

MainWindow::~MainWindow()
{
    StopEmulator(false);
    PPUDebugClose();
}
