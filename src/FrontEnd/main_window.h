/* The main window for the application */

#ifndef MAIN_WINDOW_H_
#define MAIN_WINDOW_H_

#include "wx/menu.h"
#include "wx/frame.h"
#include "wx/sizer.h"
#include "wx/treelist.h"

#include "nes_thread.h"
#include "game_window.h"
#include "ppu_debug_window.h"

class MainWindow : public wxFrame
{
	NESThread* nesThread;
	GameWindow* gameWindow;
	PPUDebugWindow* ppuDebugWindow;

	wxMenuBar* menuBar;
	wxMenu* file;
	wxMenu* emulator;
	wxMenu* settings;
	wxMenu* about;

	wxBoxSizer* vbox;
	wxTreeListCtrl* romList;

	void PopulateROMList();
	void StartEmulator(std::string& filename);

	void OnSettings(wxCommandEvent& event);
	void OnROMDoubleClick(wxCommandEvent& event);
	void OnOpenROM(wxCommandEvent& event);
	void OnThreadUpdate(wxThreadEvent& event);
	void OnEmulatorResume(wxCommandEvent& event);
	void OnEmulatorStop(wxCommandEvent& event);
	void OnEmulatorPause(wxCommandEvent& event);
	void OnPPUDebug(wxCommandEvent& event);
	//void OnThreadComplete(wxThreadEvent& event);
	void OnQuit(wxCommandEvent& event);

public:
	MainWindow();
	~MainWindow();
	void StopEmulator();
	void PPUDebugClose();
};

const int ID_OPEN_ROM = 100;
const int ID_EMULATOR_RESUME = 101;
const int ID_EMULATOR_PAUSE = 102;
const int ID_EMULATOR_STOP = 103;
const int ID_EMUALTOR_PPU_DEBUG = 104;
const int ID_SETTINGS = 105;

#endif