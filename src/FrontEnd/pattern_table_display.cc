#include <wx/image.h>
#include <wx/bitmap.h>
#include <wx/dcclient.h>

#include "pattern_table_display.h"
#include "nes.h"

void PatternTableDisplay::OnClicked(wxMouseEvent& WXUNUSED(event))
{
    CurrentPalette = (CurrentPalette + 1) % 8;

    if (Nes != nullptr && Nes->IsPaused())
    {
        Update();
    }
}

PatternTableDisplay::PatternTableDisplay(wxWindow* parent, NES* nes)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(256, 128))
    , Nes(nes)
    , CurrentPalette(0)
{
    SetBackgroundColour(*wxBLACK);
    Bind(wxEVT_LEFT_DOWN, wxMouseEventHandler(PatternTableDisplay::OnClicked), this, wxID_ANY);
}

void PatternTableDisplay::Update()
{
    if (Nes == nullptr)
    {
        return;
    }

    uint8_t patternTable[128 * 128 * 3];

    Nes->GetPatternTable(0, CurrentPalette, patternTable);
    {
        wxImage image(128, 128, patternTable, true);
        wxBitmap bitmap(image, 24);
        wxClientDC dc(this);
        dc.DrawBitmap(bitmap, 0, 0);
    }

    Nes->GetPatternTable(1, CurrentPalette, patternTable);
    {
        wxImage image(128, 128, patternTable, true);
        wxBitmap bitmap(image, 24);
        wxClientDC dc(this);
        dc.DrawBitmap(bitmap, 128, 0);
    }
}

void PatternTableDisplay::Clear()
{
    wxImage image(256, 128);
    wxBitmap bitmap(image, 24);

    wxClientDC dc(this);
    dc.DrawBitmap(bitmap, 0, 0);
}

void PatternTableDisplay::SetNes(NES* nes)
{
    Nes = nes;
}

int PatternTableDisplay::GetCurrentPalette()
{
    return CurrentPalette;
}
