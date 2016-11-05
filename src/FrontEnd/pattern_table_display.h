#pragma once

#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/window.h>
#include <wx/statbox.h>

class PatternTableDisplay : public wxPanel
{
    int currentPalette;

    wxPanel* table1;
    wxPanel* table2;

    void OnClicked(wxMouseEvent& event);

public:
    PatternTableDisplay(wxWindow* parent);
    void UpdateTable1(unsigned char* data);
    void UpdateTable2(unsigned char* data);
    void Clear();
    int GetCurrentPalette();
};
