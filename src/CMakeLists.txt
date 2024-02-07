cmake_minimum_required(VERSION 3.27)
project(test C)

set(CMAKE_C_STANDARD 11)

add_executable(test vmdLoader.c)
target_link_libraries(test json-c)
include_directories(/usr/include/python3.11)

