#pragma once

#include <mutex>
#include <condition_variable>
#include <wx/frame.h>
#include <wx/panel.h>

#include "pattern_table_display.h"

wxDECLARE_EVENT(EVT_PPU_VIEWER_CLOSED, wxCommandEvent);

class NES;
class MainWindow;

class PPUViewerWindow : public wxFrame
{
public:
    explicit PPUViewerWindow(MainWindow* mainWindow, NES* nes = nullptr);

    void Update();
    void ClearAll();
    void SetNes(NES* nes);

    int GetCurrentPalette();

private:
    MainWindow* ParentWindow;
    PatternTableDisplay *PatternDisplay;

    NES* Nes;

    wxPanel* NameTable[4];
    wxPanel* Palette[8];
    wxPanel* Sprite[64];

    void OnQuit(wxCommandEvent& event);
};
