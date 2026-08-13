cmake_minimum_required(VERSION 3.16)

project(hertharian-engine VERSION 0.1.6 LANGUAGES C)

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
    src/common/math.c
    src/common/camera.c
    src/common/fps_camera_controller.c
    src/common/timing.c
    src/input/input.c
    src/platform/mouse_source.c
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
if(NOT WIN32)
    target_link_libraries(hertharian-engine PRIVATE m)
endif()
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

    add_executable(math_tests
        tests/math_tests.c
        src/common/math.c
    )
    target_include_directories(math_tests PRIVATE include)
    target_compile_features(math_tests PRIVATE c_std_11)
    set_target_properties(math_tests PROPERTIES C_EXTENSIONS OFF)

    add_executable(camera_tests
        tests/camera_tests.c
        src/common/camera.c
        src/common/math.c
    )
    target_include_directories(camera_tests PRIVATE include)
    target_compile_features(camera_tests PRIVATE c_std_11)
    set_target_properties(camera_tests PROPERTIES C_EXTENSIONS OFF)

    add_executable(fps_camera_controller_tests
        tests/fps_camera_controller_tests.c
        src/common/fps_camera_controller.c
        src/common/camera.c
        src/common/math.c
        src/input/input.c
    )
    target_include_directories(fps_camera_controller_tests PRIVATE
        include
        src/common
        src/input
    )
    target_compile_features(fps_camera_controller_tests PRIVATE c_std_11)
    set_target_properties(fps_camera_controller_tests PROPERTIES
        C_EXTENSIONS OFF
    )

    add_executable(mouse_source_tests
        tests/mouse_source_tests.c
        src/platform/mouse_source.c
    )
    target_include_directories(mouse_source_tests PRIVATE src/platform)
    target_compile_features(mouse_source_tests PRIVATE c_std_11)
    set_target_properties(mouse_source_tests PROPERTIES C_EXTENSIONS OFF)

    if(NOT WIN32)
        target_link_libraries(math_tests PRIVATE m)
        target_link_libraries(camera_tests PRIVATE m)
        target_link_libraries(fps_camera_controller_tests PRIVATE m)
    endif()

    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(input_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
        target_compile_options(timing_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
        target_compile_options(math_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
        target_compile_options(camera_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
        target_compile_options(fps_camera_controller_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
        target_compile_options(mouse_source_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
    endif()

    add_test(NAME input_state COMMAND input_tests)
    add_test(NAME timing_state COMMAND timing_tests)
    add_test(NAME math_foundation COMMAND math_tests)
    add_test(NAME camera_foundation COMMAND camera_tests)
    add_test(NAME fps_camera_controller COMMAND fps_camera_controller_tests)
    add_test(NAME relative_mouse_source COMMAND mouse_source_tests)
endif()
