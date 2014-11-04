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
#include <gtkmm/treeview.h>
#include <glibmm/dispatcher.h>
#include <glibmm/threads.h>
#include "utilities/app_settings.h"
#include "utilities/emulator_worker.h"
#include "settings_window.h"
#include "name_table_viewer.h"
#include "game_window.h"

#ifdef DEBUG
#include "log_window.h"
#endif

class MainWindow : public Gtk::Window
{
	// Structure Defining the Layout of the ROM list
	struct ListColumns : public Gtk::TreeModelColumnRecord
	{
		Gtk::TreeModelColumn<std::string> columnFileName;
		Gtk::TreeModelColumn<std::string> columnFileSize;

		ListColumns();
	};

	ListColumns columns;

	EmulatorWorker* worker; // Worker object to manage NES thread
	AppSettings* settings; // Current application settings
	SettingsWindow* settingsWindow; // Window for editing settings
	GameWindow* gameWindow;
	NameTableViewer* viewer;
	Glib::Threads::Thread* workerThread; // Thread to run NES emulator

	Glib::RefPtr<Gtk::ListStore> listStore; // Store for the ROM list contents
	Glib::RefPtr<Gtk::Builder> builder; // Glade Builder
	Glib::RefPtr<Gtk::TreeSelection> treeSelection; // Currently selected ROM
	Glib::Dispatcher dispatcher; // Dispatcher for NES thread

	// Initializers
	void menuInitialize(); // Initialize Menu Bar
	void listInitialize(); // Initialize ROM List

	// Updaters
	void listUpdate(); // Update ROM List

	// Signal Handlers
	void onOpenROM();
	void onAllSettings();
	void onExitClicked();
	void onSettingsHide();
	void onGameHide();
	void onViewerHide();

	void onROMSelected(const Gtk::TreeModel::Path& path, Gtk::TreeView::Column* col);
	void onWorkerNotify();
	void onResumeClicked();
	void onPauseClicked();
	void onStopClicked();
	void onViewerClicked();

	// Utility Functions
	void populateStore(); // Populate the ListStore
	void startEmulator(std::string pathToROM);

	Glib::Threads::Mutex mutex;
	// Debug Stuff
#ifdef DEBUG
	LogWindow* logWindow; // Window to print logs from NES thread
	void onLogHide();
#endif

public:
	MainWindow();
	~MainWindow();

	void notify(); // Called by NES thread to notify dispatcher
};

#endif /* MAIN_WINDOW_H_ */
