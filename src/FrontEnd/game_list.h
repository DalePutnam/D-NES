#pragma once

#include <wx/window.h>
#include <wx/listctrl.h>

class GameList : public wxListCtrl
{
public:
    GameList(wxWindow* parent);
    void PopulateList();

private:
    void OnSize(wxSizeEvent& event);
};
