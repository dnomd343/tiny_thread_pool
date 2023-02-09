cmake_minimum_required(VERSION 3.0)

set(CMAKE_C_STANDARD 99)

project(tiny_pool_demo_c LANGUAGES C)

include_directories(../..)

add_executable(demo_c demo.c)
target_link_libraries(demo_c tiny_pool)
