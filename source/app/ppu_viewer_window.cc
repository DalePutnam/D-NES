#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/statbox.h>
#include <wx/image.h>
#include <wx/bitmap.h>
#include <wx/dcclient.h>

#include "ppu_viewer_window.h"
#include "main_window.h"
#include "dnes/nes.h"

static constexpr long FRAME_STYLE = (wxDEFAULT_FRAME_STYLE | wxFRAME_NO_TASKBAR) & ~wxRESIZE_BORDER & ~wxMAXIMIZE_BOX & ~wxMINIMIZE_BOX;

wxDEFINE_EVENT(EVT_PPU_VIEWER_UPDATE, wxThreadEvent);
wxDEFINE_EVENT(EVT_PPU_VIEWER_CLOSED, wxCommandEvent);

void PPUViewerWindow::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    wxCommandEvent* evt = new wxCommandEvent(EVT_PPU_VIEWER_CLOSED);
    wxQueueEvent(GetParent(), evt);
    Hide();
}

void PPUViewerWindow::OnPaletteSelected(wxCommandEvent&)
{
    SelectedPalette = PaletteSelect->GetSelection();
}

PPUViewerWindow::PPUViewerWindow(wxWindow* parent, std::unique_ptr<NES>& nes)
    : wxFrame(parent, wxID_ANY, "PPU Viewer", wxDefaultPosition, wxDefaultSize, FRAME_STYLE)
    , Nes(nes)
    , SelectedPalette(0)
#ifdef __linux
    , UpdateEventPending(false)
#endif
{
    for (uint32_t i = 0; i < 64; ++i)
    {
        SpriteBuffers[i] = new uint8_t[8*8*3];

        if (i < 8)
        {
            PaletteBuffers[i] = new uint8_t[64*16*3];
        }

        if (i < 4)
        {
            NameTableBuffers[i] = new uint8_t[256*240*3];
        }

        if (i < 2)
        {
            PatternTableBuffers[i] = new uint8_t[128*128*3];
        }
    }

    InitializeLayout();
    BindEvents();
}

PPUViewerWindow::~PPUViewerWindow()
{
#ifdef _WIN32
    // Make sure last async call has completed before deleting the window
    if (Future.valid())
    {
        Future.get();
    }
#endif

    std::unique_lock<std::recursive_mutex> lock(UpdateLock);

    for (uint32_t i = 0; i < 64; ++i)
    {
        delete [] SpriteBuffers[i];

        if (i < 8)
        {
            delete [] PaletteBuffers[i];
        }

        if (i < 4)
        {
            delete [] NameTableBuffers[i];
        }

        if (i < 2)
        {
            delete [] PatternTableBuffers[i];
        }
    }
}

