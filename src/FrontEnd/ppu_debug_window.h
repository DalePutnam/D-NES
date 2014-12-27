#ifndef PPU_DEBUG_WINDOW_H_
#define PPU_DEBUG_WINDOW_H_

#include "wx/frame.h"
#include "wx/panel.h"

class MainWindow;

class PPUDebugWindow : public wxFrame
{
	MainWindow* mainWindow;

	int palette;

	wxPanel* nameTable0;
	wxPanel* nameTable1;
	wxPanel* nameTable2;
	wxPanel* nameTable3;

	wxPanel* patternTable0;
	wxPanel* patternTable1;

	wxPanel* palette0;
	wxPanel* palette1;
	wxPanel* palette2;
	wxPanel* palette3;
	wxPanel* palette4;
	wxPanel* palette5;
	wxPanel* palette6;
	wxPanel* palette7;

	void OnQuit(wxCommandEvent& event);
	void OnPatternTableClicked(wxMouseEvent& event);

public:
	explicit PPUDebugWindow(MainWindow* mainWindow);

	void UpdateNameTable(int tableID, unsigned char* data);
	void UpdatePatternTable(int tableID, unsigned char* data);
	void UpdatePalette(int tableID, unsigned char* data);
	int GetCurrentPalette();
};

#endif