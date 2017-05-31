# D-NES

A Nintendo Entertainment System emulator I've been working on in my spare time.

CPU: Working, but no support for unofficial opcodes.  
PPU (Video): Working, but I'm still working out some of the more obscure behaviour.  
APU (Audio): Working on Windows. Preliminary support on Linux, desyncs easily, crackling on some machines 
Input: Very simple implementation. No ability to remap inputs. One controller only.  

## Build Instructions
### Windows

Requirements: Visual Studio 2015 (or 2017 with the 2015 build tools installed)

1. Clone the repository to the location of your choice.
2. Download wxWidgets-3.1.0 and extract it to D-NES/ext. Name it simply `wxWidgets`.
3. Open D-NES.sln and build either the x64 or Win32 configurations. The first compile will take ~5 minutes since wxWidgets needs to be built.
4. Done. You can now begin working on the emulator.

### Linux

1. Install wxWidgets 3.1.0
2. Install libasound-dev
3. Download the repository to the location of your choice.
4. Navigate to the repository root.
5. Run `make debug` to build with debugging symbols or simply `make` for an optimized build.

### OSX

No support for OSX at this time and possibly any time in the future as I do not own a Mac.
