#include <wx/dir.h>
#include <wx/sizer.h>
#include <wx/dirdlg.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/artprov.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>

#include "main_window.h"
#include "path_settings_window.h"
#include "utilities/app_settings.h"

#include "nes.h"

wxDEFINE_EVENT(EVT_PATH_WINDOW_CLOSED, wxCommandEvent);

void PathSettingsWindow::OnDirectorySelect(wxCommandEvent& event)
{
    std::string dialogLabel, defaultPath;

    switch (event.GetId())
    {
    case ID_ROM_PATH_SELECT:
        dialogLabel = "Choose ROM Path";
        defaultPath = RomPathText->GetValue();
        break;
    case ID_SAVE_PATH_SELECT:
        dialogLabel = "Choose Native Save Path";
        defaultPath = SavePathText->GetValue();
        break;
    case ID_STATE_PATH_SELECT:
        dialogLabel = "Choose State Save Path";
        defaultPath = StatePathText->GetValue();
        break;
    default:
        return;
    }

    wxDirDialog dialog(this, dialogLabel, defaultPath);

    if (dialog.ShowModal() == wxID_OK)
    {
        switch (event.GetId())
        {
        case ID_ROM_PATH_SELECT:
            RomPathText->SetValue(dialog.GetPath());
            break;
        case ID_SAVE_PATH_SELECT:
            SavePathText->SetValue(dialog.GetPath());
            break;
        case ID_STATE_PATH_SELECT:
            StatePathText->SetValue(dialog.GetPath());
            break;
        default:
            return;
        }
    }
}

PathSettingsWindow::PathSettingsWindow(MainWindow* parent, std::unique_ptr<NES>& nes)
    : SettingsWindowBase(parent, nes, "Path Settings")
{
    InitializeLayout();
    BindEvents();
}

void PathSettingsWindow::InitializeLayout()
{
#ifdef _WIN32
    RomPathButton = new wxButton(SettingsPanel, ID_ROM_PATH_SELECT, "...", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    SavePathButton = new wxButton(SettingsPanel, ID_SAVE_PATH_SELECT, "...", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    StatePathButton = new wxButton(SettingsPanel, ID_STATE_PATH_SELECT, "...", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
#elif __linux
    wxBitmap folderBitmap = wxArtProvider::GetBitmap(wxART_FOLDER);
    RomPathButton = new wxBitmapButton(SettingsPanel, ID_ROM_PATH_SELECT, folderBitmap, wxDefaultPosition, wxSize(-1, 24));
    SavePathButton = new wxBitmapButton(SettingsPanel, ID_SAVE_PATH_SELECT, folderBitmap, wxDefaultPosition, wxSize(-1, 24));
    StatePathButton = new wxBitmapButton(SettingsPanel, ID_STATE_PATH_SELECT, folderBitmap, wxDefaultPosition, wxSize(-1, 24));
#endif

    wxStaticText* romPathLabel = new wxStaticText(SettingsPanel, wxID_ANY, "ROM Path");
    RomPathText = new wxTextCtrl(SettingsPanel, wxID_ANY, "", wxDefaultPosition, wxSize(200, 24));

    wxBoxSizer* romSizer = new wxBoxSizer(wxHORIZONTAL);
    romSizer->Add(RomPathText, wxSizerFlags().Border(wxRIGHT, 2).Align(wxALIGN_CENTER_VERTICAL));
    romSizer->Add(RomPathButton, wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));

    wxStaticText* savePathLabel = new wxStaticText(SettingsPanel, wxID_ANY, "Native Save Path");
    SavePathText = new wxTextCtrl(SettingsPanel, wxID_ANY, "", wxDefaultPosition, wxSize(200, 24));

    wxBoxSizer* saveSizer = new wxBoxSizer(wxHORIZONTAL);
    saveSizer->Add(SavePathText, wxSizerFlags().Border(wxRIGHT, 2).Align(wxALIGN_CENTER_VERTICAL));
    saveSizer->Add(SavePathButton, wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));

    wxStaticText* statePathLabel = new wxStaticText(SettingsPanel, wxID_ANY, "State Save Path");
    StatePathText = new wxTextCtrl(SettingsPanel, wxID_ANY, "", wxDefaultPosition, wxSize(200, 24));

    wxBoxSizer* stateSizer = new wxBoxSizer(wxHORIZONTAL);
    stateSizer->Add(StatePathText, wxSizerFlags().Border(wxRIGHT, 2).Align(wxALIGN_CENTER_VERTICAL));
    stateSizer->Add(StatePathButton, wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));

    wxBoxSizer* settingsSizer = new wxBoxSizer(wxVERTICAL);
    settingsSizer->Add(romPathLabel, wxSizerFlags().Border(wxLEFT | wxRIGHT | wxTOP, 5));
    settingsSizer->Add(romSizer, wxSizerFlags().Border(wxLEFT | wxRIGHT, 5));
    settingsSizer->Add(savePathLabel, wxSizerFlags().Border(wxLEFT | wxRIGHT | wxTOP, 5));
    settingsSizer->Add(saveSizer, wxSizerFlags().Border(wxLEFT | wxRIGHT | wxBOTTOM, 5));
    settingsSizer->Add(statePathLabel, wxSizerFlags().Border(wxLEFT | wxRIGHT | wxTOP, 5));
    settingsSizer->Add(stateSizer, wxSizerFlags().Border(wxLEFT | wxRIGHT | wxBOTTOM, 5));

    SettingsPanel->SetSizer(settingsSizer);

    Fit();

    AppSettings& settings = AppSettings::GetInstance();
    std::string romPath, nativeSavePath, stateSavePath;

    settings.Read("/Paths/RomPath", &romPath);
    settings.Read("/Paths/NativeSavePath", &nativeSavePath);
    settings.Read("/Paths/StateSavePath", &stateSavePath);

    RomPathText->SetValue(romPath);
    SavePathText->SetValue(nativeSavePath);
    StatePathText->SetValue(stateSavePath);
}

void PathSettingsWindow::BindEvents()
{
    Bind(wxEVT_COMMAND_BUTTON_CLICKED, &PathSettingsWindow::OnDirectorySelect, this, ID_ROM_PATH_SELECT);
    Bind(wxEVT_COMMAND_BUTTON_CLICKED, &PathSettingsWindow::OnDirectorySelect, this, ID_SAVE_PATH_SELECT);
    Bind(wxEVT_COMMAND_BUTTON_CLICKED, &PathSettingsWindow::OnDirectorySelect, this, ID_STATE_PATH_SELECT);
}

void PathSettingsWindow::DoOk()
{
    AppSettings& settings = AppSettings::GetInstance();
    settings.Write("/Paths/RomPath", RomPathText->GetValue());
    settings.Write("/Paths/NativeSavePath", SavePathText->GetValue());
    settings.Write("/Paths/StateSavePath", StatePathText->GetValue());

    std::string newSavePath = SavePathText->GetValue().ToStdString();
    std::string newStatePath = StatePathText->GetValue().ToStdString();

    if (!wxDir::Exists(newSavePath))
    {
        wxDir::Make(newSavePath, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    }

    if (!wxDir::Exists(newStatePath))
    {
        wxDir::Make(newStatePath);
    }

    if (Nes != nullptr)
    {
        Nes->SetNativeSaveDirectory(newSavePath);
    }

    Close();
}

void PathSettingsWindow::DoCancel()
{
    Close();
}

void PathSettingsWindow::DoClose()
{
    wxCommandEvent* evt = new wxCommandEvent(EVT_PATH_WINDOW_CLOSED);
    wxQueueEvent(GetParent(), evt);
    Hide();
}
