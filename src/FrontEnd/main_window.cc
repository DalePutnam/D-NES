#include <sstream>
#include <iostream>

#include "wx/msgdlg.h"
#include "wx/filedlg.h"
#include "boost/filesystem.hpp"

#include "main_window.h"
#include "settings_window.h"
#include "utilities/app_settings.h"

void MainWindow::PopulateROMList()
{
	namespace fs = boost::filesystem;

	AppSettings* settings = AppSettings::getInstance();
	fs::path filePath(settings->get<std::string>("frontend.rompath")); // Get path from settings

	romList->DeleteAllItems();

	if (fs::exists(filePath))
	{
		if (fs::is_directory(filePath))
		{
			for (fs::directory_iterator it(filePath); it != fs::directory_iterator(); ++it)
			{
				// If the file is a regular file and it has a .nes extension, add it to the list
				if (fs::is_regular_file(it->path())
					&& it->path().has_extension()
					&& std::string(".nes").compare(it->path().extension().string()) == 0)
				{
					wxTreeListItem item = romList->AppendItem(romList->GetRootItem(), it->path().filename().string());

					// Get file size in Kibibytes
					std::ostringstream oss;
					oss << file_size(it->path()) / 1024 << " KiB";

					romList->SetItemText(item, 1, oss.str());
				}
			}
		}
	}
}

void MainWindow::StartEmulator(std::string& filename)
{
	try
	{
		if (!nesThread)
		{
			nesThread = new NESThread(this, filename, settings->FindItem(ID_CPU_LOG)->IsChecked());
			gameWindow = new GameWindow(this, nesThread->GetGameName());
			gameWindow->Show();

			if (nesThread->Run() != wxTHREAD_NO_ERROR)
			{
				delete nesThread;
				gameWindow->Close();
				gameWindow->Destroy();
				nesThread = 0;
				gameWindow = 0;

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

void MainWindow::StopEmulator()
{
	if (nesThread)
	{
		nesThread->Stop();
		nesThread->Wait();
		delete nesThread;
		nesThread = 0;

		gameWindow->Destroy();
		gameWindow = 0;

		if (ppuDebugWindow)
		{
			ppuDebugWindow->Destroy();
			ppuDebugWindow = 0;
		}
	}
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

	PopulateROMList();
}

void MainWindow::OnROMDoubleClick(wxCommandEvent& WXUNUSED(event))
{
	AppSettings* settings = AppSettings::getInstance();
	wxString filename = romList->GetItemText(romList->GetSelection());

	std::string romName = settings->get<std::string>("frontend.rompath") + "/" + filename.ToStdString();
	StartEmulator(romName);
}

void MainWindow::OnOpenROM(wxCommandEvent& WXUNUSED(event))
{
	AppSettings* settings = AppSettings::getInstance();
	wxFileDialog dialog(NULL, "Open ROM", settings->get<std::string>("frontend.rompath"), wxEmptyString, "NES Files (*.nes)|*.nes|All Files (*.*)|*.*");

	if (dialog.ShowModal() == wxID_OK)
	{
		AppSettings* settings = AppSettings::getInstance();
		wxString filename = dialog.GetFilename();

		std::string romName = settings->get<std::string>("frontend.rompath") + "/" + filename.ToStdString();
		StartEmulator(romName);
	}
}

void MainWindow::OnThreadUpdate(wxThreadEvent& WXUNUSED(event))
{
	if (nesThread)
	{
		gameWindow->UpdateImage(nesThread->GetFrame());

		if (ppuDebugWindow)
		{
			for (int i = 0; i < 64; ++i)
			{
				if (i < 2) ppuDebugWindow->UpdatePatternTable(i, nesThread->GetPatternTable(i, ppuDebugWindow->GetCurrentPalette()));
				if (i < 4) ppuDebugWindow->UpdateNameTable(i, nesThread->GetNameTable(i));

				if (i < 8)
				{
					ppuDebugWindow->UpdatePalette(i, nesThread->GetPalette(i));
					ppuDebugWindow->UpdateSecondarySprite(i, nesThread->GetPrimarySprite(i));
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

void MainWindow::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	StopEmulator();
	Close(true);
}

MainWindow::MainWindow()
	: wxFrame(NULL, wxID_ANY, "D-NES", wxDefaultPosition, wxSize(600, 460)),
	nesThread(0),
	gameWindow(0),
	ppuDebugWindow(0)
{
	file = new wxMenu;
	file->Append(ID_OPEN_ROM, wxT("&Open ROM"));
	file->AppendSeparator();
	file->Append(wxID_EXIT, wxT("&Quit"));

	emulator = new wxMenu;
	emulator->Append(ID_EMULATOR_RESUME, wxT("&Resume"));
	emulator->Append(ID_EMULATOR_STOP, wxT("&Stop"));
	emulator->Append(ID_EMULATOR_PAUSE, wxT("&Pause"));
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
	Connect(wxID_EXIT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnQuit));
	Connect(ID_CPU_LOG, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::ToggleCPULog));
	Connect(ID_SETTINGS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnSettings));
	Connect(wxEVT_TREELIST_ITEM_ACTIVATED, wxCommandEventHandler(MainWindow::OnROMDoubleClick));
	Connect(ID_OPEN_ROM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnOpenROM));
	Connect(ID_EMULATOR_RESUME, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnEmulatorResume));
	Connect(ID_EMULATOR_STOP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnEmulatorStop));
	Connect(ID_EMULATOR_PAUSE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnEmulatorPause));
	Connect(ID_EMUALTOR_PPU_DEBUG, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainWindow::OnPPUDebug));
	Connect(wxID_ANY, wxEVT_COMMAND_NESTHREAD_UPDATE, wxThreadEventHandler(MainWindow::OnThreadUpdate));

	romList = new wxTreeListCtrl(this, wxID_ANY);
	romList->AppendColumn("File Name");
	romList->AppendColumn("File Size");

	PopulateROMList();

	vbox = new wxBoxSizer(wxVERTICAL);
	vbox->Add(romList, 1, wxEXPAND | wxALL);
	SetSizer(vbox);

	Centre();
}

MainWindow::~MainWindow()
{
	StopEmulator();
}
