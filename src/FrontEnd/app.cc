#include <cstdlib>

#include "app.h"
#include "main_window.h"
#include "utilities/app_settings.h"

// App Implementation
bool MyApp::OnInit()
{
	atexit(AppSettings::cleanUp);
	MainWindow* window = new MainWindow();
	window->Show(true);
	return true;
}

// Main Function
wxIMPLEMENT_APP(MyApp);