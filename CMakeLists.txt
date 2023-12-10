cmake_minimum_required(VERSION 3.27)
project(mc_viewer C)

set(CMAKE_C_STANDARD 11)

add_subdirectory(cunk)
include_directories(${PROJECT_SOURCE_DIR}/include)
add_subdirectory(enklume)
add_subdirectory(mc_viewer)