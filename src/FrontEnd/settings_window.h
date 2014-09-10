/*
 * settings_window.h
 *
 *  Created on: Sep 7, 2014
 *      Author: Dale
 */

#ifndef SETTINGS_DIALOG_H_
#define SETTINGS_WINDOW_H_

#include <gtkmm/window.h>
#include <gtkmm/builder.h>
#include <boost/program_options.hpp>

class SettingsWindow : public Gtk::Window
{
	Glib::RefPtr<Gtk::Builder> builder;
	boost::program_options::variables_map options;

	void okClicked();
	void cancelClicked();

public:
	SettingsWindow();
};

#endif /* SETTINGS_WINDOW_H_ */
