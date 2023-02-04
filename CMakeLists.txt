cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 11)

project(tiny_thread_demo LANGUAGES CXX)

include_directories(src)

add_subdirectory(src)

add_executable(demo demo.cc)
target_link_libraries(demo tiny_pool)
