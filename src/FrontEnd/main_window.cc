/*
 * main_window.cc
 *
 *  Created on: Sep 4, 2014
 *      Author: Dale
 */
#include <iostream>
#include <sstream>
#include <gtkmm/menuitem.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/builder.h>
#include <gtkmm/treeview.h>
#include <boost/filesystem.hpp>

#include "main_window.h"
#include "../Emulator/nes.h"

MainWindow::ListColumns::ListColumns()
{
	add(columnFileName);
	add(columnFileSize);
}

void MainWindow::menuInitialize()
{
	Gtk::MenuItem* menuItem = 0;

	builder->get_widget("openMenuItem", menuItem);
	menuItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onOpenROM));
	builder->get_widget("exitMenuItem", menuItem);
	menuItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onExitClicked));
	builder->get_widget("allSettingsMenuItem", menuItem);
	menuItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onAllSettings));
}

void MainWindow::listInitialize()
{
	Gtk::TreeView* list = 0;
	builder->get_widget("romList", list);
	list->set_model(listStore);
	list->append_column("File Name", columns.columnFileName);
	list->append_column("File Size", columns.columnFileSize);

	//list->signal_row_activated().connect(sigc::mem_fun(*this, &MainWindow::onROMSelected));

	populateStore();
}

void MainWindow::listUpdate()
{
	listStore->clear();
	populateStore();
}

void MainWindow::populateStore()
{
	namespace fs = boost::filesystem;

	fs::path filePath(settings->get<std::string>("frontend.rompath"));
	if (fs::exists(filePath))
	{
		if (fs::is_directory(filePath))
		{
			Gtk::ListStore::iterator listIterator;
			for (fs::directory_iterator it(filePath); it != fs::directory_iterator(); ++it)
			{
				if (fs::is_regular_file(it->path())
					&& it->path().has_extension()
					&& std::string(".nes").compare(it->path().extension().string()) == 0)
				{
					listIterator = listStore->append();
					(*listIterator)[columns.columnFileName] = it->path().filename().string();

					std::ostringstream oss;
					oss << file_size(it->path()) / 1024 << " KiB";

					(*listIterator)[columns.columnFileSize] = oss.str();
				}
			}
		}
	}
}

void MainWindow::onOpenROM()
{
	Gtk::FileChooserDialog dialog("Choose a ROM", Gtk::FILE_CHOOSER_ACTION_OPEN);
	dialog.set_transient_for(*this);

	//Add response buttons the the dialog:
	dialog.add_button("Cancel", Gtk::RESPONSE_CANCEL);
	dialog.add_button("Open", Gtk::RESPONSE_OK);

	int result = dialog.run();

	//Handle the response:
	switch(result)
	{
		case(Gtk::RESPONSE_OK):
		{
			std::cout << "Open clicked." << std::endl;

			std::string filename = dialog.get_filename();
			std::cout << "File selected: " <<  filename << std::endl;

			NES nes(filename);
			nes.Start();

			break;
		}
		case(Gtk::RESPONSE_CANCEL):
		{
			std::cout << "Cancel clicked." << std::endl;
			break;
		}
		default:
		{
			std::cout << "Unexpected button clicked." << std::endl;
			break;
		}
	}
}

void MainWindow::onAllSettings()
{
	settingsWindow = new SettingsWindow();
	settingsWindow->set_transient_for(*this);
	settingsWindow->signal_hide().connect(sigc::mem_fun(*this, &MainWindow::onSettingsHide));
	settingsWindow->show();
}

void MainWindow::onExitClicked()
{
	hide();
}

void MainWindow::onSettingsHide()
{
	delete settingsWindow;
	listUpdate();
}

void MainWindow::onROMSelected(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* col)
{
	//std::cout << "Aww Fuck!" << std::endl;
}

MainWindow::MainWindow()
	: settings(AppSettings::getInstance()),
	  settingsWindow(0),
	  listStore(Gtk::ListStore::create(columns)),
#ifdef DEBUG
	  builder(Gtk::Builder::create_from_file("D:/Source/D-NES/src/FrontEnd/glade/MainWindow.glade"))
#else
	  builder(Gtk::Builder::create_from_resource("/glade/MainWindow.glade"))
#endif
{
	set_default_size(600, 400);
	set_title("D-NES");

	menuInitialize();
	listInitialize();

	Gtk::Box* vbox = 0;
	builder->get_widget("mainBox", vbox);
	add(*vbox);

	vbox->show_all();
}

MainWindow::~MainWindow()
{
	AppSettings::cleanUp();
}
