#include "game_window.h"
#include "main_window.h"

#include "wx/dcclient.h"
#include "wx/bitmap.h"

void GameWindow::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	mainWindow->StopEmulator();
}

GameWindow::GameWindow(MainWindow* mainWindow, wxString filename)
	: wxFrame(NULL, wxID_ANY, filename, wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE & ~wxRESIZE_BORDER & ~wxMAXIMIZE_BOX),
	mainWindow(mainWindow)
{
	SetClientSize(256, 240);
	Connect(wxID_ANY, wxEVT_CLOSE_WINDOW, wxCommandEventHandler(GameWindow::OnQuit));
}

void GameWindow::UpdateImage(unsigned char* data)
{
	wxClientDC dc(this);
	wxImage image(256, 240, data, true);
	wxBitmap bitmap(image, 24);

	dc.DrawBitmap(bitmap, 0, 0);
}