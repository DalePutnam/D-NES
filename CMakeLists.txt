cmake_minimum_required(VERSION 3.10.2)

#project(D-NES)

# Default build type is release
if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE "Release" CACHE STRING 
         "Build Type: Set to \"Release\" for optimized or \"Debug\" for a debug build" FORCE)
endif (NOT CMAKE_BUILD_TYPE)

# Output final binary to 'bin' directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Check for OpenGL and GLX
find_package(OpenGL COMPONENTS OpenGL GLX REQUIRED)

# Check for ALSA
find_package(ALSA REQUIRED)

# Check for X11
find_package(X11 REQUIRED)

# Check for wxWidgets
set(wxWidgets_CONFIG_OPTIONS --toolkit=gtk3)
find_package(wxWidgets COMPONENTS core base REQUIRED)

# Check for GTK3
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

add_subdirectory(externals)
add_subdirectory(source)
