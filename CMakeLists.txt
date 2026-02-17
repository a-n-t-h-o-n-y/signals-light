cmake_minimum_required(VERSION 3.16)
project(SignalsLight LANGUAGES CXX)

add_library(signals-light INTERFACE)

option(SIGNALS_LIGHT_BUILD_TESTS "Build signals-light tests" OFF)

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

if(SIGNALS_LIGHT_BUILD_TESTS)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(tests)
    endif()
endif()
