/*
 * settings_window.h
 *
 *  Created on: Sep 7, 2014
 *      Author: Dale
 */

#ifndef SETTINGS_DIALOG_H_
#define SETTINGS_WINDOW_H_

#include <gtkmm/dialog.h>
#include <gtkmm/builder.h>


class SettingsWindow : public Gtk::Window
{
	Glib::RefPtr<Gtk::Builder> builder;

	void okClicked();
	void cancelClicked();

public:
	SettingsWindow();
};

#endif /* SETTINGS_WINDOW_H_ */
