cmake_minimum_required(VERSION 3.14)
project(Signals_light LANGUAGES CXX)

add_library(signals-light INTERFACE)

target_include_directories(signals-light
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(${CMAKE_VERSION} VERSION_LESS "3.8")
    set(CMAKE_CXX_STANDARD 17)
else()
    target_compile_features(signals-light INTERFACE cxx_std_17)
endif()

target_compile_options(signals-light INTERFACE -Wall -Wextra)

if (NOT TARGET Catch2)
    add_subdirectory(external/Catch2)
endif()

add_subdirectory(tests)
