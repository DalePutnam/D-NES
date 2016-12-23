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
        settings = new wxFileConfig(configStream);
    }
    else
    {
        settings = new wxFileConfig();
    }


    if (!settings->HasEntry("/Paths/RomPath"))
    {
        settings->Write("/Paths/RomPath", wxGetCwd());
    }

    if (!settings->HasEntry("/Paths/RomSavePath"))
    {
        wxFileName file(wxGetCwd(), "saves");
        settings->Write("/Paths/RomSavePath", file.GetFullPath());
    }    
}

AppSettings::~AppSettings()
{
    Save();
    delete settings;
}

void AppSettings::Save()
{
    wxFileOutputStream configStream(wxGetCwd() + "/config.txt");

    if (configStream.IsOk())
    {
        settings->Save(configStream);
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