void PPUViewerWindow::InitializeLayout()
{
    SetBackgroundColour(*wxWHITE);

    // Top level sizer, used to create outer border
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(topSizer);

    wxBoxSizer* firstLevelColumns = new wxBoxSizer(wxHORIZONTAL);
    topSizer->Add(firstLevelColumns, 0, wxALL, 5);

    // Initialize Name Table Display
    wxStaticBoxSizer* nameTableBox = new wxStaticBoxSizer(wxVERTICAL, this, "Name Tables");
    NameTables = new wxPanel(this, ID_NAME_TABLE_PANEL, wxDefaultPosition, wxSize(512, 480));
    nameTableBox->Add(NameTables, wxSizerFlags().Border(wxALL, 5));
    firstLevelColumns->Add(nameTableBox);

    wxBoxSizer* secondLevelRows = new wxBoxSizer(wxVERTICAL);
    firstLevelColumns->AddSpacer(5);
    firstLevelColumns->Add(secondLevelRows);

    // Initialize Pattern Table Display
    wxStaticBoxSizer* patternTableBox = new wxStaticBoxSizer(wxVERTICAL, this, "Pattern Tables");
    PatternTables = new wxPanel(this, ID_PATTERN_TABLE_PANEL, wxDefaultPosition, wxSize(256, 128));
    patternTableBox->Add(PatternTables, wxSizerFlags().Border(wxALL, 5));
    secondLevelRows->Add(patternTableBox);
    
    wxString choices[8] =
    {
        "Background Palette 1",
        "Background Palette 2",
        "Background Palette 3",
        "Background Palette 4",
        "Sprite Palette 1",
        "Sprite Palette 2",
        "Sprite Palette 3",
        "Sprite Palette 4"
    };

    PaletteSelect = new wxChoice(this, ID_PALETTE_PANEL, wxDefaultPosition, wxDefaultSize, 8, choices);
    PaletteSelect->SetSelection(0);

    patternTableBox->Add(PaletteSelect, wxSizerFlags().Expand().Border(wxALL, 5));

    // Initialize Palette Display
    wxStaticBoxSizer* paletteBox = new wxStaticBoxSizer(wxVERTICAL, this, "Palettes");
    Palettes = new wxPanel(this, ID_PALETTE_PANEL, wxDefaultPosition, wxSize(256, 32));
    paletteBox->Add(Palettes, wxSizerFlags().Border(wxALL, 5));
    secondLevelRows->Add(paletteBox);

    // Initialize Sprite Display
    wxStaticBoxSizer* spriteBox = new wxStaticBoxSizer(wxHORIZONTAL, this, "Sprites");
    wxGridSizer* spriteGrid = new wxGridSizer(4, 16, 8, 8);

    for (int i = 0; i < 64; ++i)
    {
        Sprite[i] = new wxPanel(this, ID_SPRITE_PANEL_BASE + i, wxDefaultPosition, wxSize(8, 8));
        Sprite[i]->SetBackgroundColour(*wxBLACK);
        spriteGrid->Add(Sprite[i]);
    }

    spriteBox->Add(spriteGrid, wxSizerFlags().Border(wxALL, 5));
    spriteBox->AddSpacer(8);
    secondLevelRows->Add(spriteBox);

    Fit();
}

void PPUViewerWindow::BindEvents()
{
    Bind(EVT_PPU_VIEWER_UPDATE, wxThreadEventHandler(PPUViewerWindow::DoUpdate), this, wxID_ANY);
    Bind(wxEVT_CLOSE_WINDOW, wxCommandEventHandler(PPUViewerWindow::OnQuit), this, wxID_ANY);
    Bind(wxEVT_CHOICE, wxCommandEventHandler(PPUViewerWindow::OnPaletteSelected), this, wxID_ANY);


    NameTables->Bind(wxEVT_PAINT, wxPaintEventHandler(PPUViewerWindow::OnPaintPanels), this, wxID_ANY);
    PatternTables->Bind(wxEVT_PAINT, wxPaintEventHandler(PPUViewerWindow::OnPaintPanels), this, wxID_ANY);
    Palettes->Bind(wxEVT_PAINT, wxPaintEventHandler(PPUViewerWindow::OnPaintPanels), this, wxID_ANY);

    for (int i = 0; i < 64; ++i)
    {
        Sprite[i]->Bind(wxEVT_PAINT, wxPaintEventHandler(PPUViewerWindow::OnPaintPanels), this, wxID_ANY);
    }
}

void PPUViewerWindow::OnPaintPanels(wxPaintEvent& evt)
{
    std::unique_lock<std::recursive_mutex> lock(UpdateLock);

    if (Nes == nullptr)
    {
        int id = evt.GetId();

        if (id == ID_NAME_TABLE_PANEL)
        {
            wxPaintDC dc(NameTables);
            dc.SetBrush(wxBrush(*wxBLACK));
            dc.DrawRectangle(0, 0, 512, 480);
        }
        else if (id == ID_PATTERN_TABLE_PANEL)
        {
            wxPaintDC dc(PatternTables);
            dc.SetBrush(wxBrush(*wxBLACK));
            dc.DrawRectangle(0, 0, 256, 128);
        }
        else if (id == ID_PALETTE_PANEL)
        {
            wxPaintDC dc(Palettes);
            dc.SetBrush(wxBrush(*wxBLACK));
            dc.DrawRectangle(0, 0, 256, 32);
        }
        else if (id >= ID_SPRITE_PANEL_BASE && id < ID_SPRITE_PANEL_BASE + 64)
        {
            int i = id - ID_SPRITE_PANEL_BASE;

            wxPaintDC dc(Sprite[i]);
            dc.SetBrush(wxBrush(*wxBLACK));
            dc.DrawRectangle(0, 0, 8, 8);
        }
    }
}

