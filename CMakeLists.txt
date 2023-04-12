cmake_minimum_required(VERSION 3.0)
project(tiny_thread_pool LANGUAGES C)
set(CMAKE_C_STANDARD 99)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

option(BUILD_DYN "build shared library" OFF)
if (BUILD_DYN)
    add_compile_options(-fPIC)
    add_library(tiny_pool SHARED tiny_pool.c)
else()
    add_library(tiny_pool STATIC tiny_pool.c)
endif()

target_link_libraries(tiny_pool pthread)
