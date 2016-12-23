#include <sstream>
#include <wx/dir.h>
#include <wx/arrstr.h>
#include <wx/filename.h>

#include "game_list.h"
#include "utilities/app_settings.h"

void GameList::OnSize(wxSizeEvent& event)
{
    int windowWidth = GetVirtualSize().GetX();
    int columnWidth = GetColumnWidth(1);
    SetColumnWidth(0, windowWidth - columnWidth);

    event.Skip();
}

GameList::GameList(wxWindow* parent)
    : wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL)
{
    AppendColumn("File Name");
    AppendColumn("File Size");

    Bind(wxEVT_SIZE, wxSizeEventHandler(GameList::OnSize), this, wxID_ANY);
}

void GameList::PopulateList()
{
    AppSettings* settings = AppSettings::GetInstance();

    wxString romPath;
    settings->Read("/Paths/RomPath", &romPath);

    DeleteAllItems();

    if (wxDir::Exists(romPath))
    {
        wxArrayString romList;
        wxDir::GetAllFiles(romPath, &romList, "*.nes", wxDIR_FILES);

        for (const wxString& romName : romList)
        {
            wxFileName rom(romName);

            int index = InsertItem(GetItemCount(), rom.GetFullName());

            // Get file size in Kibibytes
            std::ostringstream oss;
            oss << rom.GetSize() / 1024 << " KiB";

            SetItem(index, 1, oss.str());
        }
    }
}
