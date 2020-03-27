#pragma once

#include "utilities/nes_ptr.h"
#include "settings_window_base.h"

class MainWindow;
class wxTextCtrl;
class wxButton;

wxDECLARE_EVENT(EVT_PATH_WINDOW_CLOSED, wxCommandEvent);

class PathSettingsWindow : public SettingsWindowBase
{
public:
    PathSettingsWindow(MainWindow* parent, NESPtr& nes);
    void SaveSettings();

protected:
    virtual void DoOk() override;
    virtual void DoCancel() override;
    virtual void DoClose() override;

private:
    void InitializeLayout();
    void BindEvents();

    wxTextCtrl* RomPathText;
    wxButton* RomPathButton;
    wxTextCtrl* SavePathText;
    wxButton* SavePathButton;
    wxTextCtrl* StatePathText;
    wxButton* StatePathButton;

    void OnDirectorySelect(wxCommandEvent& event);;
};

const int ID_ROM_PATH_SELECT = 200;
const int ID_SAVE_PATH_SELECT = 201;
const int ID_STATE_PATH_SELECT = 202;
