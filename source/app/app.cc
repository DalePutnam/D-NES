#include <cstdlib>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <wx/cmdline.h>

#include <dnes/dnes.h>

#include "app.h"
#include "main_window.h"
#include "utilities/app_settings.h"

#ifdef __linux
#include <X11/Xlib.h>
#endif

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
bool NESApp::OnInit()
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
        class CallbackImpl : public dnes::NESCallback
        {
            using steady_clock = std::chrono::steady_clock;
            using time_point = std::chrono::time_point<steady_clock>;
            using seconds = std::chrono::seconds;

        public:
            CallbackImpl() = default;

            void OnFrameComplete(dnes::NES* nes)
            {
                using namespace std::chrono_literals;

                if (frameCounter == -1)
                {
                    frameCounter = 0;
                    periodStart = steady_clock::now();

                    return;
                }

                if (std::chrono::duration_cast<seconds>(steady_clock::now() - periodStart) >= 1s)
                {
                    std::cout << "\rFPS: " << frameCounter << std::flush;

                    fpsAccumulator += frameCounter;
                    numFpsRecords++;
                    frameCounter = 0;
                    periodStart = steady_clock::now();

                    return;
                }

                frameCounter++;
            };
            void OnError(dnes::NES* nes) {};

            double GetAverageFps() {
                return static_cast<double>(fpsAccumulator) / numFpsRecords;
            }

            ~CallbackImpl() = default;
        private:
            int32_t frameCounter{-1};
            uint64_t fpsAccumulator{0};
            uint64_t numFpsRecords{0};

            time_point periodStart{time_point::min()};
        };

        std::cout << "D-NES Profile Mode" << std::endl;

        using namespace std::chrono_literals;

        CallbackImpl callback;

        std::cout << "Loading ROM " << game << std::flush;
        dnes::NES* nes = dnes::createNES();

        if (nes->Initialize(game.c_str()))
        {
            nes->SetCallback(&callback);

            nes->SetAudioEnabled(false);

            std::cout << ": Success!" << std::endl;

            std::cout << "Starting Emulator. Running 30 second test." << std::endl;

            nes->Start();


            std::this_thread::sleep_for(30s);
            //std::cin.get();

            nes->Stop();

            dnes::destroyNES(nes);

            std::cout << "\nAverage FPS: " << callback.GetAverageFps() << std::endl;
        } 
        else
        {
            std::cout << ": Failed!" << std::endl;
            std::cout << "Press Enter to Terminate." << std::endl;
            
            std::cin.get();
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

bool NESApp::Initialize(int& argc, wxChar **argv)
{
#ifdef __linux
    XInitThreads();
#endif

    return wxApp::Initialize(argc, argv);
}

// Main Function
wxIMPLEMENT_APP(NESApp);