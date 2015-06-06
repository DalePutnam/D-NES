/*
 * app_settings.h
 *
 *  Created on: Sep 10, 2014
 *      Author: Dale
 */

#ifndef APP_SETTINGS_H_
#define APP_SETTINGS_H_

#include <string>
#include <boost/property_tree/ptree.hpp>

class AppSettings
{
    // Singleton
    static AppSettings* instance;
    AppSettings();
    AppSettings(const AppSettings&); // Prevent construction by copying
    AppSettings& operator=(const AppSettings&); // Prevent assignment
    ~AppSettings(); // Prevent unwanted destruction

    // Application Settings
    boost::property_tree::ptree settings;

public:

    static AppSettings* getInstance(); // Get single instance
    static void cleanUp(); // Destroy single instance

    void save(); // Write out to file

    // Get a setting
    template<typename T>
    T get(std::string name)
    {
        return settings.get<T>(name);
    }

    // Change a setting
    template<typename T>
    void put(std::string name, const T& value)
    {
        settings.put(name, value);
    }
};


#endif /* APP_SETTINGS_H_ */
