cmake_minimum_required(VERSION 3.7)

project(Fractals)

find_package(SDL2 REQUIRED)
find_package(Threads REQUIRED)

add_executable(fractals main.cpp)
target_include_directories(fractals PUBLIC ${SDL2_INCLUDE_DIRS})
target_link_libraries(fractals Threads::Threads ${SDL2_LIBRARIES})
set_target_properties(fractals PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)
