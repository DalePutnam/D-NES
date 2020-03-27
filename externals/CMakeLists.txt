cmake_minimum_required(VERSION 3.10.2)

add_subdirectory(spdlog)
set_property(TARGET spdlog PROPERTY POSITION_INDEPENDENT_CODE ON)