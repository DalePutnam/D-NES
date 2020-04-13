cmake_minimum_required(VERSION 3.10.2)

project(dnes)

include_directories(mappers audio video common .)

add_library(dnes SHARED
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
    mappers/mmc1.cc
    mappers/uxrom.cc
    mappers/cnrom.cc
    mappers/mmc3.cc
    audio/audio_backend.cc
    audio/iaudio_platform.cc
    audio/alsa_platform.cc
    video/gl_util.cc
    video/video_backend.cc
    video/igl_platform.cc
    video/glx_platform.cc
)

set_target_properties(dnes PROPERTIES CXX_VISIBILITY_PRESET hidden)

target_link_libraries(dnes PRIVATE ${ALSA_LIBRARIES} ${OPENGL_LIBRARIES} spdlog)
target_include_directories(dnes PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/include)
