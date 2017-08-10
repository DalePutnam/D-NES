#include <cstdlib>
#include <iostream>
#include <string>
#include <wx/cmdline.h>

#include "nes.h"
#include "app.h"
#include "main_window.h"
#include "utilities/app_settings.h"

#ifdef _WIN32
static void InitConsole()
{
	AllocConsole();
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);

	std::cout.clear();
	std::cerr.clear();
	std::cin.clear();
}
#endif

// App Implementation
bool MyApp::OnInit()
{
	wxCmdLineParser parser(wxApp::argc, wxApp::argv);
	parser.AddOption("p", "profile");

	parser.Parse();

	wxString game;
	if (parser.Found("p", &game))
	{
#ifdef _WIN32
		InitConsole();
#endif
		std::cout << "D-NES Profile Mode" << std::endl;

		{
			NesParams params;
			params.RomPath = game;
			params.TurboModeEnabled = true;
            params.HeadlessMode = true;

			try 
			{
				std::cout << "Loading ROM " << params.RomPath << std::flush;
				NES nes(params);

				std::cout << ": Success!" << std::endl;

				auto UpdateFps = [&nes]()
				{
					std::cout << "\rFPS: " << nes.GetFrameRate() << std::flush;
				};

				nes.BindFrameCallback(UpdateFps);

				std::cout << "Starting Emulator. Press ENTER to Terminate." << std::endl;

				nes.Start();

				std::cin.get();

				nes.Stop();
			} 
			catch (...)
			{
				std::cout << ": Failed!" << std::endl;
				std::cout << "Press Enter to Terminate." << std::endl;
				
				std::cin.get();
			}
		}

#ifdef _WIN32
		FreeConsole();
#endif
		exit(0);
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