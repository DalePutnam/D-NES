#ifndef GAME_WINDOW_H_
#define GAME_WINDOW_H_

#include "wx/frame.h"
#include "wx/image.h"

class MainWindow;

class GameWindow : public wxFrame
{
    MainWindow* mainWindow;

    void OnQuit(wxCommandEvent& event);

public:
    GameWindow(MainWindow* mainWindow, wxString filename);

    void UpdateImage(unsigned char* data);
};

#endif