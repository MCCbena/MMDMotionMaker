cmake_minimum_required(VERSION 3.27)
project(test C)

set(CMAKE_C_STANDARD 11)

add_executable(test modelLoader.c)
target_link_libraries(test json-c)
