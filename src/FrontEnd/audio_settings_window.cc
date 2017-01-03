#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <wx/slider.h>

#include <sstream>

#include "main_window.h"
#include "audio_settings_window.h"
#include "utilities/app_settings.h"

#include "nes.h"

namespace
{
    constexpr int FRAME_STYLE = (wxDEFAULT_FRAME_STYLE | wxFRAME_NO_TASKBAR) & ~wxRESIZE_BORDER & ~wxMAXIMIZE_BOX & ~wxMINIMIZE_BOX;
};

wxDEFINE_EVENT(EVT_AUDIO_WINDOW_CLOSED, wxCommandEvent);

AudioSettingsWindow::AudioSettingsWindow(MainWindow* parentWindow)
    : wxDialog(parentWindow, wxID_ANY, "Audio Settings", wxDefaultPosition, wxDefaultSize, FRAME_STYLE)
    , Nes(nullptr)
{
    AppSettings* settings = AppSettings::GetInstance();

    wxPanel* settingsPanel = new wxPanel(this);
    settingsPanel->SetBackgroundColour(*wxWHITE);

    wxBoxSizer* volumeSizer = new wxBoxSizer(wxVERTICAL);
    wxGridSizer* volumeGrid = new wxGridSizer(1, 6, 0, 5);

    wxBoxSizer* masterBox = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* pulseOneBox = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* pulseTwoBox = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* triangleBox = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* noiseBox = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* dmcBox = new wxBoxSizer(wxVERTICAL);

    int sliderStyle = wxSL_VERTICAL | wxSL_INVERSE;

    int master, pulseOne, pulseTwo, triangle, noise, dmc;
    settings->Read("/Audio/MasterVolume", &master);
    settings->Read("/Audio/PulseOneVolume", &pulseOne);
    settings->Read("/Audio/PulseTwoVolume", &pulseTwo);
    settings->Read("/Audio/TriangleVolume", &triangle);
    settings->Read("/Audio/NoiseVolume", &noise);
    settings->Read("/Audio/DmcVolume", &dmc);

    MasterVolume = new wxSlider(settingsPanel, ID_MASTER_SLIDER, master, 0, 100, wxDefaultPosition, wxDefaultSize, sliderStyle);
    PulseOneVolume = new wxSlider(settingsPanel, ID_PULSE_ONE_SLIDER, pulseOne, 0, 100, wxDefaultPosition, wxDefaultSize, sliderStyle);
    PulseTwoVolume = new wxSlider(settingsPanel, ID_PULSE_TWO_SLIDER, pulseTwo, 0, 100, wxDefaultPosition, wxDefaultSize, sliderStyle);
    TriangleVolume = new wxSlider(settingsPanel, ID_TRIANGLE_SLIDER, triangle, 0, 100, wxDefaultPosition, wxDefaultSize, sliderStyle);
    NoiseVolume = new wxSlider(settingsPanel, ID_NOISE_SLIDER, noise, 0, 100, wxDefaultPosition, wxDefaultSize, sliderStyle);
    DmcVolume = new wxSlider(settingsPanel, ID_DMC_SLIDER, dmc, 0, 100, wxDefaultPosition, wxDefaultSize, sliderStyle);

    std::ostringstream oss;

    oss << master;
    CurrentMasterVolume = new wxStaticText(settingsPanel, wxID_ANY, oss.str());
    oss.str("");

    oss << pulseOne;
    CurrentPulseOneVolume = new wxStaticText(settingsPanel, wxID_ANY, oss.str());
    oss.str("");

    oss << pulseTwo;
    CurrentPulseTwoVolume = new wxStaticText(settingsPanel, wxID_ANY, oss.str());
    oss.str("");

    oss << triangle;
    CurrentTriangleVolume = new wxStaticText(settingsPanel, wxID_ANY, oss.str());
    oss.str("");
    
    oss << noise;
    CurrentNoiseVolume = new wxStaticText(settingsPanel, wxID_ANY, oss.str());
    oss.str("");

    oss << dmc;
    CurrentDmcVolume = new wxStaticText(settingsPanel, wxID_ANY, oss.str());

    wxSizerFlags sizerFlags;
    sizerFlags.Center();

    masterBox->Add(new wxStaticText(settingsPanel, wxID_ANY, "Master"), sizerFlags);;
    masterBox->Add(MasterVolume, sizerFlags);
    masterBox->Add(CurrentMasterVolume, sizerFlags);

    pulseOneBox->Add(new wxStaticText(settingsPanel, wxID_ANY, "Pulse 1"), sizerFlags);
    pulseOneBox->Add(PulseOneVolume, sizerFlags);
    pulseOneBox->Add(CurrentPulseOneVolume, sizerFlags);
    
    pulseTwoBox->Add(new wxStaticText(settingsPanel, wxID_ANY, "Pulse 2"), sizerFlags);
    pulseTwoBox->Add(PulseTwoVolume, sizerFlags);
    pulseTwoBox->Add(CurrentPulseTwoVolume, sizerFlags);
    
    triangleBox->Add(new wxStaticText(settingsPanel, wxID_ANY, "Triangle"), sizerFlags);
    triangleBox->Add(TriangleVolume, sizerFlags);
    triangleBox->Add(CurrentTriangleVolume, sizerFlags);
    
    noiseBox->Add(new wxStaticText(settingsPanel, wxID_ANY, "Noise"), sizerFlags);
    noiseBox->Add(NoiseVolume, sizerFlags);
    noiseBox->Add(CurrentNoiseVolume, sizerFlags);
    
    dmcBox->Add(new wxStaticText(settingsPanel, wxID_ANY, "DMC"), sizerFlags);
    dmcBox->Add(DmcVolume, sizerFlags);
    dmcBox->Add(CurrentDmcVolume, sizerFlags);

    volumeGrid->Add(masterBox, sizerFlags);
    volumeGrid->Add(pulseOneBox, sizerFlags);
    volumeGrid->Add(pulseTwoBox, sizerFlags);
    volumeGrid->Add(triangleBox, sizerFlags);
    volumeGrid->Add(noiseBox, sizerFlags);
    volumeGrid->Add(dmcBox, sizerFlags);

    wxStaticBoxSizer* channelBox = new wxStaticBoxSizer(wxHORIZONTAL, settingsPanel, "Volume Controls");
    channelBox->Add(volumeGrid, wxSizerFlags().Expand().Border(wxLEFT | wxRIGHT, 10));

    wxBoxSizer* otherSizer = new wxBoxSizer(wxHORIZONTAL);
    
    EnableAudioCheckBox = new wxCheckBox(settingsPanel, ID_AUDIO_ENABLED, "Enable Audio");
    EnableFiltersCheckBox = new wxCheckBox(settingsPanel, ID_FILTERS_ENABLED, "Enable Filters");

    bool audioEnabled, filtersEnabled;
    settings->Read("/Audio/Enabled", &audioEnabled);
    settings->Read("/Audio/FiltersEnabled", &filtersEnabled);

    EnableAudioCheckBox->SetValue(audioEnabled);
    EnableFiltersCheckBox->SetValue(filtersEnabled);

    otherSizer->Add(EnableAudioCheckBox);
    otherSizer->Add(EnableFiltersCheckBox);

    volumeSizer->Add(channelBox, wxSizerFlags().Expand().Border(wxALL, 10));
    volumeSizer->Add(otherSizer, wxSizerFlags().Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM, 10));
    settingsPanel->SetSizer(volumeSizer);

    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(settingsPanel, wxSizerFlags().Expand());
    topSizer->Add(CreateButtonSizer(wxOK | wxCANCEL), wxSizerFlags().Expand().Border(wxALL, 5));

    SetSizerAndFit(topSizer);

    Bind(wxEVT_CLOSE_WINDOW, &AudioSettingsWindow::OnClose, this);
    Bind(wxEVT_BUTTON, &AudioSettingsWindow::OnOk, this, wxID_OK);
    Bind(wxEVT_BUTTON, &AudioSettingsWindow::OnCancel, this, wxID_CANCEL);

    Bind(wxEVT_CHECKBOX, &AudioSettingsWindow::EnableAudioClicked, this, ID_AUDIO_ENABLED);
    Bind(wxEVT_CHECKBOX, &AudioSettingsWindow::EnableFiltersClicked, this, ID_FILTERS_ENABLED);
    Bind(wxEVT_SLIDER, &AudioSettingsWindow::MasterVolumeChanged, this, ID_MASTER_SLIDER);
    Bind(wxEVT_SLIDER, &AudioSettingsWindow::PulseOneVolumeChanged, this, ID_PULSE_ONE_SLIDER);
    Bind(wxEVT_SLIDER, &AudioSettingsWindow::PulseTwoVolumeChanged, this, ID_PULSE_TWO_SLIDER);
    Bind(wxEVT_SLIDER, &AudioSettingsWindow::TriangleVolumeChanged, this, ID_TRIANGLE_SLIDER);
    Bind(wxEVT_SLIDER, &AudioSettingsWindow::NoiseVolumeChanged, this, ID_NOISE_SLIDER);
    Bind(wxEVT_SLIDER, &AudioSettingsWindow::DmcVolumeChanged, this, ID_DMC_SLIDER);
}

