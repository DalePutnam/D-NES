/*
 * main.cc
 *
 *  Created on: Mar 15, 2014
 *      Author: Dale
 */

#include <ctime>
#include <string>
#include <iostream>
#include <chrono>
#include <ratio>
#include <gtkmm/application.h>

#include "FrontEnd/main_window.h"

int main(int argc, char *argv[])
{
    Glib::RefPtr<Gtk::Application> app = Gtk::Application::create(argc, argv, "org.TheWretchedEgg.D-NES");

    MainWindow window;

    return app->run(window);
}


