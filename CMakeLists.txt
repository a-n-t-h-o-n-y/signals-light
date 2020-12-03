cmake_minimum_required(VERSION 3.14)
project(Signals_light)

if (NOT TARGET Catch2)
    add_subdirectory(external/Catch2)
endif()

add_library(signals-light INTERFACE)
target_compile_features(signals-light INTERFACE cxx_std_17)
target_compile_options(signals-light INTERFACE -Wall -Wextra)
target_include_directories(signals-light INTERFACE include/)

add_subdirectory(tests)
