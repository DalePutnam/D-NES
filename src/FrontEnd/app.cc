#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
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

		/*
		auto UpdateFps = [&nes](uint8_t*)
		{
			//std::cout << nes.GetFrameRate() << '\r' << std::flush;
			//OutputDebugStringA((std::to_string(nes.GetFrameRate()) + "\r").c_str());
		};
		*/
		//nes.BindFrameCompleteCallback(UpdateFps);
		nes.Start();

		//while (!wxGetKeyState(WXK_ESCAPE));
		while (true)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			//Sleep(1000);
		}

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