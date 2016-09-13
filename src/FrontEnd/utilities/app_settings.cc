/*
 * app_settings.cc
 *
 *  Created on: Sep 11, 2014
 *      Author: Dale
 */

#include <fstream>
#include <wx/filename.h>

#include "app_settings.h"

AppSettings* AppSettings::instance = nullptr; // Initialize static instance field

AppSettings::AppSettings()
{
	wxFileName configName = wxFileConfig::GetLocalFile("config.txt");
	configName.AppendDir("D-NES");

	if (!configName.DirExists())
    {
		configName.Mkdir();
	}

	settings = new wxFileConfig("D-NES", "", "D-NES/config.txt", "", wxCONFIG_USE_LOCAL_FILE);
}

AppSettings::~AppSettings()
{
	delete settings;
}

void AppSettings::Save()
{
    // save settings to file
	settings->Flush();
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



