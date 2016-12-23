/*
 * app_settings.h
 *
 *  Created on: Sep 10, 2014
 *      Author: Dale
 */

#pragma once

#include <string>
#include <wx/fileconf.h>

class AppSettings
{
public:
    static AppSettings* GetInstance(); // Get single instance

    void Save(); // Write out to file

                 // Get a setting
    template<typename T>
    bool Read(const wxString& name, T* value)
    {
        return settings->Read(name, value);
    }

    template<>
    bool Read<std::string>(const wxString& name, std::string* value)
    {
        wxString str(*value);
        bool result = settings->Read(name, &str);

        *value = str.ToStdString();
        return result;
    }

    // Change a setting
    template<typename T>
    bool Write(const wxString& name, const T& value)
    {
        return settings->Write(name, value);
    }

private:
    // Singleton
    static AppSettings* instance;
    static void CleanUp(); // Destroy single instance

    AppSettings();
    AppSettings(const AppSettings&); // Prevent construction by copying
    AppSettings& operator=(const AppSettings&); // Prevent assignment
    ~AppSettings(); // Prevent unwanted destruction

    // Application Settings
    wxFileConfig* settings;

};
