#include <wx/panel.h>
#include <wx/combobox.h>
#include <wx/statbox.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <vector>

#include "main_window.h"
#include "video_settings_window.h"
#include "utilities/app_settings.h"

#include "nes.h"

wxDEFINE_EVENT(EVT_VIDEO_WINDOW_CLOSED, wxCommandEvent);

VideoSettingsWindow::VideoSettingsWindow(MainWindow* parent)
    : SettingsWindowBase(parent, "Video Settings")
{
    InitializeLayout();
    BindEvents();
}

void VideoSettingsWindow::InitializeLayout()
{
    AppSettings* settings = AppSettings::GetInstance();

    wxString choices[NUM_RESOLUTIONS];
    choices[_256X240] = "256x240 (x1)";
    choices[_512X480] = "512x480 (x2)";
    choices[_768X720] = "768x720 (x3)";
    choices[_1024X960] = "1024x960 (x4)";

    ResolutionComboBox = new wxComboBox(SettingsPanel, ID_RESOLUTION_CHANGED, "", wxDefaultPosition, wxDefaultSize, NUM_RESOLUTIONS, choices, wxCB_READONLY);

    int currentChoice;
    settings->Read("/Video/Resolution", &currentChoice);
    ResolutionComboBox->SetSelection(currentChoice);

    EnableNtscDecoding = new wxCheckBox(SettingsPanel, ID_NTSC_ENABLED, "Enable NTCS Decoding");
    EnableOverscan = new wxCheckBox(SettingsPanel, ID_OVERSCAN_ENABLED, "Enable Overscan");
    ShowFpsCounter = new wxCheckBox(SettingsPanel, ID_SHOW_FPS_COUNTER, "Show FPS");

    bool ntscDecoding, overscan, showFps;
    settings->Read("/Video/NtscDecoding", &ntscDecoding);
    settings->Read("/Video/Overscan", &overscan);
    settings->Read("/Video/ShowFps", &showFps);

    EnableNtscDecoding->SetValue(ntscDecoding);
    EnableOverscan->SetValue(overscan);
    ShowFpsCounter->SetValue(showFps);

    wxStaticBoxSizer* resSizer = new wxStaticBoxSizer(wxVERTICAL, SettingsPanel, "Resolution");
    resSizer->Add(ResolutionComboBox, wxSizerFlags().Expand().Border(wxALL, 5));

    wxStaticBoxSizer* otherSizer = new wxStaticBoxSizer(wxVERTICAL, SettingsPanel, "Other");
    otherSizer->Add(EnableNtscDecoding);
    otherSizer->Add(EnableOverscan);
    otherSizer->Add(ShowFpsCounter);

    wxBoxSizer* settingsSizer = new wxBoxSizer(wxVERTICAL);
    settingsSizer->Add(resSizer, wxSizerFlags().Expand().Border(wxALL, 5));
    settingsSizer->Add(otherSizer, wxSizerFlags().Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM, 5));

    SettingsPanel->SetSizer(settingsSizer);

    Fit();
}

void VideoSettingsWindow::BindEvents()
{
    Bind(wxEVT_COMBOBOX, &VideoSettingsWindow::ResolutionChanged, this, ID_RESOLUTION_CHANGED);
    Bind(wxEVT_CHECKBOX, &VideoSettingsWindow::EnableNtscDecodingClicked, this, ID_NTSC_ENABLED);
    Bind(wxEVT_CHECKBOX, &VideoSettingsWindow::EnableOverscanClicked, this, ID_OVERSCAN_ENABLED);
    Bind(wxEVT_CHECKBOX, &VideoSettingsWindow::ShowFpsCounterClicked, this, ID_SHOW_FPS_COUNTER);
}

void VideoSettingsWindow::DoClose()
{
    wxCommandEvent* evt = new wxCommandEvent(EVT_VIDEO_WINDOW_CLOSED);
    wxQueueEvent(GetParent(), evt);
    Hide();
}

void VideoSettingsWindow::DoOk()
{
    AppSettings* settings = AppSettings::GetInstance();

    settings->Write("/Video/Resolution", ResolutionComboBox->GetSelection());
    settings->Write("/Video/NtscDecoding", EnableNtscDecoding->GetValue());
    settings->Write("/Video/Overscan", EnableOverscan->GetValue());
    settings->Write("/Video/ShowFps", ShowFpsCounter->GetValue());

    UpdateNtscDecoding(EnableNtscDecoding->GetValue());
    UpdateShowFpsCounter(ShowFpsCounter->GetValue());
    UpdateGameResolution(ResolutionComboBox->GetSelection(), EnableOverscan->GetValue());

    Close();
}

void VideoSettingsWindow::DoCancel()
{
    AppSettings* settings = AppSettings::GetInstance();

    int resolution;
    bool overscan, ntscDecoding, showFps;

    settings->Read("/Video/Resolution", &resolution);
    settings->Read("/Video/NtscDecoding", &ntscDecoding);
    settings->Read("/Video/Overscan", &overscan);
    settings->Read("/Video/ShowFps", &showFps);

    UpdateNtscDecoding(ntscDecoding);
    UpdateShowFpsCounter(showFps);
    UpdateGameResolution(resolution, overscan);

    Close();
}

void VideoSettingsWindow::ResolutionChanged(wxCommandEvent& WXUNUSED(event))
{
    UpdateGameResolution(ResolutionComboBox->GetSelection(), EnableOverscan->GetValue());
}

void VideoSettingsWindow::EnableNtscDecodingClicked(wxCommandEvent& WXUNUSED(event))
{
    UpdateNtscDecoding(EnableNtscDecoding->GetValue());
}

void VideoSettingsWindow::EnableOverscanClicked(wxCommandEvent& WXUNUSED(event))
{
    UpdateGameResolution(ResolutionComboBox->GetSelection(), EnableOverscan->GetValue());
}

void VideoSettingsWindow::ShowFpsCounterClicked(wxCommandEvent& WXUNUSED(event))
{
    UpdateShowFpsCounter(ShowFpsCounter->GetValue());
}

void VideoSettingsWindow::UpdateGameResolution(int resIndex, bool overscan)
{
    if (MainWindow* parent = dynamic_cast<MainWindow*>(GetParent()))
    {
        GameResolutions resolution = static_cast<GameResolutions>(resIndex);
        parent->SetGameResolution(resolution, overscan);
    }
}

void VideoSettingsWindow::UpdateNtscDecoding(bool enabled)
{
    if (Nes != nullptr)
    {
        Nes->PpuSetNtscDecoderEnabled(enabled);
    }
}

void VideoSettingsWindow::UpdateShowFpsCounter(bool enabled)
{
    if (MainWindow* parent = dynamic_cast<MainWindow*>(GetParent()))
    {
        parent->SetShowFpsCounter(enabled);
    }
}
