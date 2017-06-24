#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <wx/slider.h>

#include "main_window.h"
#include "audio_settings_window.h"
#include "utilities/app_settings.h"

#include "nes.h"

wxDEFINE_EVENT(EVT_AUDIO_WINDOW_CLOSED, wxCommandEvent);

AudioSettingsWindow::AudioSettingsWindow(MainWindow* parent)
    : SettingsWindowBase(parent, "Audio Settings")
{
    InitializeLayout();
    BindEvents();
}

void AudioSettingsWindow::InitializeLayout()
{
    AppSettings* settings = AppSettings::GetInstance();

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

    wxSize sliderSize(-1, 100);

    MasterVolume = new wxSlider(SettingsPanel, ID_MASTER_SLIDER, master, 0, 100, wxDefaultPosition, sliderSize, sliderStyle);
    PulseOneVolume = new wxSlider(SettingsPanel, ID_PULSE_ONE_SLIDER, pulseOne, 0, 100, wxDefaultPosition, sliderSize, sliderStyle);
    PulseTwoVolume = new wxSlider(SettingsPanel, ID_PULSE_TWO_SLIDER, pulseTwo, 0, 100, wxDefaultPosition, sliderSize, sliderStyle);
    TriangleVolume = new wxSlider(SettingsPanel, ID_TRIANGLE_SLIDER, triangle, 0, 100, wxDefaultPosition, sliderSize, sliderStyle);
    NoiseVolume = new wxSlider(SettingsPanel, ID_NOISE_SLIDER, noise, 0, 100, wxDefaultPosition, sliderSize, sliderStyle);
    DmcVolume = new wxSlider(SettingsPanel, ID_DMC_SLIDER, dmc, 0, 100, wxDefaultPosition, sliderSize, sliderStyle);

    CurrentMasterVolume = new wxStaticText(SettingsPanel, wxID_ANY, std::to_string(master));
    CurrentPulseOneVolume = new wxStaticText(SettingsPanel, wxID_ANY, std::to_string(pulseOne));
    CurrentPulseTwoVolume = new wxStaticText(SettingsPanel, wxID_ANY, std::to_string(pulseTwo));
    CurrentTriangleVolume = new wxStaticText(SettingsPanel, wxID_ANY, std::to_string(triangle));
    CurrentNoiseVolume = new wxStaticText(SettingsPanel, wxID_ANY, std::to_string(noise));
    CurrentDmcVolume = new wxStaticText(SettingsPanel, wxID_ANY, std::to_string(dmc));

    wxSizerFlags sizerFlags;
    sizerFlags.Center();

    masterBox->Add(new wxStaticText(SettingsPanel, wxID_ANY, "Master"), sizerFlags);;
    masterBox->Add(MasterVolume, sizerFlags);
    masterBox->Add(CurrentMasterVolume, sizerFlags);

    pulseOneBox->Add(new wxStaticText(SettingsPanel, wxID_ANY, "Pulse 1"), sizerFlags);
    pulseOneBox->Add(PulseOneVolume, sizerFlags);
    pulseOneBox->Add(CurrentPulseOneVolume, sizerFlags);

    pulseTwoBox->Add(new wxStaticText(SettingsPanel, wxID_ANY, "Pulse 2"), sizerFlags);
    pulseTwoBox->Add(PulseTwoVolume, sizerFlags);
    pulseTwoBox->Add(CurrentPulseTwoVolume, sizerFlags);

    triangleBox->Add(new wxStaticText(SettingsPanel, wxID_ANY, "Triangle"), sizerFlags);
    triangleBox->Add(TriangleVolume, sizerFlags);
    triangleBox->Add(CurrentTriangleVolume, sizerFlags);

    noiseBox->Add(new wxStaticText(SettingsPanel, wxID_ANY, "Noise"), sizerFlags);
    noiseBox->Add(NoiseVolume, sizerFlags);
    noiseBox->Add(CurrentNoiseVolume, sizerFlags);

    dmcBox->Add(new wxStaticText(SettingsPanel, wxID_ANY, "DMC"), sizerFlags);
    dmcBox->Add(DmcVolume, sizerFlags);
    dmcBox->Add(CurrentDmcVolume, sizerFlags);

    volumeGrid->Add(masterBox, sizerFlags);
    volumeGrid->Add(pulseOneBox, sizerFlags);
    volumeGrid->Add(pulseTwoBox, sizerFlags);
    volumeGrid->Add(triangleBox, sizerFlags);
    volumeGrid->Add(noiseBox, sizerFlags);
    volumeGrid->Add(dmcBox, sizerFlags);

    wxStaticBoxSizer* channelBox = new wxStaticBoxSizer(wxHORIZONTAL, SettingsPanel, "Volume Controls");
    channelBox->Add(volumeGrid, wxSizerFlags().Expand().Border(wxLEFT | wxRIGHT, 10));

    wxBoxSizer* otherSizer = new wxBoxSizer(wxHORIZONTAL);

    EnableAudioCheckBox = new wxCheckBox(SettingsPanel, ID_AUDIO_ENABLED, "Enable Audio");
    EnableFiltersCheckBox = new wxCheckBox(SettingsPanel, ID_FILTERS_ENABLED, "Enable Filters");

    bool audioEnabled, filtersEnabled;
    settings->Read("/Audio/Enabled", &audioEnabled);
    settings->Read("/Audio/FiltersEnabled", &filtersEnabled);

    EnableAudioCheckBox->SetValue(audioEnabled);
    EnableFiltersCheckBox->SetValue(filtersEnabled);

    otherSizer->Add(EnableAudioCheckBox);
    otherSizer->Add(EnableFiltersCheckBox);

    volumeSizer->Add(channelBox, wxSizerFlags().Expand().Border(wxALL, 10));
    volumeSizer->Add(otherSizer, wxSizerFlags().Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM, 10));
    SettingsPanel->SetSizer(volumeSizer);

    Fit();
}

