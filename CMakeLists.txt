# CMakeLists.txt for wormhole generator project

cmake_minimum_required(VERSION 3.27)

project(WormholeGenerator)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(IMZADI_BUILD_GAMES OFF)
set(IMZADI_BUILD_TOOLS OFF)
set(IMZADI_BUILD_SHARED OFF)
set(AUDIODATALIB_BUILD_SHARED OFF)
set(HAPPYMATH_BUILD_TEST OFF)

add_subdirectory(Library)
add_subdirectory(Test)
add_subdirectory(Simulation)
add_subdirectory(Dependencies/Imzadi)
add_subdirectory(Dependencies/ParseParty)