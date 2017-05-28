#pragma once

#include <wx/dialog.h>

class NES;
class MainWindow;
class wxComboBox;
class wxCheckBox;

wxDECLARE_EVENT(EVT_VIDEO_WINDOW_CLOSED, wxCommandEvent);

class VideoSettingsWindow : public wxDialog
{
public:
    VideoSettingsWindow(MainWindow* parentWindow);
    void SetNes(NES* nes);

private:
    void OnClose(wxCloseEvent& event);
    void OnOk(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);

    void ResolutionChanged(wxCommandEvent& event);
    void EnableNtscDecodingClicked(wxCommandEvent& event);
    void EnableOverscanClicked(wxCommandEvent& event);
    void ShowFpsCounterClicked(wxCommandEvent& event);

    void UpdateGameResolution(int resIndex, bool overscan);
    void UpdateNtscDecoding(bool enabled);
    void UpdateShowFpsCounter(bool enabled);

    NES* Nes;

    wxComboBox* ResolutionComboBox;
    wxCheckBox* EnableNtscDecoding;
    wxCheckBox* EnableOverscan;
    wxCheckBox* ShowFpsCounter;
};

const int ID_RESOLUTION_CHANGED = 300;
const int ID_NTSC_ENABLED = 301;
const int ID_OVERSCAN_ENABLED = 302;
const int ID_SHOW_FPS_COUNTER = 303;
