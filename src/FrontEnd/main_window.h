/* The main window for the application */

#ifndef MAIN_WINDOW_H_
#define MAIN_WINDOW_H_

#include "wx/menu.h"
#include "wx/image.h"
#include "wx/panel.h"
#include "wx/frame.h"
#include "wx/sizer.h"
#include "wx/statusbr.h"
#include "wx/treelist.h"
#include "wx/listctrl.h"

#include "game_list.h"
#include "nes_thread.h"
#include "game_window.h"
#include "ppu_debug_window.h"

class MainWindow : public wxFrame
{
    NESThread* nesThread;
    PPUDebugWindow* ppuDebugWindow;

    wxMenuBar* menuBar;
    wxMenu* file;
    wxMenu* scale;
    wxMenu* emulator;
    wxMenu* settings;
    wxMenu* about;

    wxBoxSizer* vbox;
    GameList* romList;

    wxImage frame;
    wxSize gameSize;

    void StartEmulator(std::string& filename);
    void UpdateImage(unsigned char* data);

    void ToggleCPULog(wxCommandEvent& event);
    void OnSettings(wxCommandEvent& event);
    void OnROMDoubleClick(wxListEvent& event);
    void OnOpenROM(wxCommandEvent& event);
    void OnThreadUpdate(wxThreadEvent& event);
    void OnEmulatorResume(wxCommandEvent& event);
    void OnEmulatorStop(wxCommandEvent& event);
    void OnEmulatorPause(wxCommandEvent& event);
    void OnEmulatorScale1X(wxCommandEvent& event);
    void OnEmulatorScale2X(wxCommandEvent& event);
    void OnEmulatorScale3X(wxCommandEvent& event);
    void OnEmulatorScale4X(wxCommandEvent& event);
    void OnPPUDebug(wxCommandEvent& event);
    void OnUnexpectedShutdown(wxThreadEvent& event);
    void OnFPSUpdate(wxThreadEvent& event);
    void OnQuit(wxCommandEvent& event);
    void OnSize(wxSizeEvent& event);

    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);

public:
    MainWindow();
    ~MainWindow();

    void StopEmulator(bool showRomList = true);
    void PPUDebugClose();

    NESThread* GetNESThread();
};

const int ID_OPEN_ROM = 100;
const int ID_EMULATOR_RESUME = 101;
const int ID_EMULATOR_PAUSE = 102;
const int ID_EMULATOR_STOP = 103;
const int ID_EMUALTOR_PPU_DEBUG = 104;
const int ID_SETTINGS = 105;
const int ID_CPU_LOG = 106;
const int ID_EMULATOR_SCALE_1X = 107;
const int ID_EMULATOR_SCALE_2X = 108;
const int ID_EMULATOR_SCALE_3X = 109;
const int ID_EMULATOR_SCALE_4X = 110;

#endif