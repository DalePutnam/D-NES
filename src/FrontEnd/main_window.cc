#include <sstream>
#include <iostream>

#include "wx/msgdlg.h"
#include "wx/dcclient.h"
#include "wx/filedlg.h"
#include "boost/filesystem.hpp"

#include "main_window.h"
#include "settings_window.h"
#include "utilities/app_settings.h"

void MainWindow::StartEmulator(std::string& filename)
{
    try
    {
        if (!nesThread)
        {
            nesThread = new NESThread(this, filename, settings->FindItem(ID_CPU_LOG)->IsChecked());

            romList = 0;
            vbox->Clear(true);
            SetTitle(nesThread->GetGameName());
            SetClientSize(gameSize);

            if (nesThread->Run() != wxTHREAD_NO_ERROR)
            {
                delete nesThread;
                nesThread = 0;
             
                vbox->Clear(true);
                romList = new GameList(this);
                vbox->Add(romList, 1, wxEXPAND | wxALL);
                romList->PopulateList();
                SetSize(wxSize(600, 460));
                SetTitle("D-NES");

                wxMessageDialog message(NULL, "Failed to Start Emulator", "ERROR", wxOK | wxICON_ERROR);
                message.ShowModal();
            }
        }
        else
        {
            wxMessageDialog message(NULL, "Emulator Instance Already Running", "Cannot Complete Action", wxOK);
            message.ShowModal();
        }
    }
    catch (std::string& err)
    {
        wxMessageDialog message(NULL, err, "ERROR", wxOK | wxICON_ERROR);
        message.ShowModal();
    }
}

void MainWindow::StopEmulator(bool showRomList)
{
    if (nesThread)
    {
        nesThread->Stop();
        nesThread->Wait();
        delete nesThread;
        nesThread = 0;

        if (showRomList)
        {
            romList = new GameList(this);
            vbox->Add(romList, 1, wxEXPAND | wxALL);
            romList->PopulateList();
            SetSize(wxSize(600, 460));
            SetTitle("D-NES");
        }

        if (ppuDebugWindow)
        {
            ppuDebugWindow->ClearAll();
        }
    }
}

void MainWindow::UpdateImage(unsigned char* data)
{
    wxClientDC dc(this);
    frame.Create(256, 240, data, true);

    wxImage image = frame;
    image.Rescale(GetVirtualSize().GetX(), GetVirtualSize().GetY());

    wxBitmap bitmap(image, 24);

    dc.DrawBitmap(bitmap, 0, 0);
}

