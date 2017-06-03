#include <wx/panel.h>
#include <wx/sizer.h>

#include "main_window.h"
#include "settings_window_base.h"
#include "nes.h"

static constexpr long DIALOG_STYLE = wxDEFAULT_DIALOG_STYLE & ~wxRESIZE_BORDER & ~wxMAXIMIZE_BOX & ~wxMINIMIZE_BOX & ~wxCLOSE_BOX;

SettingsWindowBase::SettingsWindowBase(MainWindow* parent, const std::string& title)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, DIALOG_STYLE)
    , Nes(nullptr)
{
    SettingsPanel = new wxPanel(this);

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(SettingsPanel, wxSizerFlags().Expand());
    mainSizer->Add(CreateButtonSizer(wxOK | wxCANCEL), wxSizerFlags().Expand().Border(wxALL, 5));

    SetSizer(mainSizer);

    Bind(wxEVT_BUTTON, &SettingsWindowBase::OnOk, this, wxID_OK);
    Bind(wxEVT_BUTTON, &SettingsWindowBase::OnCancel, this, wxID_CANCEL);
}

void SettingsWindowBase::SetNes(NES* nes)
{
    Nes = nes;
}

void SettingsWindowBase::OnOk(wxCommandEvent& WXUNUSED(event))
{
    Ok();
}

void SettingsWindowBase::OnCancel(wxCommandEvent& WXUNUSED(event))
{
    Cancel();
}
