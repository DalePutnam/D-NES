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
#include <gtkmm/messagedialog.h>
#include <gtkmm/builder.h>
#include <gtkmm/treeview.h>
#include <gtkmm/textbuffer.h>
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

	// Connect all signals
	builder->get_widget("openMenuItem", menuItem);
	menuItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onOpenROM));
	builder->get_widget("exitMenuItem", menuItem);
	menuItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onExitClicked));
	builder->get_widget("allSettingsMenuItem", menuItem);
	menuItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onAllSettings));
	builder->get_widget("resumeMenuItem", menuItem);
	menuItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onResumeClicked));
	builder->get_widget("stopMenuItem", menuItem);
	menuItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onStopClicked));
	builder->get_widget("pauseMenuItem", menuItem);
	menuItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onPauseClicked));
	builder->get_widget("nameTableMenuItem", menuItem);
	menuItem->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::onViewerClicked));
}

void MainWindow::listInitialize()
{
	Gtk::TreeView* list = 0;
	builder->get_widget("romList", list);

	// Add columns to list store
	list->set_model(listStore);
	list->append_column("File Name", columns.columnFileName);
	list->append_column("File Size", columns.columnFileSize);

	// Connect signal
	list->signal_row_activated().connect(sigc::mem_fun(*this, &MainWindow::onROMSelected));

	// Initialize selection
	treeSelection = list->get_selection();
	populateStore(); // Populate list store
}

void MainWindow::listUpdate()
{
	listStore->clear(); // Clear out current list store
	populateStore(); // Re-populate list store
}

void MainWindow::onOpenROM()
{
	// Create ROM chooser dialog
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
			std::string filename = dialog.get_filename();
			startEmulator(filename);
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
	// Create and open modal settings window
	settingsWindow = new SettingsWindow();
	settingsWindow->set_transient_for(*this);
	settingsWindow->signal_hide().connect(sigc::mem_fun(*this, &MainWindow::onSettingsHide));
	settingsWindow->show();
}

void MainWindow::onViewerClicked()
{
	if (worker)
	{
		viewer = new NameTableViewer(*worker);
		viewer->signal_hide().connect(sigc::mem_fun(*this, &MainWindow::onViewerHide));
		viewer->show();
	}
}

void MainWindow::onViewerHide()
{
	delete viewer;
	viewer = 0;
}

void MainWindow::onExitClicked()
{
	hide(); // Close this window
}

void MainWindow::onSettingsHide()
{
	// Close settings window
	delete settingsWindow;
	settingsWindow = 0;

	// Update affected components
	listUpdate();
}

void MainWindow::onGameHide()
{
	if (workerThread) // If worker thread exists
	{
		Glib::Threads::Mutex::Lock lock(mutex);
		// Issue stop command and wait for completion
		worker->stopWorker();
		workerThread->join();
		workerThread = 0; // Destroy worker thread
		delete worker;
		worker = 0;
	}

	// Close gameWindow window
	delete gameWindow;
	gameWindow = 0;
	delete viewer;
	viewer = 0;
	delete logWindow;
	logWindow = 0;
}

#ifdef DEBUG
void MainWindow::onLogHide()
{
	if (workerThread) // If worker thread exists
	{
		Glib::Threads::Mutex::Lock lock(mutex);
		// Issue stop command and wait for completion
		worker->stopWorker();
		workerThread->join();
		workerThread = 0; // Destroy worker thread
		delete worker;
		worker = 0;
	}

	// Close log window
	delete logWindow;
	logWindow = 0;
}
#endif

void MainWindow::onROMSelected(const Gtk::TreeModel::Path& path, Gtk::TreeView::Column* col)
{
	// Get file name from list and start emulator
	std::string filename = (*treeSelection->get_selected())[columns.columnFileName];
	startEmulator(settings->get<std::string>("frontend.rompath") + "/" + filename);
}

void MainWindow::onWorkerNotify()
{
	if (gameWindow && worker->isFrameUpdated())
	{
		Glib::Threads::Mutex::Lock lock(mutex);
		if (worker)
		{
			gameWindow->UpdateFrame(worker->getFrame());
		}
	}

#ifdef DEBUG
	if (logWindow && worker->isLogUpdated()) // If log window is active
	{
		Glib::Threads::Mutex::Lock lock(mutex);
		if (worker)
		{
			logWindow->updateBuffer(worker->getLogChunk()); // Update log text
			worker->setLogRetrieved(true); // Notify worker that
		}
	}
#endif
	if (workerThread && worker->hasStopped()) // If worker has stopped on it's own
	{
		Glib::Threads::Mutex::Lock lock(mutex);
		// Synchronize and destroy worker thread
		workerThread->join();
		workerThread = 0;
		delete worker;
		worker = 0;
	}
}

