cmake_minimum_required(VERSION 3.9)
project(Signals_light LANGUAGES CXX)

add_library(signals-light INTERFACE)

target_include_directories(signals-light
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(signals-light
    INTERFACE
        cxx_std_17
)

include(GNUInstallDirs)
install(
    DIRECTORY
        ${PROJECT_SOURCE_DIR}/include/signals_light
    DESTINATION
        ${CMAKE_INSTALL_INCLUDEDIR}
)

if (NOT TARGET Catch2)
    add_subdirectory(external/Catch2)
endif()

add_subdirectory(tests)
