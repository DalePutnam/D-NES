/*
 * settings_window.cc
 *
 *  Created on: Sep 7, 2014
 *      Author: Dale
 */
#include <string>
#include <fstream>
#include <gtkmm/notebook.h>
#include <gtkmm/button.h>
#include <gtkmm/entry.h>
#include <gtkmm/box.h>

#include "settings_window.h"

void SettingsWindow::okClicked()
{
	Gtk::Entry* entry = 0;
	builder->get_widget("romPath", entry);
	settings->put("frontend.rompath", entry->get_text());
	settings->save();

	hide();
}

void SettingsWindow::cancelClicked()
{
	hide();
}

SettingsWindow::SettingsWindow()
	: settings(AppSettings::getInstance()),
#ifdef DEBUG
	  builder(Gtk::Builder::create_from_file("D:/Source/D-NES/src/FrontEnd/glade/SettingsWindow.glade"))
#else
	  builder(Gtk::Builder::create_from_resource("/glade/SettingsWindow.glade"))
#endif
{
	set_default_size(300, 400);
	set_title("D-NES Settings");
	set_resizable(false);
	set_modal(true);

	Gtk::Box* vbox = 0;
	builder->get_widget("settingsBox", vbox);
	add(*vbox);

	Gtk::Button* button = 0;
	builder->get_widget("settingsOKButton", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &SettingsWindow::okClicked));
	builder->get_widget("settingsCancelButton", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &SettingsWindow::cancelClicked));

	Gtk::Entry* entry = 0;
	builder->get_widget("romPath", entry);

	entry->set_text(settings->get<std::string>("frontend.rompath"));

	vbox->show_all();
}


