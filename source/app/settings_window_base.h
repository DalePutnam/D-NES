#pragma once

#include <memory>
#include <wx/dialog.h>

#include <dnes/dnes.h>

#include "utilities/nes_ptr.h"

class wxPanel;
class MainWindow;

class SettingsWindowBase : public wxDialog
{
public:
    SettingsWindowBase(MainWindow* parent, NESPtr& nes, const std::string& title);
    virtual ~SettingsWindowBase() = default;

protected:
    virtual void DoOk() = 0;
    virtual void DoCancel() = 0;
    virtual void DoClose() = 0;

    NESPtr& Nes;
    wxPanel* SettingsPanel;

private:
    void OnOk(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
};
