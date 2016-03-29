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

namespace
{
    void Run(MainWindow* window, NES* nes)
    {
        try
        {
            nes->Run();
        }
        catch (const std::exception& /* e */)
        {
            wxThreadEvent evt(wxEVT_NES_UNEXPECTED_SHUTDOWN);
            wxQueueEvent(window, evt.Clone());
            window->SetFocus();
        }
    }
}

void MainWindow::StartEmulator(std::string& filename)
{
    try
    {
        if (nes == nullptr)
        {
            std::function<void(uint8_t*)> FrameCallback = [this](uint8_t* inFrameBuffer)
            {
                using namespace boost::chrono;

                wxImage image(256, 240, inFrameBuffer, true);
                wxBitmap bitmap(image, 24);
                wxMemoryDC mdc(bitmap);
                wxClientDC cdc(this);
                cdc.StretchBlit(0, 0, GetVirtualSize().GetWidth(), GetVirtualSize().GetHeight(), &mdc, 0, 0, 256, 240);

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
            };

            intervalStart = boost::chrono::steady_clock::now();

            NesParams params;
            params.RomPath = filename;
            params.CpuLogEnabled = settings->FindItem(ID_CPU_LOG)->IsChecked();
            params.FrameLimitEnabled = emulator->FindItem(ID_EMULATOR_LIMIT)->IsChecked();
            params.FrameCompleteCallback = &FrameCallback;

            nes = new NES(params);
            thread = new std::thread(Run, this, nes);

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
    catch (std::string& err)
    {
        wxMessageDialog message(NULL, err, "ERROR", wxOK | wxICON_ERROR);
        message.ShowModal();
    }
}

void MainWindow::StopEmulator(bool showRomList)
{
    if (nes != nullptr)
    {
        nes->Stop();
        thread->join();

        delete nes;
        delete thread;

        nes = nullptr;
        thread = nullptr;

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
// 	wxClientDC dc(this);
// 	nesThread->DrawFrame(dc, GetVirtualSize().GetX(), GetVirtualSize().GetY());
}

void MainWindow::ToggleCPULog(wxCommandEvent& WXUNUSED(event))
{
    if (nes != nullptr)
    {
        if (settings->FindItem(ID_CPU_LOG)->IsChecked())
        {
            nes->EnableCPULog();
        }
        else
        {
            nes->DisableCPULog();
        }
    }
}

void MainWindow::ToggleFrameLimit(wxCommandEvent& event)
{
	if (nes != nullptr)
	{
		if (emulator->FindItem(ID_EMULATOR_LIMIT)->IsChecked())
		{
			nes->EnableFrameLimit();
		}
		else
		{
			nes->DisableFrameLimit();
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
//     if (nesThread)
//     {
// 		wxClientDC dc(this);
// 		nesThread->DrawFrame(dc, GetVirtualSize().GetX(), GetVirtualSize().GetY());
// 
//         if (ppuDebugWindow)
//         {
//             for (int i = 0; i < 64; ++i)
//             {
//                 if (i < 2) ppuDebugWindow->UpdatePatternTable(i, nesThread->GetPatternTable(i, ppuDebugWindow->GetCurrentPalette()));
//                 if (i < 4) ppuDebugWindow->UpdateNameTable(i, nesThread->GetNameTable(i));
// 
//                 if (i < 8)
//                 {
//                     ppuDebugWindow->UpdatePalette(i, nesThread->GetPalette(i));
//                 }
// 
//                 ppuDebugWindow->UpdatePrimarySprite(i, nesThread->GetPrimarySprite(i));
//             }
//         }
//     }
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

void MainWindow::NextPixel(uint32_t pixel)
{
    unsigned char red = static_cast<unsigned char>((pixel & 0xFF0000) >> 16);
    unsigned char green = static_cast<unsigned char>((pixel & 0x00FF00) >> 8);
    unsigned char blue = static_cast<unsigned char>(pixel & 0x0000FF);

    frameBuffer[pixelCount * 3] = red;
    frameBuffer[(pixelCount * 3) + 1] = green;
    frameBuffer[(pixelCount * 3) + 2] = blue;

    ++pixelCount;

    if (pixelCount == 256 * 240)
    {
        using namespace boost::chrono;

        wxImage image(256, 240, frameBuffer, true);
        wxBitmap bitmap(image, 24);
        wxMemoryDC mdc(bitmap);
        wxClientDC cdc(this);
        cdc.StretchBlit(0, 0, GetVirtualSize().GetWidth(), GetVirtualSize().GetHeight(), &mdc, 0, 0, 256, 240);

        pixelCount = 0;
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
}

void MainWindow::DrawFrame(uint8_t* inFrameBuffer)
{
    using namespace boost::chrono;

    wxImage image(256, 240, inFrameBuffer, true);
    wxBitmap bitmap(image, 24);
    wxMemoryDC mdc(bitmap);
    wxClientDC cdc(this);
    cdc.StretchBlit(0, 0, GetVirtualSize().GetWidth(), GetVirtualSize().GetHeight(), &mdc, 0, 0, 256, 240);

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

MainWindow::MainWindow(): 
    wxFrame(NULL, wxID_ANY, "D-NES", wxDefaultPosition, wxSize(600, 460)),
    nes(nullptr),
    thread(nullptr),
    fpsCounter(0),
    currentFPS(0),
    intervalStart(boost::chrono::steady_clock::now()),
    ppuDebugWindow(0),
    gameSize(wxSize(256, 240)),
    frameSizeDirty(false)
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

//     Bind(wxEVT_COMMAND_NESTHREAD_FRAME_UPDATE, wxThreadEventHandler(MainWindow::OnThreadUpdate), this, wxID_ANY);
//     Bind(wxEVT_COMMAND_NESTHREAD_FPS_UPDATE, wxThreadEventHandler(MainWindow::OnFPSUpdate), this, wxID_ANY);
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
