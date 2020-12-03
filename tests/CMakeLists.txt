cmake_minimum_required(VERSION 3.14)

add_executable(signals_light_tests EXCLUDE_FROM_ALL
    tests.main.cpp
    signal.test.cpp
)

target_link_libraries(signals_light_tests PUBLIC Catch2::Catch2 signals-light)
