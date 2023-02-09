cmake_minimum_required(VERSION 3.0)

set(CMAKE_C_STANDARD 99)

project(tiny_thread_pool LANGUAGES C)

add_library(tiny_pool STATIC tiny_pool.c)
target_link_libraries(tiny_pool pthread)

add_subdirectory(demo/c)
add_subdirectory(demo/cpp)
