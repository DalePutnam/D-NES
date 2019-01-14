/*
 * app_settings.h
 *
 *  Created on: Sep 10, 2014
 *      Author: Dale
 */

#pragma once

#include <string>
#include <memory>
#include <wx/fileconf.h>

class AppSettings
{
public:
    static AppSettings& GetInstance(); // Get single instance

    void Save(); // Write out to file

    // Get a setting
    template<typename T>
    bool Read(const wxString& name, T* value)
    {
        return Settings->Read(name, value);
    }

    bool Read(const wxString& name, std::string* value)
    {
        wxString str(*value);
        bool result = Settings->Read(name, &str);

        *value = str.ToStdString();
        return result;
    }

    // Change a setting
    template<typename T>
    bool Write(const wxString& name, const T& value)
    {
        return Settings->Write(name, value);
    }    

private:
    // Singleton
    AppSettings();
    ~AppSettings();

    // Disable copying and moving
    AppSettings(const AppSettings&) = delete;
    AppSettings& operator=(const AppSettings&) = delete;
    AppSettings(AppSettings&&) = delete;
    AppSettings& operator=(AppSettings&&) = delete;

    // Application Settings
    std::unique_ptr<wxFileConfig> Settings;
};

