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

	static AppSettings* getInstance();
	static void cleanUp();

	void save();

	template<typename T>
	T get(std::string name)
	{
		return settings.get<T>(name);
	}

	template<typename T>
	void put(std::string name, const T& value)
	{
		settings.put(name, value);
	}
};


#endif /* APP_SETTINGS_H_ */
