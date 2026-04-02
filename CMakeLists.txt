cmake_minimum_required(VERSION 3.31)
project(DataAgent_demo)

set(CMAKE_CXX_STANDARD 20)

add_executable(DataAgent_demo main.cpp
        Storage/*.cpp
        Storage/*.h)
