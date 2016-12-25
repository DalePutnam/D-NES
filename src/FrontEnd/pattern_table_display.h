#pragma once

#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/window.h>
#include <wx/statbox.h>

class PatternTableDisplay : public wxPanel
{
public:
    PatternTableDisplay(wxWindow* parent);
    void UpdateTable1(unsigned char* data);
    void UpdateTable2(unsigned char* data);
    void Clear();
    int GetCurrentPalette();

private:
    void OnClicked(wxMouseEvent& event);

    int CurrentPalette;
    wxPanel* Table1;
    wxPanel* Table2;
};
