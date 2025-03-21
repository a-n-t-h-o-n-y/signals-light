# Test Library Dependency
include(FetchContent)
FetchContent_Declare(
    zzz
    GIT_REPOSITORY https://github.com/a-n-t-h-o-n-y/zzz.git
    GIT_TAG dcf3a15309e9919246e525a0a5c12108b6876380
)
FetchContent_MakeAvailable(zzz)

add_executable(signals_light_tests EXCLUDE_FROM_ALL
    signal.test.cpp
)

target_link_libraries(signals_light_tests
    PRIVATE
        signals-light
        zzz
)

target_compile_options(signals_light_tests
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)
