# D-NES

A Nintendo Entertainment System emulator I've been working on in my spare time.

CPU: Working, but no support for unofficial opcodes.  
PPU (Video): Working, but I'm still working out some of the more obscure behaviour.  
APU (Audio): Working. Still some issues with popping, but otherwise it's functional.  
Input: Very simple implementation. No ability to remap inputs. One controller only.  

## Build Instructions
### Windows

Requirements: Visual Studio 2015

1. Download the repository to the location of your choice.
2. In the root of the repository create a directory called `ext`.
3. Download boost 1.60.0 and extract it to the ext folder. Make sure to name the folder simply `boost` with no version number.
4. Open a Command Prompt and navigate to `ext/boost`
5. Run `bootstrap.bat`
6. Run `b2.exe toolset=msvc variant=debug link=static --with-system --with-filesystem --with-chrono --with-iostreams`
7. Run `b2.exe toolset=msvc variant=release link=static --with-system --with-filesystem --with-chrono --with-iostreams`
8. Download wxWidgets-3.1.0 and extract it to the externals folder. Name it simply `wxWidgets`.
9. Navigate to `ext/wxWidgets/build/msw` and open `wx_vc14.sln`.
10. Build both the Debug and Release configurations. Once done close the solution.
11. The build environment is now set up. Open D-NES.sln and build either Debug or Release.

### Linux (Broken currently)

1. Download the repository to the location of your choice.
2. Use your preferred package manager to install boost and wxWidgets-3.0
3. Navigate to the repository root.
4. Run `make debug` to build with debugging symbols or simply `make` for an optimized build.

### OSX

No support for OSX at this time and possibly any time in the future as I do not own a Mac.

