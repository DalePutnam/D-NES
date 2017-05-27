#pragma once

#include <wx/dialog.h>

class NES;
class MainWindow;
class wxComboBox;
class wxCheckBox;

wxDECLARE_EVENT(EVT_AUDIO_WINDOW_CLOSED, wxCommandEvent);

class VideoSettingsWindow : public wxDialog
{
public:
    VideoSettingsWindow(MainWindow* parentWindow);
    void SetNes(NES* nes);

    //static std::vector<std::pair<std::string, wxSize> > ;

private:
    void OnClose(wxCloseEvent& event);
    void OnOk(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);

    NES* Nes;

    wxComboBox* ResolutionComboBox;
    wxCheckBox* EnableNtscDecoding;
    wxCheckBox* EnableOverscan;
};

const int ID_RESOLUTION_CHANGED = 300;
const int ID_NTSC_ENABLED = 301;
const int ID_OVERSCAN_ENABLED = 302;