void AudioSettingsWindow::SetNes(NES* nes)
{
    Nes = nes;
}

void AudioSettingsWindow::OnClose(wxCloseEvent& event)
{
    wxCommandEvent* evt = new wxCommandEvent(EVT_AUDIO_WINDOW_CLOSED);
    wxQueueEvent(GetParent(), evt);
    Hide();
}

void AudioSettingsWindow::OnOk(wxCommandEvent& event)
{
    AppSettings* settings = AppSettings::GetInstance();

    settings->Write("/Audio/MasterVolume", MasterVolume->GetValue());
    settings->Write("/Audio/PulseOneVolume", PulseOneVolume->GetValue());
    settings->Write("/Audio/PulseTwoVolume", PulseTwoVolume->GetValue());
    settings->Write("/Audio/TriangleVolume", TriangleVolume->GetValue());
    settings->Write("/Audio/NoiseVolume", NoiseVolume->GetValue());
    settings->Write("/Audio/DmcVolume", DmcVolume->GetValue());

    settings->Write("/Audio/Enabled", EnableAudioCheckBox->GetValue());
    settings->Write("/Audio/FiltersEnabled", EnableFiltersCheckBox->GetValue());

    Close();
}

void AudioSettingsWindow::OnCancel(wxCommandEvent& event)
{
    // Reset settings on cancel
    if (Nes != nullptr)
    { 
        AppSettings* settings = AppSettings::GetInstance();

        bool audioEnabled, filtersEnabled;
        settings->Read("/Audio/Enabled", &audioEnabled);
        settings->Read("/Audio/FiltersEnabled", &filtersEnabled);

        Nes->ApuSetMuted(!audioEnabled);
        Nes->ApuSetFiltersEnabled(filtersEnabled);

        int master, pulseOne, pulseTwo, triangle, noise, dmc;
        settings->Read("/Audio/MasterVolume", &master);
        settings->Read("/Audio/PulseOneVolume", &pulseOne);
        settings->Read("/Audio/PulseTwoVolume", &pulseTwo);
        settings->Read("/Audio/TriangleVolume", &triangle);
        settings->Read("/Audio/NoiseVolume", &noise);
        settings->Read("/Audio/DmcVolume", &dmc);

        Nes->ApuSetMasterVolume(master / 100.0f);
        Nes->ApuSetPulseOneVolume(pulseOne / 100.0f);
        Nes->ApuSetPulseTwoVolume(pulseTwo / 100.0f);
        Nes->ApuSetTriangleVolume(triangle / 100.0f);
        Nes->ApuSetNoiseVolume(noise / 100.0f);
        Nes->ApuSetDmcVolume(dmc / 100.0f);
    }

    Close();
}

