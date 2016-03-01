# D-NES

A Nintendo Entertainment System emulator I've been working on in my spare time.

CPU: Working, but no support for unofficial opcodes.  
PPU (Video): Working, but I'm still working out some of the more obscure behaviour.  
APU (Audio): Unimplemented.  
Input: Very simple implementation. No ability to remap inputs.  

## Build Instructions
### Windows

Requirements: Visual Studio 2013 (I haven't tried 2015 yet)

1. Download the repository to the location of your choice.
2. In the root of the repository create a directory called `externals`.
3. Download boost and extract it to the externals folder. Make sure to name the folder simply `boost` with no version number.
4. Open a Command Prompt and navigate to `externals/boost`
5. Run `bootstrap.bat`
6. Run `b2.exe toolset=msvc-12.0 variant=debug link=static --with-system --with-filesystem --with-chrono --with-iostreams`
7. Run `b2.exe toolset=msvc-12.0 variant=release link=static --with-system --with-filesystem --with-chrono --with-iostreams`
8. Download wxWidgets-3.0 and extract it to the externals folder. Name it simply `wxWidgets`.
9. Navigate to `externals/wxWidgets/build/msw` and open `wx_vc12.sln`.
10. Build both the Debug and Release configurations. Once done close the solution.
11. The build environment is now setup. Open D-NES.sln and build either Debug or Release.

### Linux

1. Download the repository to the location of your choice.
2. Use your preferred package manager to install boost and wxWidgets-3.0
3. Navigate to the repository root.
4. Run `make debug` to build with debugging symbols or simply `make` for an optimized build.

### OSX

No support for OSX at this time and possibly any time in the future as I do not own a Mac.