void MainWindow::onResumeClicked()
{
	if (workerThread)
	{
		Glib::Threads::Mutex::Lock lock(mutex);
		worker->resumeWorker();
	}
}

void MainWindow::onStopClicked()
{
	if (workerThread) // If worker thread exists
	{
		Glib::Threads::Mutex::Lock lock(mutex);
		// Issue stop command and wait for completion
		worker->stopWorker();
		workerThread->join();
		workerThread = 0; // Destroy worker thread
		delete worker;
		worker = 0;
	}
#ifdef DEBUG
	// Close log window
	delete gameWindow;
	gameWindow = 0;
	delete viewer;
	viewer = 0;
	delete logWindow;
	logWindow = 0;
#endif
}

void MainWindow::onPauseClicked()
{
	if (workerThread)
	{
		Glib::Threads::Mutex::Lock lock(mutex);
		worker->pauseWorker();
	}
}

void MainWindow::populateStore()
{
	namespace fs = boost::filesystem;

	fs::path filePath(settings->get<std::string>("frontend.rompath")); // Get path from settings
	if (fs::exists(filePath))
	{
		if (fs::is_directory(filePath))
		{
			// Iterate over every file in the directory
			Gtk::ListStore::iterator listIterator;
			for (fs::directory_iterator it(filePath); it != fs::directory_iterator(); ++it)
			{
				// If the file is a regular file and it has a .nes extension, add it to the list
				if (fs::is_regular_file(it->path())
					&& it->path().has_extension()
					&& std::string(".nes").compare(it->path().extension().string()) == 0)
				{
					listIterator = listStore->append();
					(*listIterator)[columns.columnFileName] = it->path().filename().string();

					// Get file size in Kibibytes
					std::ostringstream oss;
					oss << file_size(it->path()) / 1024 << " KiB";

					(*listIterator)[columns.columnFileSize] = oss.str();
				}
			}
		}
	}
}

void MainWindow::startEmulator(std::string pathToROM)
{
	try
	{
		if (!worker)
		{

#ifdef DEBUG
		// Create worker instance to manage emulator
		worker = new EmulatorWorker(pathToROM, *this, false);
		// Open log window
		//logWindow = new LogWindow();
		//logWindow->signal_hide().connect(sigc::mem_fun(*this, &MainWindow::onLogHide));
		//logWindow->show();
		gameWindow = new GameWindow(256, 240);
		gameWindow->signal_hide().connect(sigc::mem_fun(*this, &MainWindow::onGameHide));
		gameWindow->show();
#else
		worker = new EmulatorWorker(pathToROM, *this);
#endif
		// Start NES emulator
		workerThread = Glib::Threads::Thread::create(sigc::bind(sigc::mem_fun(*worker, &EmulatorWorker::startWorker), this));
		}
		else
		{
			// Print error
			Gtk::MessageDialog dialog(*this, "Emulator Running", Gtk::MESSAGE_INFO);
			dialog.set_secondary_text("An Emulator instance is already running\n");
			dialog.run();
		}
	}
	catch (std::string& err)
	{
		// The worker causes the exception so it's safe to delete
		delete worker;
		worker = 0;

		// Print error
		Gtk::MessageDialog dialog(*this, "ROM Error", Gtk::MESSAGE_ERROR);
		dialog.set_secondary_text("Failed to open ROM\n" + err);
		dialog.run();
	}
}

MainWindow::MainWindow() :
	worker(0),
	settings(AppSettings::getInstance()),
	settingsWindow(0),
	gameWindow(0),
	viewer(0),
	workerThread(0),
	listStore(Gtk::ListStore::create(columns)),
#ifdef DEBUG
	builder(Gtk::Builder::create_from_file("D:/Source/D-NES/src/FrontEnd/glade/MainWindow.glade")), // Get glade from file
	logWindow(0)
#else
	builder(Gtk::Builder::create_from_resource("/glade/MainWindow.glade")) // Get glade from resource
#endif
{
	set_default_size(600, 400);
	set_title("D-NES");

	// Initialize components
	menuInitialize();
	listInitialize();

	// Connect Signals
	dispatcher.connect(sigc::mem_fun(*this, &MainWindow::onWorkerNotify));

	// Add glade file contents to window
	Gtk::Box* vbox = 0;
	builder->get_widget("mainBox", vbox);
	add(*vbox);

	vbox->show_all(); // Show window
}

MainWindow::~MainWindow()
{
	AppSettings::cleanUp();
	delete worker;
	delete settingsWindow;
	delete gameWindow;
	delete viewer;
#ifdef DEBUG
	delete logWindow;
#endif
}

void MainWindow::notify()
{
	dispatcher.emit(); // Notify dispatcher
}
