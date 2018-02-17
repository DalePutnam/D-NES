# D-NES

A Nintendo Entertainment System emulator I've been working on in my spare time.

CPU: Working, but no support for unofficial opcodes.   
PPU (Video): Working, but I'm still working out some of the more obscure behaviour.   
APU (Audio): Working on Windows and Linux. Still some popping, but much better than before.   
Input: Very simple implementation. No ability to remap inputs. One controller only.   

## Build Instructions
### Windows

Requirements: Visual Studio 2017

1. Clone the repository to the location of your choice.
2. Download wxWidgets 3.0.3 and extract it to a directory of your choice.
3. Open wx_vc12.sln in build/msw in the wxWidgets directory with Visual Studio 2017 and upgrade the solution.
3. Build the x64 Debug and Release (non-DLL) wxWidgets configurations.
4. Add an environment variable called WXWIDGETS_ROOT that points to root of the wxWidgets directory.
5. Open D-NES.sln and build the solution.

### Linux

1. Install wxWidgets 3.1.0
2. Install libasound-dev
3. Download the repository to the location of your choice.
4. Navigate to the repository root.
5. Run `make debug` to build with debugging symbols or simply `make` for an optimized build.

### OSX

No support for OSX.
