#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/statbox.h>
#include <wx/image.h>
#include <wx/bitmap.h>
#include <wx/dcclient.h>

#include "main_window.h"
#include "ppu_debug_window.h"

void PPUDebugWindow::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    mainWindow->PPUDebugClose();
}

void PPUDebugWindow::OnPatternTableClicked(wxMouseEvent& WXUNUSED(event))
{
    paletteIndex = (paletteIndex + 1) % 8;
}

PPUDebugWindow::PPUDebugWindow(MainWindow* mainWindow)
    : wxFrame(mainWindow, wxID_ANY, "PPU Debug", wxDefaultPosition, wxDefaultSize, (wxDEFAULT_FRAME_STYLE | wxFRAME_NO_TASKBAR) & ~wxRESIZE_BORDER & ~wxMAXIMIZE_BOX)
    , mainWindow(mainWindow)
    , paletteIndex(0)
{
    patternTableDisplay = new PatternTableDisplay(this);

    wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* hbox0 = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* vbox0 = new wxBoxSizer(wxVERTICAL);
    wxGridSizer* grid0 = new wxGridSizer(2, 2, 0, 0);
    wxGridSizer* grid1 = new wxGridSizer(2, 4, 0, 0);
    wxGridSizer* grid2 = new wxGridSizer(4, 16, 8, 8);

    wxStaticBoxSizer* sbox0 = new wxStaticBoxSizer(wxVERTICAL, this, "Name Tables");
    wxStaticBoxSizer* sbox1 = new wxStaticBoxSizer(wxHORIZONTAL, this, "Pattern Tables");
    wxStaticBoxSizer* sbox2 = new wxStaticBoxSizer(wxVERTICAL, this, "Palettes");
    wxStaticBoxSizer* sbox3 = new wxStaticBoxSizer(wxHORIZONTAL, this, "Sprites");

    for (int i = 0; i < 4; ++i)
    {
        nameTable[i] = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(256, 240));
        nameTable[i]->SetBackgroundColour(*wxBLACK);
    }

    for (int i = 0; i < 8; ++i)
    {
        palette[i] = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(64, 16));
        palette[i]->SetBackgroundColour(*wxBLACK);
    }

    for (int i = 0; i < 64; ++i)
    {
        primarySprite[i] = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(8, 8));
        primarySprite[i]->SetBackgroundColour(*wxBLACK);
    }

    topsizer->Add(hbox0, 0, wxALL, 5);

    hbox0->Add(sbox0);
    hbox0->AddSpacer(5);
    hbox0->Add(vbox0);

    sbox0->Add(grid0);

    for (int i = 0; i < 4; ++i)
    {
        grid0->Add(nameTable[i]);
    }

    vbox0->Add(sbox1);
    sbox1->Add(patternTableDisplay);

    vbox0->Add(sbox2);
    sbox2->Add(grid1);

    for (int i = 0; i < 8; ++i)
    {
        grid1->Add(palette[i]);
    }

    vbox0->Add(sbox3);
    sbox3->Add(grid2);
    sbox3->AddSpacer(8);

    for (int i = 0; i < 64; ++i)
    {
        grid2->Add(primarySprite[i]);
    }

    SetBackgroundColour(*wxWHITE);
    SetSizer(topsizer);
    Fit();

    Connect(wxID_ANY, wxEVT_CLOSE_WINDOW, wxCommandEventHandler(PPUDebugWindow::OnQuit));
}

void PPUDebugWindow::UpdateNameTable(int tableID, unsigned char* data)
{
    wxImage image(256, 240, data, true);
    wxBitmap bitmap(image, 24);

    wxClientDC dc(nameTable[tableID]);
    dc.DrawBitmap(bitmap, 0, 0);
}

void PPUDebugWindow::UpdatePatternTable(int tableID, unsigned char* data)
{
    if (tableID == 0)
    {
        patternTableDisplay->UpdateTable1(data);
    }
    else if (tableID == 1)
    {
        patternTableDisplay->UpdateTable2(data);
    }
}

void PPUDebugWindow::UpdatePalette(int tableID, unsigned char* data)
{
    wxImage image(64, 16, data, true);
    wxBitmap bitmap(image, 24);

    wxClientDC dc(palette[tableID]);
    dc.DrawBitmap(bitmap, 0, 0);
}

void PPUDebugWindow::UpdatePrimarySprite(int sprite, unsigned char* data)
{
    wxImage image(8, 8, data, true);
    wxBitmap bitmap(image, 24);

    wxClientDC dc(primarySprite[sprite]);
    dc.DrawBitmap(bitmap, 0, 0);
}

void PPUDebugWindow::ClearAll()
{
    for (int i = 0; i < 4; ++i)
    {
        wxImage image(256, 240);
        wxBitmap bitmap(image, 24);

        wxClientDC dc(nameTable[i]);
        dc.DrawBitmap(bitmap, 0, 0);
    }

    patternTableDisplay->Clear();

    for (int i = 0; i < 8; ++i)
    {
        wxImage image(64, 16);
        wxBitmap bitmap(image, 24);

        wxClientDC dc(palette[i]);
        dc.DrawBitmap(bitmap, 0, 0);
    }

    for (int i = 0; i < 64; ++i)
    {
        wxImage image(8, 8);
        wxBitmap bitmap(image, 24);

        wxClientDC dc(primarySprite[i]);
        dc.DrawBitmap(bitmap, 0, 0);
    }
}

int PPUDebugWindow::GetCurrentPalette()
{
    return patternTableDisplay->GetCurrentPalette();
}