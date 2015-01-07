#ifndef PPU_DEBUG_WINDOW_H_
#define PPU_DEBUG_WINDOW_H_

#include "wx/frame.h"
#include "wx/panel.h"

class MainWindow;

class PPUDebugWindow : public wxFrame
{
	MainWindow* mainWindow;

	int paletteIndex;

	wxPanel* nameTable[4];
	wxPanel* patternTable[2];
	wxPanel* palette[8];
	wxPanel* primarySprite[64];
	wxPanel* secondarySprite[8];
	
	void OnQuit(wxCommandEvent& event);
	void OnPatternTableClicked(wxMouseEvent& event);

public:
	explicit PPUDebugWindow(MainWindow* mainWindow);

	void UpdateNameTable(int tableID, unsigned char* data);
	void UpdatePatternTable(int tableID, unsigned char* data);
	void UpdatePalette(int tableID, unsigned char* data);
	void UpdatePrimarySprite(int sprite, unsigned char* data);
	void UpdateSecondarySprite(int sprite, unsigned char* data);
	int GetCurrentPalette();
};

#endif