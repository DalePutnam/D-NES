#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/statbox.h>
#include <wx/image.h>
#include <wx/bitmap.h>
#include <wx/dcclient.h>

#include "ppu_debug_window.h"
#include "main_window.h"
#include "nes.h"

void PPUDebugWindow::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    ParentWindow->PPUDebugClose();
}

PPUDebugWindow::PPUDebugWindow(MainWindow* mainWindow, NES* nes)
    : wxFrame(mainWindow, wxID_ANY, "PPU Debug", wxDefaultPosition, wxDefaultSize, (wxDEFAULT_FRAME_STYLE | wxFRAME_NO_TASKBAR) & ~wxRESIZE_BORDER & ~wxMAXIMIZE_BOX & ~wxMINIMIZE_BOX)
    , ParentWindow(mainWindow)
    , Nes(nes)
{
    PatternDisplay = new PatternTableDisplay(this, nes);

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
        NameTable[i] = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(256, 240));
        NameTable[i]->SetBackgroundColour(*wxBLACK);
    }

    for (int i = 0; i < 8; ++i)
    {
        Palette[i] = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(64, 16));
        Palette[i]->SetBackgroundColour(*wxBLACK);
    }

    for (int i = 0; i < 64; ++i)
    {
        Sprite[i] = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(8, 8));
        Sprite[i]->SetBackgroundColour(*wxBLACK);
    }

    topsizer->Add(hbox0, 0, wxALL, 5);

    hbox0->Add(sbox0);
    hbox0->AddSpacer(5);
    hbox0->Add(vbox0);

    sbox0->Add(grid0, wxSizerFlags().Border(wxALL, 5));

    for (int i = 0; i < 4; ++i)
    {
        grid0->Add(NameTable[i]);
    }

    vbox0->Add(sbox1);
    sbox1->Add(PatternDisplay, wxSizerFlags().Border(wxALL, 5));

    vbox0->Add(sbox2);
    sbox2->Add(grid1, wxSizerFlags().Border(wxALL, 5));

    for (int i = 0; i < 8; ++i)
    {
        grid1->Add(Palette[i]);
    }

    vbox0->Add(sbox3);
    sbox3->Add(grid2, wxSizerFlags().Border(wxALL, 5));
    sbox3->AddSpacer(8);

    for (int i = 0; i < 64; ++i)
    {
        grid2->Add(Sprite[i]);
    }

    SetBackgroundColour(*wxWHITE);
    SetSizer(topsizer);
    Fit();

    Bind(wxEVT_CLOSE_WINDOW, wxCommandEventHandler(PPUDebugWindow::OnQuit), this, wxID_ANY);
}

void PPUDebugWindow::Update()
{
    if (Nes == nullptr)
    {
        return;
    }

    PatternDisplay->Update();

    for (int i = 0; i < 64; ++i)
    {
        if (i < 4)
        {
            uint8_t nameTable[256 * 240 * 3];
            Nes->GetNameTable(i, nameTable);
            wxImage image(256, 240, nameTable, true);
            wxBitmap bitmap(image, 24);

            wxClientDC dc(NameTable[i]);
            dc.DrawBitmap(bitmap, 0, 0);
        }

        if (i < 8)
        {
            uint8_t palette[64 * 16 * 3];
            Nes->GetPalette(i, palette);
            wxImage image(64, 16, palette, true);
            wxBitmap bitmap(image, 24);

            wxClientDC dc(Palette[i]);
            dc.DrawBitmap(bitmap, 0, 0);
        }

        uint8_t sprite[8 * 8 * 3];
        Nes->GetPrimarySprite(i, sprite);
        wxImage image(8, 8, sprite, true);
        wxBitmap bitmap(image, 24);

        wxClientDC dc(Sprite[i]);
        dc.DrawBitmap(bitmap, 0, 0);
    }
}

void PPUDebugWindow::ClearAll()
{
    for (int i = 0; i < 4; ++i)
    {
        wxImage image(256, 240, true);
        wxBitmap bitmap(image, 24);

        wxClientDC dc(NameTable[i]);
        dc.DrawBitmap(bitmap, 0, 0);
    }

    PatternDisplay->Clear();

    for (int i = 0; i < 8; ++i)
    {
        wxImage image(64, 16, true);
        wxBitmap bitmap(image, 24);

        wxClientDC dc(Palette[i]);
        dc.DrawBitmap(bitmap, 0, 0);
    }

    for (int i = 0; i < 64; ++i)
    {
        wxImage image(8, 8, true);
        wxBitmap bitmap(image, 24);

        wxClientDC dc(Sprite[i]);
        dc.DrawBitmap(bitmap, 0, 0);
    }
}

void PPUDebugWindow::SetNes(NES* nes)
{
    Nes = nes;
    PatternDisplay->SetNes(nes);
}

int PPUDebugWindow::GetCurrentPalette()
{
    return PatternDisplay->GetCurrentPalette();
}