void PPUViewerWindow::DoUpdate(wxThreadEvent&)
{
    std::unique_lock<std::recursive_mutex> lock(UpdateLock);

    if (Nes == nullptr)
    {
#ifdef __linux
        UpdateEventPending = false;
#endif
        return;
    }

    for (int i = 0; i < 64; ++i)
    {
        wxImage image(8, 8, SpriteBuffers[i], true);
        wxBitmap bitmap(image, 24);
        wxClientDC dc(Sprite[i]);
        dc.DrawBitmap(bitmap, 0, 0);

        if (i < 8)
        {
            wxImage image(64, 16, PaletteBuffers[i], true);
            wxBitmap bitmap(image, 24);
            wxClientDC dc(Palettes);
            dc.DrawBitmap(bitmap, 64*(i%4), 16*(i/4));
        }

        if (i < 4)
        {
            wxImage image(256, 240, NameTableBuffers[i], true);
            wxBitmap bitmap(image, 24);
            wxClientDC dc(NameTables);
            dc.DrawBitmap(bitmap, 256*(i%2), 240*(i/2));
        }

        if (i < 2)
        {
            wxImage image(128, 128, PatternTableBuffers[i], true);
            wxBitmap bitmap(image, 24);
            wxClientDC dc(PatternTables);
            dc.DrawBitmap(bitmap, 128*i, 0);
        }
    }
#ifdef __linux
    UpdateEventPending = false;
#endif
}

void PPUViewerWindow::UpdatePanels()
{
#ifdef _WIN32
    // Before acquiring the lock make sure that the last async call completed
    if (Future.valid())
    {
        Future.get();
    }
#endif

    std::unique_lock<std::recursive_mutex> lock(UpdateLock);

    if (Nes == nullptr)
    {
        return;
    }

    for (int i = 0; i < 64; ++i)
    {
        Nes->GetSprite(i, SpriteBuffers[i]);

        if (i < 8)
        {
            Nes->GetPalette(i, PaletteBuffers[i]);
        }

        if (i < 4)
        {
            Nes->GetNameTable(i, NameTableBuffers[i]);
        }

        if (i < 2)
        {
            Nes->GetPatternTable(i, SelectedPalette, PatternTableBuffers[i]);
        }
    }

#ifdef _WIN32
    Future = std::async(std::launch::async, &PPUViewerWindow::DoUpdate, this, wxThreadEvent(EVT_PPU_VIEWER_UPDATE));
#elif defined(__linux)   
    // If there's already an update event pending don't bother queuing another one
    if (!UpdateEventPending)
    {
        wxThreadEvent evt(EVT_PPU_VIEWER_UPDATE);
        wxQueueEvent(this, evt.Clone());
        UpdateEventPending = true;
    }
#endif
}

void PPUViewerWindow::ClearAll()
{
    std::unique_lock<std::recursive_mutex> lock(UpdateLock);

    {
        wxClientDC dc(NameTables);
        dc.SetBrush(wxBrush(*wxBLACK));
        dc.DrawRectangle(0, 0, 512, 480);
    }

    {
        wxClientDC dc(PatternTables);
        dc.SetBrush(wxBrush(*wxBLACK));
        dc.DrawRectangle(0, 0, 256, 128);
    }
    
    {
        wxClientDC dc(Palettes);
        dc.SetBrush(wxBrush(*wxBLACK));
        dc.DrawRectangle(0, 0, 256, 32);
    }

    for (int i = 0; i < 64; ++i)
    {
        wxClientDC dc(Sprite[i]);
        dc.SetBrush(wxBrush(*wxBLACK));
        dc.DrawRectangle(0, 0, 8, 8);
    }
}
