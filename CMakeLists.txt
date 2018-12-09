cmake_minimum_required(VERSION 3.7)

project(Fractals)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(Fractals main.cpp)
target_link_libraries(Fractals ${SDL2_LIBRARIES})
