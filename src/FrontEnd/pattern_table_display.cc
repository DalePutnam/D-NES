#include "wx/image.h"
#include "wx/bitmap.h"
#include "wx/dcclient.h"
#include "pattern_table_display.h"

void PatternTableDisplay::OnClicked(wxMouseEvent& WXUNUSED(event))
{
    currentPallete = (currentPallete + 1) % 8;
}

PatternTableDisplay::PatternTableDisplay(wxWindow* parent) :
    wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(256, 128)),
    currentPallete(0)
{
    SetBackgroundColour(*wxBLACK);
    Bind(wxEVT_LEFT_DOWN, wxMouseEventHandler(PatternTableDisplay::OnClicked), this, wxID_ANY);
}

void PatternTableDisplay::UpdateTable1(unsigned char* data)
{
    wxImage image(128, 128, data, true);
    wxBitmap bitmap(image, 24);

    wxClientDC dc(this);
    dc.DrawBitmap(bitmap, 0, 0);
}

void PatternTableDisplay::UpdateTable2(unsigned char* data)
{
    wxImage image(128, 128, data, true);
    wxBitmap bitmap(image, 24);

    wxClientDC dc(this);
    dc.DrawBitmap(bitmap, 128, 0);
}

void PatternTableDisplay::Clear()
{
    wxImage image(256, 128);
    wxBitmap bitmap(image, 24);

    wxClientDC dc(this);
    dc.DrawBitmap(bitmap, 0, 0);
}

int PatternTableDisplay::GetCurrentPalette()
{
    return currentPallete;
}