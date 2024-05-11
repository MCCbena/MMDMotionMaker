cmake_minimum_required(VERSION 3.27)
project(test C)

set(CMAKE_C_STANDARD 11)

add_executable(test
        vmd/vmdLoader.c
        vmd/vmdStruct.h
        pmx/modelLoader.c
        wrapper.c
        vmd/indexlib.c)
target_link_libraries(test json-c m)
include_directories(/usr/include/python3.11, vmd)

