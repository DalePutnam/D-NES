#include "game_list.h"
#include "boost/filesystem.hpp"
#include "utilities/app_settings.h"

void GameList::OnSize(wxSizeEvent& event)
{
    int windowWidth = GetVirtualSize().GetX();
    int columnWidth = GetColumnWidth(1);
    SetColumnWidth(0, windowWidth - columnWidth);

    event.Skip();
}

GameList::GameList(wxWindow* parent) :
    wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL)
{
    AppendColumn("File Name");
    AppendColumn("File Size");

    Bind(wxEVT_SIZE, wxSizeEventHandler(GameList::OnSize), this, wxID_ANY);
}

void GameList::PopulateList()
{
    namespace fs = boost::filesystem;

    AppSettings* settings = AppSettings::getInstance();
    fs::path filePath(settings->get<std::string>("frontend.rompath")); // Get path from settings

    DeleteAllItems();

    if (fs::exists(filePath))
    {
        if (fs::is_directory(filePath))
        {
            for (fs::directory_iterator it(filePath); it != fs::directory_iterator(); ++it)
            {
                // If the file is a regular file and it has a .nes extension, add it to the list
                if (fs::is_regular_file(it->path())
                    && it->path().has_extension()
                    && std::string(".nes").compare(it->path().extension().string()) == 0)
                {
                    int index = InsertItem(GetItemCount(), it->path().filename().string());

                    // Get file size in Kibibytes
                    std::ostringstream oss;
                    oss << file_size(it->path()) / 1024 << " KiB";

                    SetItem(index, 1, oss.str());
                }
            }
        }
    }
}
