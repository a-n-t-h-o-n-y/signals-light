cmake_minimum_required(VERSION 3.14)

add_executable(tests EXCLUDE_FROM_ALL
               tests.main.cpp
               signal.test.cpp
               )

target_link_libraries(tests PRIVATE Catch2::Catch2 signals-light)
