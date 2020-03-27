#pragma once

#include <mutex>
#include <memory>
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/combobox.h>

#include <dnes/dnes.h>

#include "utilities/nes_ptr.h"

#ifdef _WIN32
#include <future>
#endif

wxDECLARE_EVENT(EVT_PPU_VIEWER_UPDATE, wxThreadEvent);
wxDECLARE_EVENT(EVT_PPU_VIEWER_CLOSED, wxCommandEvent);

class PPUViewerWindow : public wxFrame
{
public:
    explicit PPUViewerWindow(wxWindow* parent, NESPtr& nes);
    ~PPUViewerWindow() noexcept;

    void UpdatePanels();
    void ClearAll();

private:
    void InitializeLayout();
    void BindEvents();

    void OnQuit(wxCommandEvent& event);
    void OnPaletteSelected(wxCommandEvent& event);
    void OnPaintPanels(wxPaintEvent& evt);
    void DoUpdate(wxThreadEvent& evt);

    std::recursive_mutex UpdateLock;

    wxPanel* PatternTables;
    wxPanel* NameTables;
    wxPanel* Palettes;
    wxPanel* Sprite[64];
    wxChoice* PaletteSelect;

    NESPtr& Nes;

    int SelectedPalette;
    uint8_t* NameTableBuffers[4];
    uint8_t* PatternTableBuffers[2];
    uint8_t* PaletteBuffers[8];
    uint8_t* SpriteBuffers[64];

#ifdef _WIN32
    std::future<void> Future;
#elif defined(__linux)
    bool UpdateEventPending;
#endif
};

static const int ID_NAME_TABLE_PANEL = 100;
static const int ID_PATTERN_TABLE_PANEL = 101;
static const int ID_PALETTE_PANEL = 102;
static const int ID_SPRITE_PANEL_BASE = 103;