#pragma once

#include <wx/window.h>
#include <wx/listctrl.h>

class GameList : public wxListCtrl
{
    void OnSize(wxSizeEvent& event);

public:

    GameList(wxWindow* parent);

    void PopulateList();
};
