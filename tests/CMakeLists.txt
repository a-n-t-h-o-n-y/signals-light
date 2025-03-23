# Test Library Dependency
include(FetchContent)
FetchContent_Declare(
    zzz
    GIT_REPOSITORY https://github.com/a-n-t-h-o-n-y/zzz.git
    GIT_TAG 9d7c047f47c81a95a5ea824075253618356593a2
)
FetchContent_MakeAvailable(zzz)

add_executable(signals_light.tests.unit EXCLUDE_FROM_ALL
    signal.test.cpp
)

target_link_libraries(signals_light.tests.unit
    PRIVATE
        signals-light
        zzz
)

target_compile_options(signals_light.tests.unit
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

message(STATUS "AddressSanitizer Enabled for signals_light.tests.unit")
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(signals_light.tests.unit
        PRIVATE
            -fsanitize=address
            -fno-omit-frame-pointer
    )
    target_link_options(signals_light.tests.unit
        PRIVATE
            -fsanitize=address
    )
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(signals_light.tests.unit
        PRIVATE
            /fsanitize=address
    )
    target_link_options(signals_light.tests.unit
        PRIVATE
            /INCREMENTAL:NO
            /DEBUG
            /fsanitize=address
    )
endif()