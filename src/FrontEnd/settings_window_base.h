#pragma once

#include <wx/dialog.h>

class NES;
class wxPanel;
class MainWindow;

class SettingsWindowBase : public wxDialog
{
public:
    SettingsWindowBase(MainWindow* parent, const std::string& title);
    void SetNes(NES* nes);

protected:
    virtual void DoOk() = 0;
    virtual void DoCancel() = 0;
    virtual void DoClose() = 0;

    NES* Nes;
    wxPanel* SettingsPanel;

private:
    void OnOk(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
};
