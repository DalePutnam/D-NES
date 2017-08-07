#pragma once

#include <mutex>
#include <condition_variable>
#include <future>
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/combobox.h>

wxDECLARE_EVENT(EVT_PPU_VIEWER_UPDATE, wxThreadEvent);
wxDECLARE_EVENT(EVT_PPU_VIEWER_CLOSED, wxCommandEvent);

class NES;

class PPUViewerWindow : public wxFrame
{
public:
    explicit PPUViewerWindow(wxWindow* parent, NES* nes = nullptr);
    ~PPUViewerWindow();

    void UpdatePanels();
    void ClearAll();
    void SetNes(NES* nes);

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

    NES* Nes;

	std::future<void> fut;
	bool EventHandled;
	int SelectedPalette;
    uint8_t* NameTableBuffers[4];
    uint8_t* PatternTableBuffers[2];
    uint8_t* PaletteBuffers[8];
    uint8_t* SpriteBuffers[64];
};

static const int ID_NAME_TABLE_PANEL = 100;
static const int ID_PATTERN_TABLE_PANEL = 101;
static const int ID_PALETTE_PANEL = 102;
static const int ID_SPRITE_PANEL_BASE = 103;