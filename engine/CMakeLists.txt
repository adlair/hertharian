cmake_minimum_required(VERSION 3.16)

project(hertharian-engine VERSION 0.1.1 LANGUAGES C)

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)

configure_file(
    cmake/hth_version.h.in
    generated/hth_version.h
    @ONLY
)

add_executable(hertharian-engine
    src/main.c
    src/common/engine.c
    src/platform/platform_sdl3.c
)

target_include_directories(hertharian-engine PRIVATE
    include
    src/platform
    "${PROJECT_BINARY_DIR}/generated"
)
target_link_libraries(hertharian-engine PRIVATE SDL3::SDL3)
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
        COMMAND hertharian-engine --frames 3
    )
    set_tests_properties(platform_lifecycle_frames_3 PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy"
        PASS_REGULAR_EXPRESSION "Initializing platform.*Platform initialized\..*Frame 0.*Frame 1.*Frame 2.*Platform shutdown complete\..*Engine shutdown complete\."
    )
endif()
