#pragma once

#include "settings_window_base.h"

class MainWindow;
class wxComboBox;
class wxCheckBox;

wxDECLARE_EVENT(EVT_VIDEO_WINDOW_CLOSED, wxCommandEvent);

class VideoSettingsWindow : public SettingsWindowBase
{
public:
    VideoSettingsWindow(MainWindow* parent);

protected:
    virtual void DoOk() override;
    virtual void DoCancel() override;
    virtual void DoClose() override;

private:
    void InitializeLayout();
    void BindEvents();

    void ResolutionChanged(wxCommandEvent& event);
    void EnableNtscDecodingClicked(wxCommandEvent& event);
    void EnableOverscanClicked(wxCommandEvent& event);
    void ShowFpsCounterClicked(wxCommandEvent& event);

    void UpdateGameResolution(int resIndex, bool overscan);
    void UpdateNtscDecoding(bool enabled);
    void UpdateShowFpsCounter(bool enabled);

    wxComboBox* ResolutionComboBox;
    wxCheckBox* EnableNtscDecoding;
    wxCheckBox* EnableOverscan;
    wxCheckBox* ShowFpsCounter;
};

const int ID_RESOLUTION_CHANGED = 300;
const int ID_NTSC_ENABLED = 301;
const int ID_OVERSCAN_ENABLED = 302;
const int ID_SHOW_FPS_COUNTER = 303;