void AudioSettingsWindow::EnableAudioClicked(wxCommandEvent& event)
{
    if (Nes != nullptr)
    {
        Nes->ApuSetMuted(!EnableAudioCheckBox->GetValue());
    }
}

void AudioSettingsWindow::EnableFiltersClicked(wxCommandEvent& event)
{
    if (Nes != nullptr)
    {
        Nes->ApuSetFiltersEnabled(EnableFiltersCheckBox->GetValue());
    }
}

void AudioSettingsWindow::MasterVolumeChanged(wxCommandEvent& event)
{
    if (Nes != nullptr)
    {
        Nes->ApuSetMasterVolume(MasterVolume->GetValue() / 100.0f);
    }

    std::ostringstream oss;
    oss << MasterVolume->GetValue();
    CurrentMasterVolume->SetLabel(oss.str());
}

void AudioSettingsWindow::PulseOneVolumeChanged(wxCommandEvent& event)
{
    if (Nes != nullptr)
    {
        Nes->ApuSetPulseOneVolume(PulseOneVolume->GetValue() / 100.0f);
    }

    std::ostringstream oss;
    oss << PulseOneVolume->GetValue();
    CurrentPulseOneVolume->SetLabel(oss.str());
}

void AudioSettingsWindow::PulseTwoVolumeChanged(wxCommandEvent& event)
{
    if (Nes != nullptr)
    {
        Nes->ApuSetPulseTwoVolume(PulseTwoVolume->GetValue() / 100.0f);
    }

    std::ostringstream oss;
    oss << PulseTwoVolume->GetValue();
    CurrentPulseTwoVolume->SetLabel(oss.str());
}

void AudioSettingsWindow::TriangleVolumeChanged(wxCommandEvent& event)
{
    if (Nes != nullptr)
    {
        Nes->ApuSetTriangleVolume(TriangleVolume->GetValue() / 100.0f);
    }

    std::ostringstream oss;
    oss << TriangleVolume->GetValue();
    CurrentTriangleVolume->SetLabel(oss.str());
}

void AudioSettingsWindow::NoiseVolumeChanged(wxCommandEvent& event)
{
    if (Nes != nullptr)
    {
        Nes->ApuSetNoiseVolume(NoiseVolume->GetValue() / 100.0f);
    }

    std::ostringstream oss;
    oss << NoiseVolume->GetValue();
    CurrentNoiseVolume->SetLabel(oss.str());
}

void AudioSettingsWindow::DmcVolumeChanged(wxCommandEvent& event)
{
    if (Nes != nullptr)
    {
        Nes->ApuSetDmcVolume(DmcVolume->GetValue() / 100.0f);
    }

    std::ostringstream oss;
    oss << DmcVolume->GetValue();
    CurrentDmcVolume->SetLabel(oss.str());
}