void MainWindow::ToggleCPULog(wxCommandEvent& WXUNUSED(event))
{
    if (nesThread)
    {
        if (settings->FindItem(ID_CPU_LOG)->IsChecked())
        {
            nesThread->EnableCPULog();
        }
        else
        {
            nesThread->DisableCPULog();
        }
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

void MainWindow::OnThreadUpdate(wxThreadEvent& WXUNUSED(event))
{
    if (nesThread)
    {
        UpdateImage(nesThread->GetFrame());

        if (ppuDebugWindow)
        {
            for (int i = 0; i < 64; ++i)
            {
                if (i < 2) ppuDebugWindow->UpdatePatternTable(i, nesThread->GetPatternTable(i, ppuDebugWindow->GetCurrentPalette()));
                if (i < 4) ppuDebugWindow->UpdateNameTable(i, nesThread->GetNameTable(i));

                if (i < 8)
                {
                    ppuDebugWindow->UpdatePalette(i, nesThread->GetPalette(i));
                }

                ppuDebugWindow->UpdatePrimarySprite(i, nesThread->GetPrimarySprite(i));
            }
        }

        nesThread->UnlockFrame();
    }
}

void MainWindow::OnEmulatorResume(wxCommandEvent& WXUNUSED(event))
{
    if (nesThread)
    {
        nesThread->EmulatorResume();
    }
}

void MainWindow::OnEmulatorStop(wxCommandEvent& WXUNUSED(event))
{
    StopEmulator();
}

void MainWindow::OnEmulatorPause(wxCommandEvent& WXUNUSED(event))
{
    if (nesThread)
    {
        nesThread->EmulatorPause();
    }
}

void MainWindow::OnEmulatorScale1X(wxCommandEvent& WXUNUSED(event))
{
    gameSize = wxSize(256, 240);
    SetClientSize(gameSize);

    if (nesThread)
    {
        wxClientDC dc(this);

        wxImage image = frame;
        image.Rescale(GetVirtualSize().GetX(), GetVirtualSize().GetY());

        wxBitmap bitmap(image, 24);
        dc.DrawBitmap(bitmap, 0, 0);
    }
}

void MainWindow::OnEmulatorScale2X(wxCommandEvent& WXUNUSED(event))
{
    gameSize = wxSize(512, 480);
    SetClientSize(gameSize);

    if (nesThread)
    {
        wxClientDC dc(this);

        wxImage image = frame;
        image.Rescale(GetVirtualSize().GetX(), GetVirtualSize().GetY());

        wxBitmap bitmap(image, 24);
        dc.DrawBitmap(bitmap, 0, 0);
    }
}

void MainWindow::OnEmulatorScale3X(wxCommandEvent& WXUNUSED(event))
{
    gameSize = wxSize(768, 720);
    SetClientSize(gameSize);

    if (nesThread)
    {
        wxClientDC dc(this);

        wxImage image = frame;
        image.Rescale(GetVirtualSize().GetX(), GetVirtualSize().GetY());

        wxBitmap bitmap(image, 24);
        dc.DrawBitmap(bitmap, 0, 0);
    }
}

void MainWindow::OnEmulatorScale4X(wxCommandEvent& WXUNUSED(event))
{
    gameSize = wxSize(1024, 960);
    SetClientSize(gameSize);

    if (nesThread)
    {
        wxClientDC dc(this);

        wxImage image = frame;
        image.Rescale(GetVirtualSize().GetX(), GetVirtualSize().GetY());

        wxBitmap bitmap(image, 24);
        dc.DrawBitmap(bitmap, 0, 0);
    }
}

void MainWindow::OnPPUDebug(wxCommandEvent& WXUNUSED(event))
{
    if (!ppuDebugWindow)
    {
        ppuDebugWindow = new PPUDebugWindow(this);
        ppuDebugWindow->Show();
    }
}

void MainWindow::PPUDebugClose()
{
    if (ppuDebugWindow)
    {
        ppuDebugWindow->Destroy();
        ppuDebugWindow = 0;
    }
}

void MainWindow::OnUnexpectedShutdown(wxThreadEvent& WXUNUSED(event))
{
    wxMessageDialog message(NULL, "Emulator Crashed", "ERROR", wxOK | wxICON_ERROR);
    message.ShowModal();
    StopEmulator();
}

void MainWindow::OnFPSUpdate(wxThreadEvent& WXUNUSED(event))
{
    if (nesThread)
    {
        std::ostringstream oss;
        oss << nesThread->GetCurrentFPS() << " FPS - " << nesThread->GetGameName();
        SetTitle(oss.str());
    }
}

void MainWindow::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    StopEmulator(false);
    Close(true);
}

void MainWindow::OnSize(wxSizeEvent& WXUNUSED(event))
{
    if (nesThread)
    {
        wxClientDC dc(this);

        wxImage image = frame;
        image.Rescale(GetVirtualSize().GetX(), GetVirtualSize().GetY());

        wxBitmap bitmap(image, 24);
        dc.DrawBitmap(bitmap, 0, 0);
    }
}

void MainWindow::OnKeyDown(wxKeyEvent& event)
{
    if (nesThread)
    {
        unsigned char currentState = nesThread->GetControllerOneState();

        switch (event.GetKeyCode())
        {
        case 'Z':
            nesThread->SetControllerOneState(currentState | 0x1);
            break;
        case 'X':
            nesThread->SetControllerOneState(currentState | 0x2);
            break;
        case WXK_CONTROL:
            nesThread->SetControllerOneState(currentState | 0x4);
            break;
        case WXK_RETURN:
            nesThread->SetControllerOneState(currentState | 0x8);
            break;
        case WXK_UP:
            nesThread->SetControllerOneState(currentState | 0x10);
            break;
        case WXK_DOWN:
            nesThread->SetControllerOneState(currentState | 0x20);
            break;
        case WXK_LEFT:
            nesThread->SetControllerOneState(currentState | 0x40);
            break;
        case WXK_RIGHT:
            nesThread->SetControllerOneState(currentState | 0x80);
            break;
        default:
            break;
        }
    }
}

void MainWindow::OnKeyUp(wxKeyEvent& event)
{
    if (nesThread)
    {
        unsigned char currentState = nesThread->GetControllerOneState();

        switch (event.GetKeyCode())
        {
        case 'Z':
            nesThread->SetControllerOneState(currentState & ~0x1);
            break;
        case 'X':
            nesThread->SetControllerOneState(currentState & ~0x2);
            break;
        case WXK_CONTROL:
            nesThread->SetControllerOneState(currentState & ~0x4);
            break;
        case WXK_RETURN:
            nesThread->SetControllerOneState(currentState & ~0x8);
            break;
        case WXK_UP:
            nesThread->SetControllerOneState(currentState & ~0x10);
            break;
        case WXK_DOWN:
            nesThread->SetControllerOneState(currentState & ~0x20);
            break;
        case WXK_LEFT:
            nesThread->SetControllerOneState(currentState & ~0x40);
            break;
        case WXK_RIGHT:
            nesThread->SetControllerOneState(currentState & ~0x80);
            break;
        default:
            break;
        }
    }
}

NESThread* MainWindow::GetNESThread()
{
    return nesThread;
}

MainWindow::MainWindow()
    : wxFrame(NULL, wxID_ANY, "D-NES", wxDefaultPosition, wxSize(600, 460)),
    nesThread(0),
    ppuDebugWindow(0),
    gameSize(256, 240)
{
    file = new wxMenu;
    file->Append(ID_OPEN_ROM, wxT("&Open ROM"));
    file->AppendSeparator();
    file->Append(wxID_EXIT, wxT("&Quit"));

    scale = new wxMenu;
    scale->AppendRadioItem(ID_EMULATOR_SCALE_1X, wxT("1X"));
    scale->AppendRadioItem(ID_EMULATOR_SCALE_2X, wxT("2X"));
    scale->AppendRadioItem(ID_EMULATOR_SCALE_3X, wxT("3X"));
    scale->AppendRadioItem(ID_EMULATOR_SCALE_4X, wxT("4X"));

    emulator = new wxMenu;
    emulator->Append(ID_EMULATOR_PAUSE, wxT("&Pause"));
    emulator->Append(ID_EMULATOR_RESUME, wxT("&Resume"));
    emulator->Append(ID_EMULATOR_STOP, wxT("&Stop"));
    emulator->AppendSeparator();
    emulator->AppendSubMenu(scale, wxT("&Scale"));
    emulator->AppendSeparator();
    emulator->Append(ID_EMUALTOR_PPU_DEBUG, wxT("&PPU Debugger"));

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
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnEmulatorScale1X), this, ID_EMULATOR_SCALE_1X);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnEmulatorScale2X), this, ID_EMULATOR_SCALE_2X);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnEmulatorScale3X), this, ID_EMULATOR_SCALE_3X);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnEmulatorScale4X), this, ID_EMULATOR_SCALE_4X);
    Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnPPUDebug), this, ID_EMUALTOR_PPU_DEBUG);

    Bind(wxEVT_COMMAND_NESTHREAD_FRAME_UPDATE, wxThreadEventHandler(MainWindow::OnThreadUpdate), this, wxID_ANY);
    Bind(wxEVT_COMMAND_NESTHREAD_FPS_UPDATE, wxThreadEventHandler(MainWindow::OnFPSUpdate), this, wxID_ANY);
    Bind(wxEVT_COMMAND_NESTHREAD_UNEXPECTED_SHUTDOWN, wxThreadEventHandler(MainWindow::OnUnexpectedShutdown), this, wxID_ANY);

    Bind(wxEVT_SIZING, wxSizeEventHandler(MainWindow::OnSize), this, wxID_ANY);

    Bind(wxEVT_KEY_DOWN, wxKeyEventHandler(MainWindow::OnKeyDown), this, wxID_ANY);
    Bind(wxEVT_KEY_UP, wxKeyEventHandler(MainWindow::OnKeyUp), this, wxID_ANY);

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
