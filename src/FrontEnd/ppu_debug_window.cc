#include "ppu_debug_window.h"
#include "main_window.h"

#include "wx/sizer.h"
#include "wx/stattext.h"
#include "wx/statbox.h"
#include "wx/image.h"
#include "wx/bitmap.h"
#include "wx/dcclient.h"

void PPUDebugWindow::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	mainWindow->PPUDebugClose();
}

void PPUDebugWindow::OnPatternTableClicked(wxMouseEvent& WXUNUSED(event))
{
	palette = (palette + 1) % 8;
}

PPUDebugWindow::PPUDebugWindow(MainWindow* mainWindow)
	: wxFrame(NULL, wxID_ANY, "PPU Debug", wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE & ~wxRESIZE_BORDER & ~wxMAXIMIZE_BOX),
	mainWindow(mainWindow),
	palette(0)
{
	wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* hbox0 = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* hbox1 = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* vbox0 = new wxBoxSizer(wxVERTICAL);
	wxGridSizer* grid0 = new wxGridSizer(2, 2, 5, 5);
	wxGridSizer* grid1 = new wxGridSizer(2, 4, 0, 0);

	wxStaticBoxSizer* sbox0 = new wxStaticBoxSizer(wxVERTICAL, this, "Name Tables");
	wxStaticBoxSizer* sbox1 = new wxStaticBoxSizer(wxHORIZONTAL, this, "Pattern Tables");
	wxStaticBoxSizer* sbox2 = new wxStaticBoxSizer(wxVERTICAL, this, "Palettes");

	nameTable0 = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(256, 240));
	nameTable1 = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(256, 240));
	nameTable2 = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(256, 240));
	nameTable3 = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(256, 240));
	nameTable0->SetBackgroundColour(*wxBLACK);
	nameTable1->SetBackgroundColour(*wxBLACK);
	nameTable2->SetBackgroundColour(*wxBLACK);
	nameTable3->SetBackgroundColour(*wxBLACK);

	patternTable0 = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(128, 128));
	patternTable1 = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(128, 128));
	patternTable0->SetBackgroundColour(*wxBLACK);
	patternTable1->SetBackgroundColour(*wxBLACK);

	palette0 = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(64, 16));
	palette1 = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(64, 16));
	palette2 = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(64, 16));
	palette3 = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(64, 16));
	palette4 = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(64, 16));
	palette5 = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(64, 16));
	palette6 = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(64, 16));
	palette7 = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(64, 16));
	palette0->SetBackgroundColour(*wxBLACK);
	palette1->SetBackgroundColour(*wxBLACK);
	palette2->SetBackgroundColour(*wxBLACK);
	palette3->SetBackgroundColour(*wxBLACK);
	palette4->SetBackgroundColour(*wxBLACK);
	palette5->SetBackgroundColour(*wxBLACK);
	palette6->SetBackgroundColour(*wxBLACK);
	palette7->SetBackgroundColour(*wxBLACK);

	topsizer->Add(hbox0, 0, wxALL, 5);

	hbox0->Add(sbox0);
	hbox0->AddSpacer(5);
	hbox0->Add(vbox0);

	sbox0->Add(grid0);
	grid0->Add(nameTable0);
	grid0->Add(nameTable1);
	grid0->Add(nameTable2);
	grid0->Add(nameTable3);

	vbox0->Add(sbox1);
	sbox1->Add(patternTable0);
	sbox1->Add(patternTable1);

	vbox0->Add(sbox2);
	sbox2->Add(grid1);
	grid1->Add(palette0);
	grid1->Add(palette1);
	grid1->Add(palette2);
	grid1->Add(palette3);
	grid1->Add(palette4);
	grid1->Add(palette5);
	grid1->Add(palette6);
	grid1->Add(palette7);

	SetBackgroundColour(*wxLIGHT_GREY);
	SetSizer(topsizer);
	Fit();

	Connect(wxID_ANY, wxEVT_CLOSE_WINDOW, wxCommandEventHandler(PPUDebugWindow::OnQuit));
}

void PPUDebugWindow::UpdateNameTable(int tableID, unsigned char* data)
{
	wxImage image(256, 240, data, true);
	wxBitmap bitmap(image, 24);

	if (tableID == 0)
	{
		wxClientDC dc(nameTable0);
		dc.DrawBitmap(bitmap, 0, 0);
	}
	else if (tableID == 1)
	{
		wxClientDC dc(nameTable1);
		dc.DrawBitmap(bitmap, 0, 0);
	}
	else if (tableID == 2)
	{
		wxClientDC dc(nameTable2);
		dc.DrawBitmap(bitmap, 0, 0);
	}
	else 
	{
		wxClientDC dc(nameTable3);
		dc.DrawBitmap(bitmap, 0, 0);
	}
}

void PPUDebugWindow::UpdatePatternTable(int tableID, unsigned char* data)
{
	wxImage image(128, 128, data, true);
	wxBitmap bitmap(image, 24);

	if (tableID == 0)
	{
		wxClientDC dc(patternTable0);
		dc.DrawBitmap(bitmap, 0, 0);
	}
	else
	{
		wxClientDC dc(patternTable1);
		dc.DrawBitmap(bitmap, 0, 0);
	}
}

void PPUDebugWindow::UpdatePalette(int tableID, unsigned char* data)
{
	wxImage image(64, 16, data, true);
	wxBitmap bitmap(image, 24);

	if (tableID == 0)
	{
		wxClientDC dc(palette0);
		dc.DrawBitmap(bitmap, 0, 0);
	}
	else if (tableID == 1)
	{
		wxClientDC dc(palette1);
		dc.DrawBitmap(bitmap, 0, 0);
	}
	else if (tableID == 2)
	{
		wxClientDC dc(palette2);
		dc.DrawBitmap(bitmap, 0, 0);
	}
	else if (tableID == 3)
	{
		wxClientDC dc(palette3);
		dc.DrawBitmap(bitmap, 0, 0);
	}
	else if (tableID == 4)
	{
		wxClientDC dc(palette4);
		dc.DrawBitmap(bitmap, 0, 0);
	}
	else if (tableID == 5)
	{
		wxClientDC dc(palette5);
		dc.DrawBitmap(bitmap, 0, 0);
	}
	else if (tableID == 6)
	{
		wxClientDC dc(palette6);
		dc.DrawBitmap(bitmap, 0, 0);
	}
	else
	{
		wxClientDC dc(palette7);
		dc.DrawBitmap(bitmap, 0, 0);
	}
}

int PPUDebugWindow::GetCurrentPalette()
{
	return palette;
}