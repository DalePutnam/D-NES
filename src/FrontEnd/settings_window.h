/* All Settings window */

#ifndef SETTINGS_WINDOW_H_
#define SETTINGS_WINDOW_H_

#include "wx/dialog.h"
#include "wx/textctrl.h"
#include "wx/button.h"
#include "wx/notebook.h"

class SettingsWindow : public wxDialog
{
    wxNotebook* notebook;
    wxTextCtrl* romDirectory;
    wxButton* directorySelect;
    wxButton* ok;
    wxButton* cancel;

    void PopulateFields();
    void OnDirectorySelect(wxCommandEvent& event);;

public:
    SettingsWindow();

    void SaveSettings();
};

const int ID_DIR_SELECT = 200;

#endif