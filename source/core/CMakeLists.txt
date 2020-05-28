cmake_minimum_required(VERSION 3.10.2)

project(dnes)

include_directories(mappers audio video .)

add_library(dnes SHARED
    nes.cc
    cpu.cc
    ppu.cc
    apu.cc
    cart.cc
    ines_file.cc
    mappers/mapper_base.cc
    mappers/nrom.cc
    mappers/mmc1.cc
    mappers/uxrom.cc
    mappers/cnrom.cc
    mappers/mmc3.cc
    audio/alsa_backend.cc
    video/gl_util.cc
    video/opengl_backend.cc
    video/igl_platform.cc
    video/glx_platform.cc
)

set_target_properties(dnes PROPERTIES CXX_VISIBILITY_PRESET hidden)

target_link_libraries(dnes PRIVATE ${ALSA_LIBRARIES} ${OPENGL_LIBRARIES} spdlog)
target_include_directories(dnes PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/include)
