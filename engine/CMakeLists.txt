cmake_minimum_required(VERSION 3.16)

project(hertharian-engine VERSION 0.1.4 LANGUAGES C)

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)
find_package(OpenGL REQUIRED)

configure_file(
    cmake/hth_version.h.in
    generated/hth_version.h
    @ONLY
)

add_executable(hertharian-engine
    src/main.c
    src/common/engine.c
    src/common/timing.c
    src/input/input.c
    src/platform/platform_sdl3.c
    src/renderer/renderer.c
    src/renderer/renderer_opengl.c
)

target_include_directories(hertharian-engine PRIVATE
    include
    src/common
    src/input
    src/platform
    src/renderer
    "${PROJECT_BINARY_DIR}/generated"
)
target_link_libraries(hertharian-engine PRIVATE SDL3::SDL3 OpenGL::GL)
target_compile_features(hertharian-engine PRIVATE c_std_11)
set_target_properties(hertharian-engine PROPERTIES
    C_EXTENSIONS OFF
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(hertharian-engine PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
    )
endif()

include(CTest)

if(BUILD_TESTING)
    add_test(NAME platform_lifecycle_frames_3
        COMMAND hertharian-engine --headless --frames 3
    )
    set_tests_properties(platform_lifecycle_frames_3 PROPERTIES
        PASS_REGULAR_EXPRESSION "Headless mode\..*Renderer disabled\..*Frame 0.*Frame 1.*Frame 2.*Platform shutdown complete\..*Engine shutdown complete\."
    )

    add_executable(input_tests
        tests/input_tests.c
        src/input/input.c
    )
    target_include_directories(input_tests PRIVATE
        include
        src/common
        src/input
    )
    target_compile_features(input_tests PRIVATE c_std_11)
    set_target_properties(input_tests PROPERTIES C_EXTENSIONS OFF)

    add_executable(timing_tests
        tests/timing_tests.c
        src/common/timing.c
    )
    target_include_directories(timing_tests PRIVATE
        include
        src/common
    )
    target_compile_features(timing_tests PRIVATE c_std_11)
    set_target_properties(timing_tests PROPERTIES C_EXTENSIONS OFF)

    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(input_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
        target_compile_options(timing_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
    endif()

    add_test(NAME input_state COMMAND input_tests)
    add_test(NAME timing_state COMMAND timing_tests)
endif()
