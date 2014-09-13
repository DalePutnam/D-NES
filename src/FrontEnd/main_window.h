/*
 * main_window.h
 *
 *  Created on: Sep 4, 2014
 *      Author: Dale
 */

#ifndef MAIN_WINDOW_H_
#define MAIN_WINDOW_H_

#include <gtkmm/window.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treeviewcolumn.h>
#include "app_settings.h"
#include "settings_window.h"

class MainWindow : public Gtk::Window
{
	struct ListColumns : public Gtk::TreeModelColumnRecord
	{
		Gtk::TreeModelColumn<Glib::ustring> columnFileName;
		Gtk::TreeModelColumn<Glib::ustring> columnFileSize;

		ListColumns();
	};

	ListColumns columns;

	AppSettings* settings;
	SettingsWindow* settingsWindow;
	Glib::RefPtr<Gtk::ListStore> listStore;
	Glib::RefPtr<Gtk::Builder> builder;

	// Initializers
	void menuInitialize();
	void listInitialize();

	// Updaters
	void listUpdate();

	void populateStore();

	// Signal Handlers
	void onOpenROM();
	void onAllSettings();
	void onExitClicked();
	void onSettingsHide();
	void onROMSelected(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* col);

public:
	MainWindow();
	~MainWindow();
};

#endif /* MAIN_WINDOW_H_ */
