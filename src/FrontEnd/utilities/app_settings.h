/*
 * app_settings.h
 *
 *  Created on: Sep 10, 2014
 *      Author: Dale
 */

#ifndef APP_SETTINGS_H_
#define APP_SETTINGS_H_

#include <string>
#include <wx/fileconf.h>

class AppSettings
{
    // Singleton
    static AppSettings* instance;
	static void CleanUp(); // Destroy single instance

    AppSettings();
    AppSettings(const AppSettings&); // Prevent construction by copying
    AppSettings& operator=(const AppSettings&); // Prevent assignment
    ~AppSettings(); // Prevent unwanted destruction

    // Application Settings
	wxFileConfig* settings;
public:

    static AppSettings* GetInstance(); // Get single instance

    void Save(); // Write out to file

    // Get a setting
    template<typename T>
	bool Read(const wxString& name, T* value, const T& defaultValue)
    {
		return settings->Read(name, value, defaultValue);
    }

    // Change a setting
    template<typename T>
    bool Write(const wxString& name, const T& value)
    {
		return settings->Write(name, value);
    }
};


#endif /* APP_SETTINGS_H_ */