void AudioSettingsWindow::BindEvents()
{
    Bind(wxEVT_CHECKBOX, &AudioSettingsWindow::EnableAudioClicked, this, ID_AUDIO_ENABLED);
    Bind(wxEVT_CHECKBOX, &AudioSettingsWindow::EnableFiltersClicked, this, ID_FILTERS_ENABLED);
    Bind(wxEVT_SLIDER, &AudioSettingsWindow::MasterVolumeChanged, this, ID_MASTER_SLIDER);
    Bind(wxEVT_SLIDER, &AudioSettingsWindow::PulseOneVolumeChanged, this, ID_PULSE_ONE_SLIDER);
    Bind(wxEVT_SLIDER, &AudioSettingsWindow::PulseTwoVolumeChanged, this, ID_PULSE_TWO_SLIDER);
    Bind(wxEVT_SLIDER, &AudioSettingsWindow::TriangleVolumeChanged, this, ID_TRIANGLE_SLIDER);
    Bind(wxEVT_SLIDER, &AudioSettingsWindow::NoiseVolumeChanged, this, ID_NOISE_SLIDER);
    Bind(wxEVT_SLIDER, &AudioSettingsWindow::DmcVolumeChanged, this, ID_DMC_SLIDER);
}

void AudioSettingsWindow::DoClose()
{
    wxCommandEvent* evt = new wxCommandEvent(EVT_AUDIO_WINDOW_CLOSED);
    wxQueueEvent(GetParent(), evt);
    Hide();
}

void AudioSettingsWindow::DoOk()
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

void AudioSettingsWindow::DoCancel()
{
    // Reset settings on cancel
    if (Nes != nullptr)
    {
        AppSettings* settings = AppSettings::GetInstance();

        bool audioEnabled, filtersEnabled;
        settings->Read("/Audio/Enabled", &audioEnabled);
        settings->Read("/Audio/FiltersEnabled", &filtersEnabled);

        Nes->ApuSetAudioEnabled(audioEnabled);
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
        Nes->ApuSetAudioEnabled(EnableAudioCheckBox->GetValue());
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

    CurrentMasterVolume->SetLabel(std::to_string(MasterVolume->GetValue()));
}

void AudioSettingsWindow::PulseOneVolumeChanged(wxCommandEvent& event)
{
    if (Nes != nullptr)
    {
        Nes->ApuSetPulseOneVolume(PulseOneVolume->GetValue() / 100.0f);
    }

    CurrentPulseOneVolume->SetLabel(std::to_string(PulseOneVolume->GetValue()));
}

void AudioSettingsWindow::PulseTwoVolumeChanged(wxCommandEvent& event)
{
    if (Nes != nullptr)
    {
        Nes->ApuSetPulseTwoVolume(PulseTwoVolume->GetValue() / 100.0f);
    }

    CurrentPulseTwoVolume->SetLabel(std::to_string(PulseTwoVolume->GetValue()));
}

void AudioSettingsWindow::TriangleVolumeChanged(wxCommandEvent& event)
{
    if (Nes != nullptr)
    {
        Nes->ApuSetTriangleVolume(TriangleVolume->GetValue() / 100.0f);
    }

    CurrentTriangleVolume->SetLabel(std::to_string(TriangleVolume->GetValue()));
}

void AudioSettingsWindow::NoiseVolumeChanged(wxCommandEvent& event)
{
    if (Nes != nullptr)
    {
        Nes->ApuSetNoiseVolume(NoiseVolume->GetValue() / 100.0f);
    }

    CurrentNoiseVolume->SetLabel(std::to_string(NoiseVolume->GetValue()));
}

void AudioSettingsWindow::DmcVolumeChanged(wxCommandEvent& event)
{
    if (Nes != nullptr)
    {
        Nes->ApuSetDmcVolume(DmcVolume->GetValue() / 100.0f);
    }

    CurrentDmcVolume->SetLabel(std::to_string(DmcVolume->GetValue()));
}
