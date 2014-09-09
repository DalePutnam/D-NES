/*
 * main_window.cc
 *
 *  Created on: Sep 4, 2014
 *      Author: Dale
 */
#include <iostream>
#include <gtkmm/menuitem.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/builder.h>

#include "main_window.h"
#include "../Emulator/nes.h"

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
	settings = new SettingsWindow();
	settings->set_transient_for(*this);
	settings->signal_hide().connect(sigc::mem_fun(*this, &MainWindow::onSettingsHide));
	settings->show();
}

void MainWindow::onExitClicked()
{
	hide();
}

void MainWindow::onSettingsHide()
{
	delete settings;
}

MainWindow::MainWindow()
{
	set_default_size(600, 400);
	set_title("D-NES");

#ifdef DEBUG
	Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_file("D:/Source/D-NES/src/FrontEnd/glade/MainWindow.glade");
#else
	Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_resource("/glade/MainWindow.glade");
#endif

	Gtk::Box* vbox = 0;
	builder->get_widget("mainBox", vbox);
	add(*vbox);

	Gtk::MenuItem* menuItem = 0;

	builder->get_widget("openMenuItem", menuItem);
	menuItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onOpenROM));
	builder->get_widget("exitMenuItem", menuItem);
	menuItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onExitClicked));
	builder->get_widget("allSettingsMenuItem", menuItem);
	menuItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onAllSettings));

	vbox->show_all();
}

