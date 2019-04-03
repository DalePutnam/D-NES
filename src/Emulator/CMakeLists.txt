cmake_minimum_required(VERSION 3.10.2)

project(Emulator)

include_directories(mappers audio video common .)

add_library(core
    nes.cc
    cpu.cc
    ppu.cc
    apu.cc
    cart.cc
    common/file.cc
    common/nes_exception.cc
    common/ines.cc
    mappers/mapper_base.cc
    mappers/nrom.cc
    mappers/sxrom.cc
    audio/audio_backend.cc
    audio/iaudio_platform.cc
    audio/alsa_platform.cc
    video/gl_util.cc
    video/video_backend.cc
    video/igl_platform.cc
    video/glx_platform.cc
)

target_link_libraries(core ${ALSA_LIBRARIES} ${OPENGL_LIBRARIES})

set(CORE_INCLUDE_DIRS ${PROJECT_SOURCE_DIR} CACHE INTERNAL "Core: Include Directories" FORCE)