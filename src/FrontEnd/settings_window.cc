#include <wx/sizer.h>
#include <wx/dirdlg.h>
#include <wx/stattext.h>

#include "settings_window.h"
#include "utilities/app_settings.h"

void SettingsWindow::PopulateFields()
{
    AppSettings* settings = AppSettings::GetInstance();
	wxString romPath;
	settings->Read<wxString>("ROMPath", &romPath, "");

	*romDirectory << romPath;
}

void SettingsWindow::OnDirectorySelect(wxCommandEvent& WXUNUSED(event))
{
    wxDirDialog dialog(NULL, "Choose ROM Path", romDirectory->GetLineText(0));

    if (dialog.ShowModal() == wxID_OK)
    {
        romDirectory->Clear();
        *romDirectory << dialog.GetPath();
    }
}

SettingsWindow::SettingsWindow()
    : wxDialog(NULL, wxID_ANY, "Settings", wxDefaultPosition, wxDefaultSize)
{
    notebook = new wxNotebook(this, wxID_ANY, wxPoint(-1, -1), wxSize(-1, -1), wxNB_TOP);
    wxWindow* page1 = new wxWindow(notebook, wxID_ANY);
    page1->SetBackgroundColour(*wxWHITE);
    wxBoxSizer* hbox0 = new wxBoxSizer(wxHORIZONTAL);
    page1->SetSizer(hbox0);
    notebook->AddPage(page1, "Basic Settings");

    wxStaticText* label0 = new wxStaticText(page1, wxID_ANY, "ROM Path:");
    romDirectory = new wxTextCtrl(page1, wxID_ANY, "", wxDefaultPosition, wxSize(150, 24));
    directorySelect = new wxButton(page1, ID_DIR_SELECT, "...", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    hbox0->Add(label0, 0, wxALIGN_CENTER_VERTICAL);
    hbox0->Add(romDirectory, 1, wxALIGN_CENTER_VERTICAL);
    hbox0->Add(directorySelect, 0, wxALIGN_CENTER_VERTICAL);

    ok = new wxButton(this, wxID_OK, "OK");
    cancel = new wxButton(this, wxID_CANCEL, "Cancel");

    wxBoxSizer* hbox1 = new wxBoxSizer(wxHORIZONTAL);
    hbox1->Add(ok, 0, wxALIGN_CENTER_VERTICAL);
    hbox1->Add(cancel, 0, wxALIGN_CENTER_VERTICAL);

    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    vbox->Add(notebook, 1, wxEXPAND | wxALL);
    vbox->Add(hbox1, 0, wxALIGN_RIGHT);

    SetSizer(vbox);
    Fit();

    PopulateFields();

    Connect(ID_DIR_SELECT, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(SettingsWindow::OnDirectorySelect));
}

void SettingsWindow::SaveSettings()
{
    AppSettings* settings = AppSettings::GetInstance();
    settings->Write<wxString>("ROMPath", romDirectory->GetLineText(0));
    settings->Save();
}