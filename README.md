# D-NES

A Nintendo Entertainment System emulator I've been working on in my spare time.

Officially retiring from working on this. There's so much else I want to work on, but this emulator keeps finding a way to take up my time. Even though no one uses it and it was purely
to see if I could make an emulator that functions at all, which I've achieved. Time to move on.

CPU: Working with full support for unofficial opcodes  
PPU (Video): Working, but I'm still working out some of the more obscure behaviour.   
APU (Audio): Working on Windows and Linux.  
Input: Very simple implementation. No ability to remap inputs. One controller only.   

## Build Instructions
### Windows

Requirements: Visual Studio 2017

1. Clone the repository to the location of your choice.
2. Download wxWidgets 3.0.4 and extract it to a directory of your choice.
3. Open wx_vc12.sln in build/msw in the wxWidgets directory with Visual Studio 2017 and upgrade the solution.
4. Build the x64 Debug and Release (non-DLL) wxWidgets configurations.
5. Add an environment variable called WXWIDGETS_ROOT that points to root of the wxWidgets directory.
6. Open D-NES.sln and build the solution.

### Linux

1. Install wxWidgets 3.0.4
2. Install libasound2-dev
3. Download the repository to the location of your choice.
4. Navigate to the repository root and run the following commands
5. `mkdir build`
6. `cd build`
7. `cmake ..`
8. `make`
9. The output program can be found in the `build/bin`

### OSX

No support for OSX.
