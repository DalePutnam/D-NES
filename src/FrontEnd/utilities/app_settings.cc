/*
 * app_settings.cc
 *
 *  Created on: Sep 11, 2014
 *      Author: Dale
 */

#include <boost/property_tree/xml_parser.hpp>
#include "app_settings.h"

AppSettings* AppSettings::instance = 0; // Initialize static instance field

AppSettings::AppSettings()
{
	// Get settings from file
	boost::property_tree::xml_parser::read_xml("./config.xml", settings);
}

AppSettings::~AppSettings() {}

void AppSettings::save()
{
	// save settings to file
	boost::property_tree::xml_parser::write_xml("./config.xml", settings);
}

AppSettings* AppSettings::getInstance()
{
	if (instance == 0) // If instance doesn't exist
	{
		// Create and return new instance
		instance = new AppSettings();
		return instance;
	}
	else
	{
		return instance; // Otherwise return the instance
	}
}

void AppSettings::cleanUp()
{
	delete instance;
}


