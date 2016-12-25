#pragma once

#include <mutex>
#include <wx/frame.h>
#include <wx/panel.h>

#include "pattern_table_display.h"

class MainWindow;

class PPUDebugWindow : public wxFrame
{
public:
    explicit PPUDebugWindow(MainWindow* mainWindow);

    void UpdateNameTable(int tableID, unsigned char* data);
    void UpdatePatternTable(int tableID, unsigned char* data);
    void UpdatePalette(int tableID, unsigned char* data);
    void UpdatePrimarySprite(int sprite, unsigned char* data);
    void ClearAll();
    int GetCurrentPalette();

private:
    MainWindow* ParentWindow;

    int PaletteIndex;

    PatternTableDisplay *PatternDisplay;

    wxPanel* NameTable[4];
    wxPanel* PatternTable[2];
    wxPanel* Palette[8];
    wxPanel* PrimarySprite[64];

    void OnQuit(wxCommandEvent& event);
    void OnPatternTableClicked(wxMouseEvent& event);
};
