/* All Settings window */

#pragma once

#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/notebook.h>

class SettingsWindow : public wxDialog
{
public:
    SettingsWindow();
    void SaveSettings();

private:
    wxNotebook* Notebook;
    wxTextCtrl* RomDirectory;
    wxButton* DirectorySelect;
    wxButton* OkButton;
    wxButton* CancelButton;

    void PopulateFields();
    void OnDirectorySelect(wxCommandEvent& event);;
};

const int ID_DIR_SELECT = 200;
