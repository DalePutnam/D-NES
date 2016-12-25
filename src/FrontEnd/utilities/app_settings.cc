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
    wxFileInputStream configStream(wxGetCwd() + "/config.txt");

    if (configStream.IsOk())
    {
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

    if (!Settings->HasEntry("/Paths/RomSavePath"))
    {
        wxFileName file(wxGetCwd(), "saves");
        Settings->Write("/Paths/RomSavePath", file.GetFullPath());
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



