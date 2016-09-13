#ifndef GAME_LIST_H_
#define GAME_LIST_H_

#include <wx/window.h>
#include <wx/listctrl.h>

class GameList : public wxListCtrl
{
    void OnSize(wxSizeEvent& event);

public:

    GameList(wxWindow* parent);
    
    void PopulateList();
};

#endif
