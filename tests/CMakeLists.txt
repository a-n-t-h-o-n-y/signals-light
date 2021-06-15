cmake_minimum_required(VERSION 3.9)

add_executable(signals_light_tests EXCLUDE_FROM_ALL
    tests.main.cpp
    signal.test.cpp
)

target_link_libraries(signals_light_tests
    PRIVATE
        Catch2::Catch2
        signals-light
)

target_compile_options(signals_light_tests
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

