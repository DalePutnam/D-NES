#pragma once

#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/window.h>
#include <wx/statbox.h>

class NES;

class PatternTableDisplay : public wxPanel
{
public:
    PatternTableDisplay(wxWindow* parent, NES* nes = nullptr);

    void Update();
    void Clear();
    void SetNes(NES* nes);

    int GetCurrentPalette();

private:
    void OnClicked(wxMouseEvent& event);

    NES* Nes;

    int CurrentPalette;
    wxPanel* Table1;
    wxPanel* Table2;
};
