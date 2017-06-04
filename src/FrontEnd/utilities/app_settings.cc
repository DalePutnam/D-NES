/*
 * app_settings.cc
 *
 *  Created on: Sep 11, 2014
 *      Author: Dale
 */

#include <fstream>
#include <wx/filename.h>
#include <wx/wfstream.h>

#include "app_settings.h"

AppSettings* AppSettings::instance = nullptr; // Initialize static instance field

AppSettings::AppSettings()
{
    wxFileName configFile(wxGetCwd(), "config.txt");

    if (configFile.FileExists())
    {
        wxFileInputStream configStream(configFile.GetFullPath());
        Settings = new wxFileConfig(configStream);
    }
    else
    {
        Settings = new wxFileConfig();
    }

    if (!Settings->HasEntry("/Paths/RomPath"))
    {
        Settings->Write("/Paths/RomPath", wxGetCwd());
    }

    if (!Settings->HasEntry("/Paths/NativeSavePath"))
    {
        wxFileName file(wxGetCwd() + "/saves", "native");
        Settings->Write("/Paths/NativeSavePath", file.GetFullPath());
    }

    if (!Settings->HasEntry("/Paths/StateSavePath"))
    {
        wxFileName file(wxGetCwd() + "/saves", "state");
        Settings->Write("/Paths/StateSavePath", file.GetFullPath());
    }

    if (!Settings->HasEntry("/Audio/Enabled"))
    {
        Settings->Write("/Audio/Enabled", true);
    }

    if (!Settings->HasEntry("/Audio/FiltersEnabled"))
    {
        Settings->Write("/Audio/FiltersEnabled", false);
    }

    if (!Settings->HasEntry("/Audio/MasterVolume"))
    {
        Settings->Write("/Audio/MasterVolume", 100);
    }

    if (!Settings->HasEntry("/Audio/PulseOneVolume"))
    {
        Settings->Write("/Audio/PulseOneVolume", 100);
    }

    if (!Settings->HasEntry("/Audio/PulseTwoVolume"))
    {
        Settings->Write("/Audio/PulseTwoVolume", 100);
    }

    if (!Settings->HasEntry("/Audio/TriangleVolume"))
    {
        Settings->Write("/Audio/TriangleVolume", 100);
    }

    if (!Settings->HasEntry("/Audio/NoiseVolume"))
    {
        Settings->Write("/Audio/NoiseVolume", 100);
    }

    if (!Settings->HasEntry("/Audio/DmcVolume"))
    {
        Settings->Write("/Audio/DmcVolume", 100);
    }

    if (!Settings->HasEntry("/Video/Resolution"))
    {
        Settings->Write("/Video/Resolution", 0);
    }

    if (!Settings->HasEntry("/Video/NtscDecoding"))
    {
        Settings->Write("/Video/NtscDecoding", false);
    }

    if (!Settings->HasEntry("/Video/Overscan"))
    {
        Settings->Write("/Video/Overscan", true);
    }

    if (!Settings->HasEntry("/Video/ShowFps"))
    {
        Settings->Write("/Video/ShowFps", false);
    }

    if (!Settings->HasEntry("/Menu/Width"))
    {
        Settings->Write("/Menu/Width", 600);
    }

    if (!Settings->HasEntry("/Menu/Height"))
    {
        Settings->Write("/Menu/Height", 460);
    }
}

AppSettings::~AppSettings()
{
    Save();
    delete Settings;
}

void AppSettings::Save()
{
    wxFileOutputStream configStream(wxGetCwd() + "/config.txt");

    if (configStream.IsOk())
    {
        Settings->Save(configStream);
    }
}

AppSettings* AppSettings::GetInstance()
{
    if (instance == nullptr) // If instance doesn't exist
    {
        // Create and return new instance
        instance = new AppSettings();
        atexit(AppSettings::CleanUp);

        return instance;
    }
    else
    {
        return instance; // Otherwise return the instance
    }
}

void AppSettings::CleanUp()
{
    delete instance;
}
