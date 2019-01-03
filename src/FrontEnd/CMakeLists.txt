cmake_minimum_required(VERSION 3.10.2)

project(FrontEnd)

include_directories(utilities ${CORE_INCLUDE_DIRS})

include(${wxWidgets_USE_FILE})

include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})
add_definitions(${GTK_CFLAGS_OTHER})

add_executable(D-NES
    app.cc
    main_window.cc
    settings_window_base.cc
    audio_settings_window.cc
    video_settings_window.cc
    path_settings_window.cc
    ppu_viewer_window.cc
    game_list.cc
    utilities/app_settings.cc
)

target_link_libraries(D-NES ${wxWidgets_LIBRARIES} ${X11_LIBRARIES} ${GTK3_LIBRARIES} core)