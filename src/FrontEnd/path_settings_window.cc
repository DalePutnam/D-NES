#include <wx/sizer.h>
#include <wx/dirdlg.h>
#include <wx/stattext.h>

#include "path_settings_window.h"
#include "utilities/app_settings.h"

void PathSettingsWindow::PopulateFields()
{
    AppSettings* settings = AppSettings::GetInstance();
    wxString romPath;
    settings->Read("/Paths/RomPath", &romPath);

    *RomDirectory << romPath;
}

void PathSettingsWindow::OnDirectorySelect(wxCommandEvent& WXUNUSED(event))
{
    wxDirDialog dialog(this, "Choose ROM Path", RomDirectory->GetLineText(0));

    if (dialog.ShowModal() == wxID_OK)
    {
        RomDirectory->Clear();
        *RomDirectory << dialog.GetPath();
    }
}

PathSettingsWindow::PathSettingsWindow()
    : wxDialog(NULL, wxID_ANY, "Settings", wxDefaultPosition, wxDefaultSize)
{
    Notebook = new wxNotebook(this, wxID_ANY, wxPoint(-1, -1), wxSize(-1, -1), wxNB_TOP);
    wxWindow* page1 = new wxWindow(Notebook, wxID_ANY);
    page1->SetBackgroundColour(*wxWHITE);
    wxBoxSizer* hbox0 = new wxBoxSizer(wxHORIZONTAL);
    page1->SetSizer(hbox0);
    Notebook->AddPage(page1, "Basic Settings");

    wxStaticText* label0 = new wxStaticText(page1, wxID_ANY, "ROM Path:");
    RomDirectory = new wxTextCtrl(page1, wxID_ANY, "", wxDefaultPosition, wxSize(150, 24));
    DirectorySelect = new wxButton(page1, ID_DIR_SELECT, "...", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    hbox0->Add(label0, 0, wxALIGN_CENTER_VERTICAL);
    hbox0->Add(RomDirectory, 1, wxALIGN_CENTER_VERTICAL);
    hbox0->Add(DirectorySelect, 0, wxALIGN_CENTER_VERTICAL);

    OkButton = new wxButton(this, wxID_OK, "OK");
    CancelButton = new wxButton(this, wxID_CANCEL, "Cancel");

    wxBoxSizer* hbox1 = new wxBoxSizer(wxHORIZONTAL);
    hbox1->Add(OkButton, 0, wxALIGN_CENTER_VERTICAL);
    hbox1->Add(CancelButton, 0, wxALIGN_CENTER_VERTICAL);

    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    vbox->Add(Notebook, 1, wxEXPAND | wxALL);
    vbox->Add(hbox1, 0, wxALIGN_RIGHT);

    SetSizer(vbox);
    Fit();

    PopulateFields();

    Connect(ID_DIR_SELECT, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(PathSettingsWindow::OnDirectorySelect));
}

void PathSettingsWindow::SaveSettings()
{
    AppSettings* settings = AppSettings::GetInstance();
    settings->Write("/Paths/RomPath", RomDirectory->GetLineText(0));
    settings->Save();
}
