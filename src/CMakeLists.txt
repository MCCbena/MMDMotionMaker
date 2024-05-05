cmake_minimum_required(VERSION 3.27)
project(test C)

set(CMAKE_C_STANDARD 11)

add_executable(test
        vmd/vmdLoader.c
        vmd/vmdStruct.h
        vmd/vmdLoader.c
        vmd/indexlib.c
        pmx/modelLoader.c
        pmx/pmxStruct.h)
target_link_libraries(test json-c m)
include_directories(/usr/include/python3.11, vmd)

