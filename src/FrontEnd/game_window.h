#ifndef GAME_WINDOW_H_
#define GAME_WINDOW_H_

#include "wx/frame.h"
#include "wx/image.h"

class MainWindow;

class GameWindow : public wxFrame
{
    MainWindow* mainWindow;

    std::string filename;
    void OnQuit(wxCommandEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);

public:
    GameWindow(MainWindow* mainWindow, wxString filename);

    void SetFPS(int fps);
    void UpdateImage(unsigned char* data);
};

#endif