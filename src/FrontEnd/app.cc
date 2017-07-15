#include <cstdlib>

#include <wx/cmdline.h>

#include "nes.h"
#include "app.h"
#include "main_window.h"
#include "utilities/app_settings.h"

// App Implementation
bool MyApp::OnInit()
{
	wxCmdLineParser parser(wxApp::argc, wxApp::argv);
	parser.AddOption("p", "profile");

	parser.Parse();

	wxString game;
	if (parser.Found("p", &game))
	{
		NesParams params;
		params.RomPath = game;
		params.FrameLimitEnabled = false;
		params.AudioEnabled = false;

		NES nes(params);
		nes.Start();

		while (!wxGetKeyState(WXK_ESCAPE));

		nes.Stop();
	}
	else
	{
		MainWindow* window = new MainWindow();
		window->Show(true);
	}

	return true;
}

// Main Function
wxIMPLEMENT_APP(MyApp);