cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 11)

project(tiny_pool_demo_cpp LANGUAGES CXX)

include_directories(../..)

add_executable(demo_cpp demo.cc)
target_link_libraries(demo_cpp tiny_pool)
