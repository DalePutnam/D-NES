#pragma once

#include <memory>
#include <wx/dialog.h>

class NES;
class wxPanel;
class MainWindow;

class SettingsWindowBase : public wxDialog
{
public:
    SettingsWindowBase(MainWindow* parent, std::unique_ptr<NES>& nes, const std::string& title);

protected:
    virtual void DoOk() = 0;
    virtual void DoCancel() = 0;
    virtual void DoClose() = 0;

    std::unique_ptr<NES>& Nes;
    wxPanel* SettingsPanel;

private:
    void OnOk(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
};
