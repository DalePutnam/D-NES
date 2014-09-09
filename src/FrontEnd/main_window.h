/*
 * main_window.h
 *
 *  Created on: Sep 4, 2014
 *      Author: Dale
 */

#ifndef MAIN_WINDOW_H_
#define MAIN_WINDOW_H_

#include <gtkmm/window.h>
#include "settings_window.h"

class MainWindow : public Gtk::Window
{
	SettingsWindow* settings;

	// Signal Handlers
	void onOpenROM();
	void onAllSettings();
	void onExitClicked();
	void onSettingsHide();

public:
	MainWindow();
};

#endif /* MAIN_WINDOW_H_ */
