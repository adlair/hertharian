cmake_minimum_required(VERSION 3.16)

project(gf-engine VERSION 0.1.0 LANGUAGES C)

configure_file(
    cmake/gf_version.h.in
    generated/gf_version.h
    @ONLY
)

add_executable(gf-engine
    src/main.c
    src/common/engine.c
)

target_include_directories(gf-engine PRIVATE
    include
    "${PROJECT_BINARY_DIR}/generated"
)
target_compile_features(gf-engine PRIVATE c_std_11)
set_target_properties(gf-engine PROPERTIES
    C_EXTENSIONS OFF
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(gf-engine PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
    )
endif()

include(CTest)

if(BUILD_TESTING)
    add_test(NAME engine_lifecycle_frames_3
        COMMAND gf-engine --frames 3
    )
    set_tests_properties(engine_lifecycle_frames_3 PROPERTIES
        PASS_REGULAR_EXPRESSION "Frame 0.*Frame 1.*Frame 2.*Engine shutdown complete\."
    )
endif()
