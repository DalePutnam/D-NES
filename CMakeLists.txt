cmake_minimum_required(VERSION 3.10.2)

project(D-NES)

# Output final binary to 'bin' directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Use C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Check for OpenGL and GLX
find_package(OpenGL COMPONENTS OpenGL GLX REQUIRED)

# Check for ALSA
find_package(ALSA REQUIRED)

# Check for X11
find_package(X11 REQUIRED)

# Check for wxWidgets
find_package(wxWidgets COMPONENTS core base REQUIRED)

# Check for GTK3
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

add_subdirectory(src/Emulator)
add_subdirectory(src/FrontEnd)
