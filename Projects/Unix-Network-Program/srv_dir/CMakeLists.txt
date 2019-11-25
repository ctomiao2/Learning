cmake_minimum_required(VERSION 2.8)
add_definitions(-std=c++11)
set(CMAKE_C_FLAGS "-w")
set(CMAKE_C_FLAGS "-g")
aux_source_directory(. DIR_LIB_SRCS)
add_library(srv ${DIR_LIB_SRCS})

