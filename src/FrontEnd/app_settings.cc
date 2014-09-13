/*
 * app_settings.cc
 *
 *  Created on: Sep 11, 2014
 *      Author: Dale
 */

#include <boost/property_tree/xml_parser.hpp>
#include "app_settings.h"

AppSettings* AppSettings::instance = 0;

AppSettings::AppSettings()
{
	boost::property_tree::xml_parser::read_xml("./config.xml", settings);
}

AppSettings::~AppSettings() {}

void AppSettings::save()
{
	boost::property_tree::xml_parser::write_xml("./config.xml", settings);
}

AppSettings* AppSettings::getInstance()
{
	if (instance == 0)
	{
		instance = new AppSettings();
		return instance;
	}
	else
	{
		return instance;
	}
}

void AppSettings::cleanUp()
{
	delete instance;
}



