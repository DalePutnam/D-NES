#include <sstream>

#include "game_window.h"
#include "main_window.h"

#include "wx/dcclient.h"
#include "wx/bitmap.h"
#include "nes_thread.h"

void GameWindow::OnQuit(wxCommandEvent& WXUNUSED(event))
{ 
    mainWindow->StopEmulator();
}

void GameWindow::SetFPS(int fps)
{
    std::ostringstream oss;
    oss << fps << " FPS " << filename;
    this->SetTitle(oss.str());
}

void GameWindow::OnKeyDown(wxKeyEvent& event)
{
    NESThread* nesThread = mainWindow->GetNESThread();

    if (nesThread)
    {
        unsigned char currentState = nesThread->GetControllerOneState();

        switch (event.GetKeyCode())
        {
        case 'Z':
            nesThread->SetControllerOneState(currentState | 0x1);
            break;
        case 'X':
            nesThread->SetControllerOneState(currentState | 0x2);
            break;
        case WXK_CONTROL:
            nesThread->SetControllerOneState(currentState | 0x4);
            break;
        case WXK_RETURN:
            nesThread->SetControllerOneState(currentState | 0x8);
            break;
        case WXK_UP:
            nesThread->SetControllerOneState(currentState | 0x10);
            break;
        case WXK_DOWN:
            nesThread->SetControllerOneState(currentState | 0x20);
            break;
        case WXK_LEFT:
            nesThread->SetControllerOneState(currentState | 0x40);
            break;
        case WXK_RIGHT:
            nesThread->SetControllerOneState(currentState | 0x80);
            break;
        default:
            break;
        }
    }
}

void GameWindow::OnKeyUp(wxKeyEvent& event)
{
    NESThread* nesThread = mainWindow->GetNESThread();

    if (nesThread)
    {
        unsigned char currentState = nesThread->GetControllerOneState();

        switch (event.GetKeyCode())
        {
        case 'Z':
            nesThread->SetControllerOneState(currentState & ~0x1);
            break;
        case 'X':
            nesThread->SetControllerOneState(currentState & ~0x2);
            break;
        case WXK_CONTROL:
            nesThread->SetControllerOneState(currentState & ~0x4);
            break;
        case WXK_RETURN:
            nesThread->SetControllerOneState(currentState & ~0x8);
            break;
        case WXK_UP:
            nesThread->SetControllerOneState(currentState & ~0x10);
            break;
        case WXK_DOWN:
            nesThread->SetControllerOneState(currentState & ~0x20);
            break;
        case WXK_LEFT:
            nesThread->SetControllerOneState(currentState & ~0x40);
            break;
        case WXK_RIGHT:
            nesThread->SetControllerOneState(currentState & ~0x80);
            break;
        default:
            break;
        }
    }
}

GameWindow::GameWindow(MainWindow* mainWindow, wxString filename)
    : wxFrame(NULL, wxID_ANY, filename, wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE & ~wxRESIZE_BORDER & ~wxMAXIMIZE_BOX),
    mainWindow(mainWindow),
    filename(filename)
{
    SetClientSize(256, 240);
    Connect(wxID_ANY, wxEVT_CLOSE_WINDOW, wxCommandEventHandler(GameWindow::OnQuit));
    Connect(wxID_ANY, wxEVT_KEY_DOWN, wxKeyEventHandler(GameWindow::OnKeyDown));
    Connect(wxID_ANY, wxEVT_KEY_UP, wxKeyEventHandler(GameWindow::OnKeyUp));
}

void GameWindow::UpdateImage(unsigned char* data)
{
    wxClientDC dc(this);
    wxImage image(256, 240, data, true);
    wxBitmap bitmap(image, 24);

    dc.DrawBitmap(bitmap, 0, 0);
}