cmake_minimum_required(VERSION 3.16)

project(hertharian-engine VERSION 0.3.4 LANGUAGES C)

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)
find_package(OpenGL REQUIRED)
find_package(X11 QUIET)

configure_file(
    cmake/hth_version.h.in
    generated/hth_version.h
    @ONLY
)

get_filename_component(
    HTH_DEVELOPMENT_RESOURCE_ROOT
    "${PROJECT_SOURCE_DIR}/../assets"
    REALPATH
)
file(TO_CMAKE_PATH
    "${HTH_DEVELOPMENT_RESOURCE_ROOT}"
    HTH_DEVELOPMENT_RESOURCE_ROOT
)
configure_file(
    cmake/hth_resource_config.h.in
    generated/hth_resource_config.h
    @ONLY
)

add_executable(hertharian-engine
    src/main.c
    src/actor/actor.c
    src/common/engine.c
    src/common/math.c
    src/common/camera.c
    src/common/fps_camera_controller.c
    src/common/runtime_options.c
    src/common/timing.c
    src/collision/aabb.c
    src/collision/collision_trace.c
    src/collision/collision_world.c
    src/entity/entity.c
    src/gameplay/actor_spawn.c
    src/gameplay/damage_intent.c
    src/gameplay/enemy.c
    src/gameplay/health.c
    src/input/input.c
    src/geometry/geometry.c
    src/image/image.c
    src/level/level.c
    src/level/level_selection.c
    src/material/bootstrap_materials.c
    src/material/material.c
    src/player/player_body.c
    src/player/player_locomotion.c
    src/player/player_movement.c
    src/physics/dynamic_body.c
    src/physics/dynamic_collision.c
    src/resource/resource.c
    src/spatial/spatial.c
    src/platform/keyboard_reconciliation.c
    src/platform/mouse_source.c
    src/platform/platform_sdl3.c
    src/renderer/renderer.c
    src/renderer/renderer_opengl.c
    src/view/view_dynamics.c
    src/world/world.c
)

target_include_directories(hertharian-engine PRIVATE
    include
    src/actor
    src/common
    src/collision
    src/entity
    src/gameplay
    src/input
    src/geometry
    src/image
    src/level
    src/material
    src/platform
    src/player
    src/physics
    src/renderer
    src/resource
    src/spatial
    src/view
    src/world
    "${PROJECT_BINARY_DIR}/generated"
)
target_link_libraries(hertharian-engine PRIVATE SDL3::SDL3 OpenGL::GL)
if(X11_FOUND)
    target_sources(hertharian-engine PRIVATE src/platform/platform_x11.c)
    target_link_libraries(hertharian-engine PRIVATE X11::X11)
    target_compile_definitions(hertharian-engine PRIVATE
        HTH_HAVE_X11_OBSERVER=1
    )
endif()
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

    add_executable(entity_tests
        tests/entity_tests.c
        src/entity/entity.c
    )
    target_include_directories(entity_tests PRIVATE src/entity)
    target_compile_definitions(entity_tests PRIVATE HTH_ENTITY_TESTING=1)
    target_compile_features(entity_tests PRIVATE c_std_11)
    set_target_properties(entity_tests PROPERTIES C_EXTENSIONS OFF)

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

    add_executable(spatial_tests
        tests/spatial_tests.c
        src/spatial/spatial.c
        src/entity/entity.c
    )
    target_include_directories(spatial_tests PRIVATE
        include src/entity src/spatial
    )
    target_compile_features(spatial_tests PRIVATE c_std_11)
    set_target_properties(spatial_tests PROPERTIES C_EXTENSIONS OFF)

    add_executable(dynamic_body_tests
        tests/dynamic_body_tests.c
        src/physics/dynamic_body.c
        src/spatial/spatial.c
        src/entity/entity.c
    )
    target_include_directories(dynamic_body_tests PRIVATE
        include src/entity src/physics src/spatial
    )
    target_compile_features(dynamic_body_tests PRIVATE c_std_11)
    set_target_properties(dynamic_body_tests PROPERTIES C_EXTENSIONS OFF)

    add_executable(dynamic_collision_tests
        tests/dynamic_collision_tests.c
        src/physics/dynamic_body.c
        src/physics/dynamic_collision.c
        src/spatial/spatial.c
        src/entity/entity.c
        src/collision/aabb.c
        src/collision/collision_trace.c
        src/collision/collision_world.c
        src/common/math.c
        src/world/world.c
    )
    target_include_directories(dynamic_collision_tests PRIVATE
        include src/collision src/entity src/physics src/spatial src/world
    )
    target_compile_features(dynamic_collision_tests PRIVATE c_std_11)
    set_target_properties(dynamic_collision_tests PROPERTIES C_EXTENSIONS OFF)

    add_executable(actor_tests
        tests/actor_tests.c
        src/actor/actor.c
        src/physics/dynamic_body.c
        src/physics/dynamic_collision.c
        src/spatial/spatial.c
        src/entity/entity.c
        src/collision/aabb.c
        src/collision/collision_trace.c
        src/collision/collision_world.c
        src/common/math.c
        src/world/world.c
    )
    target_include_directories(actor_tests PRIVATE
        include src/actor src/collision src/entity src/physics src/spatial
        src/world
    )
    target_compile_features(actor_tests PRIVATE c_std_11)
    set_target_properties(actor_tests PROPERTIES C_EXTENSIONS OFF)

    add_executable(health_tests
        tests/health_tests.c
        src/gameplay/health.c
        src/actor/actor.c
        src/physics/dynamic_body.c
        src/spatial/spatial.c
        src/entity/entity.c
    )
    target_include_directories(health_tests PRIVATE
        include src/actor src/entity src/gameplay src/physics src/spatial
    )
    target_compile_features(health_tests PRIVATE c_std_11)
    set_target_properties(health_tests PROPERTIES C_EXTENSIONS OFF)

    add_executable(damage_intent_tests
        tests/damage_intent_tests.c
        src/gameplay/damage_intent.c
        src/gameplay/health.c
        src/actor/actor.c
        src/physics/dynamic_body.c
        src/spatial/spatial.c
        src/entity/entity.c
    )
    target_include_directories(damage_intent_tests PRIVATE
        include src/actor src/entity src/gameplay src/physics src/spatial
    )
    target_compile_features(damage_intent_tests PRIVATE c_std_11)
    set_target_properties(damage_intent_tests PROPERTIES C_EXTENSIONS OFF)

    add_executable(actor_spawn_tests
        tests/actor_spawn_tests.c
        src/gameplay/actor_spawn.c
        src/gameplay/damage_intent.c
        src/gameplay/health.c
        src/actor/actor.c
        src/physics/dynamic_body.c
        src/spatial/spatial.c
        src/entity/entity.c
    )
    target_include_directories(actor_spawn_tests PRIVATE
        include src/actor src/entity src/gameplay src/physics src/spatial
    )
    target_compile_features(actor_spawn_tests PRIVATE c_std_11)
    set_target_properties(actor_spawn_tests PROPERTIES C_EXTENSIONS OFF)

    add_executable(enemy_tests
        tests/enemy_tests.c
        src/gameplay/actor_spawn.c
        src/gameplay/enemy.c
        src/gameplay/health.c
        src/actor/actor.c
        src/physics/dynamic_body.c
        src/spatial/spatial.c
        src/entity/entity.c
    )
    target_include_directories(enemy_tests PRIVATE
        include src/actor src/entity src/gameplay src/physics src/spatial
    )
    target_compile_features(enemy_tests PRIVATE c_std_11)
    set_target_properties(enemy_tests PROPERTIES C_EXTENSIONS OFF)

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
        src/platform/mouse_source.c
    )
    target_include_directories(fps_camera_controller_tests PRIVATE
        include
        src/common
        src/input
        src/platform
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

    add_executable(keyboard_reconciliation_tests
        tests/keyboard_reconciliation_tests.c
        src/platform/keyboard_reconciliation.c
    )
    target_include_directories(keyboard_reconciliation_tests PRIVATE
        include src/platform
    )
    target_compile_features(keyboard_reconciliation_tests PRIVATE c_std_11)
    set_target_properties(keyboard_reconciliation_tests PROPERTIES
        C_EXTENSIONS OFF
    )

    if(X11_FOUND)
        add_executable(platform_x11_tests
            tests/platform_x11_tests.c
            src/platform/platform_x11.c
        )
        target_include_directories(platform_x11_tests PRIVATE src/platform)
        target_link_libraries(platform_x11_tests PRIVATE X11::X11)
        target_compile_features(platform_x11_tests PRIVATE c_std_11)
        set_target_properties(platform_x11_tests PROPERTIES C_EXTENSIONS OFF)
    endif()

    add_executable(player_body_tests
        tests/player_body_tests.c
        src/player/player_body.c
        src/collision/aabb.c
        src/common/camera.c
        src/common/math.c
    )
    target_include_directories(player_body_tests PRIVATE
        include src/collision src/player
    )
    target_compile_features(player_body_tests PRIVATE c_std_11)
    set_target_properties(player_body_tests PROPERTIES C_EXTENSIONS OFF)

    add_executable(player_movement_tests
        tests/player_movement_tests.c
        src/player/player_movement.c
        src/player/player_body.c
        src/player/player_locomotion.c
        src/collision/aabb.c
        src/collision/collision_trace.c
        src/collision/collision_world.c
        src/world/world.c
        src/input/input.c
        src/common/math.c
        src/platform/keyboard_reconciliation.c
    )
    target_include_directories(player_movement_tests PRIVATE
        include src/common src/collision src/input src/platform src/player
        src/world
    )
    target_compile_features(player_movement_tests PRIVATE c_std_11)
    set_target_properties(player_movement_tests PROPERTIES C_EXTENSIONS OFF)

    add_executable(locomotion_tests
        tests/locomotion_tests.c
        src/player/player_locomotion.c
        src/player/player_body.c
        src/collision/aabb.c
        src/common/math.c
    )
    target_include_directories(locomotion_tests PRIVATE
        include src/collision src/player
    )
    target_compile_features(locomotion_tests PRIVATE c_std_11)
    set_target_properties(locomotion_tests PROPERTIES C_EXTENSIONS OFF)

    add_executable(collision_tests
        tests/collision_tests.c
        src/collision/aabb.c
        src/collision/collision_world.c
        src/world/world.c
    )
    target_include_directories(collision_tests PRIVATE
        include src/collision src/world
    )
    target_compile_features(collision_tests PRIVATE c_std_11)
    set_target_properties(collision_tests PROPERTIES C_EXTENSIONS OFF)

    add_executable(world_tests
        tests/world_tests.c
        src/world/world.c
        src/collision/aabb.c
        src/collision/collision_trace.c
        src/collision/collision_world.c
        src/common/math.c
        src/player/player_body.c
    )
    target_include_directories(world_tests PRIVATE
        include src/collision src/player src/world
    )
    target_compile_features(world_tests PRIVATE c_std_11)
    set_target_properties(world_tests PROPERTIES C_EXTENSIONS OFF)

    add_executable(resource_tests
        tests/resource_tests.c
        src/resource/resource.c
    )
    target_include_directories(resource_tests PRIVATE src/resource)
    target_compile_features(resource_tests PRIVATE c_std_11)
    set_target_properties(resource_tests PROPERTIES C_EXTENSIONS OFF)

    add_executable(level_tests
        tests/level_tests.c
        src/level/level.c
        src/resource/resource.c
        src/world/world.c
        src/collision/aabb.c
        src/collision/collision_trace.c
        src/collision/collision_world.c
        src/player/player_body.c
        src/common/math.c
    )
    target_include_directories(level_tests PRIVATE
        include src/collision src/level src/player src/resource src/world
        "${PROJECT_BINARY_DIR}/generated"
    )
    target_compile_features(level_tests PRIVATE c_std_11)
    set_target_properties(level_tests PROPERTIES C_EXTENSIONS OFF)

    add_executable(level_selection_tests
        tests/level_selection_tests.c
        src/common/runtime_options.c
        src/level/level_selection.c
        src/level/level.c
        src/resource/resource.c
        src/world/world.c
        src/collision/aabb.c
        src/collision/collision_trace.c
        src/collision/collision_world.c
        src/player/player_body.c
        src/common/math.c
    )
    target_include_directories(level_selection_tests PRIVATE
        include src/common src/collision src/level src/player src/resource
        src/world
        "${PROJECT_BINARY_DIR}/generated"
    )
    target_compile_features(level_selection_tests PRIVATE c_std_11)
    set_target_properties(level_selection_tests PROPERTIES C_EXTENSIONS OFF)

    add_executable(material_tests
        tests/material_tests.c
        src/material/bootstrap_materials.c
        src/material/material.c
        src/image/image.c
        src/resource/resource.c
    )
    target_include_directories(material_tests PRIVATE
        include src/collision src/image src/material src/resource src/world
        "${PROJECT_BINARY_DIR}/generated"
    )
    target_compile_features(material_tests PRIVATE c_std_11)
    set_target_properties(material_tests PROPERTIES C_EXTENSIONS OFF)

    add_executable(image_tests
        tests/image_tests.c
        src/image/image.c
    )
    target_include_directories(image_tests PRIVATE src/image)
    target_compile_features(image_tests PRIVATE c_std_11)
    set_target_properties(image_tests PROPERTIES C_EXTENSIONS OFF)

    add_executable(geometry_tests
        tests/geometry_tests.c
        src/geometry/geometry.c
    )
    target_include_directories(geometry_tests PRIVATE src/geometry)
    target_compile_features(geometry_tests PRIVATE c_std_11)
    set_target_properties(geometry_tests PROPERTIES C_EXTENSIONS OFF)

    add_executable(view_dynamics_tests
        tests/view_dynamics_tests.c
        src/view/view_dynamics.c
        src/common/math.c
    )
    target_include_directories(view_dynamics_tests PRIVATE
        include src/view
    )
    target_compile_features(view_dynamics_tests PRIVATE c_std_11)
    set_target_properties(view_dynamics_tests PROPERTIES C_EXTENSIONS OFF)

    add_executable(collision_trace_tests
        tests/collision_trace_tests.c
        src/collision/aabb.c
        src/collision/collision_trace.c
        src/collision/collision_world.c
        src/common/math.c
        src/world/world.c
    )
    target_include_directories(collision_trace_tests PRIVATE
        include src/collision src/world
    )
    target_compile_features(collision_trace_tests PRIVATE c_std_11)
    set_target_properties(collision_trace_tests PROPERTIES C_EXTENSIONS OFF)

    if(NOT WIN32)
        target_link_libraries(math_tests PRIVATE m)
        target_link_libraries(camera_tests PRIVATE m)
        target_link_libraries(fps_camera_controller_tests PRIVATE m)
        target_link_libraries(player_body_tests PRIVATE m)
        target_link_libraries(player_movement_tests PRIVATE m)
        target_link_libraries(locomotion_tests PRIVATE m)
        target_link_libraries(collision_trace_tests PRIVATE m)
        target_link_libraries(view_dynamics_tests PRIVATE m)
        target_link_libraries(collision_tests PRIVATE m)
        target_link_libraries(world_tests PRIVATE m)
        target_link_libraries(level_tests PRIVATE m)
        target_link_libraries(level_selection_tests PRIVATE m)
        target_link_libraries(material_tests PRIVATE m)
        target_link_libraries(geometry_tests PRIVATE m)
        target_link_libraries(dynamic_body_tests PRIVATE m)
        target_link_libraries(dynamic_collision_tests PRIVATE m)
        target_link_libraries(actor_tests PRIVATE m)
        target_link_libraries(health_tests PRIVATE m)
        target_link_libraries(damage_intent_tests PRIVATE m)
        target_link_libraries(actor_spawn_tests PRIVATE m)
        target_link_libraries(enemy_tests PRIVATE m)
    endif()

    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(input_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
        target_compile_options(entity_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
        target_compile_options(timing_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
        target_compile_options(spatial_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
        target_compile_options(dynamic_body_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
        target_compile_options(dynamic_collision_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
        target_compile_options(actor_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
        target_compile_options(health_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
        target_compile_options(damage_intent_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
        target_compile_options(actor_spawn_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
        target_compile_options(enemy_tests PRIVATE
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
        target_compile_options(keyboard_reconciliation_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
        if(X11_FOUND)
            target_compile_options(platform_x11_tests PRIVATE
                -Wall -Wextra -Wpedantic -Wconversion -Wshadow
            )
        endif()
        target_compile_options(player_body_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
        target_compile_options(player_movement_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
        target_compile_options(locomotion_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
        target_compile_options(collision_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
        target_compile_options(collision_trace_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
        target_compile_options(view_dynamics_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
        target_compile_options(world_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
        target_compile_options(resource_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
        target_compile_options(level_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
        target_compile_options(level_selection_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
        target_compile_options(material_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
        target_compile_options(image_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
        target_compile_options(geometry_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        )
    endif()

    add_test(NAME input_state COMMAND input_tests)
    add_test(NAME entity_foundation COMMAND entity_tests)
    add_test(NAME timing_state COMMAND timing_tests)
    add_test(NAME spatial_entity_foundation COMMAND spatial_tests)
    add_test(NAME dynamic_body_foundation COMMAND dynamic_body_tests)
    add_test(NAME dynamic_collision_foundation COMMAND dynamic_collision_tests)
    add_test(NAME actor_foundation COMMAND actor_tests)
    add_test(NAME health_damage_foundation COMMAND health_tests)
    add_test(NAME damage_intent_foundation COMMAND damage_intent_tests)
    add_test(NAME actor_spawn_foundation COMMAND actor_spawn_tests)
    add_test(NAME enemy_foundation COMMAND enemy_tests)
    add_test(NAME math_foundation COMMAND math_tests)
    add_test(NAME camera_foundation COMMAND camera_tests)
    add_test(NAME fps_camera_controller COMMAND fps_camera_controller_tests)
    add_test(NAME relative_mouse_source COMMAND mouse_source_tests)
    add_test(NAME keyboard_reconciliation COMMAND keyboard_reconciliation_tests)
    if(X11_FOUND)
        add_test(NAME platform_x11_observer COMMAND platform_x11_tests)
    endif()
    add_test(NAME player_body_foundation COMMAND player_body_tests)
    add_test(NAME player_movement_foundation COMMAND player_movement_tests)
    add_test(NAME locomotion_foundation COMMAND locomotion_tests)
    add_test(NAME collision_foundation COMMAND collision_tests)
    add_test(NAME collision_trace_foundation COMMAND collision_trace_tests)
    add_test(NAME view_dynamics_foundation COMMAND view_dynamics_tests)
    add_test(NAME world_representation_foundation COMMAND world_tests)
    add_test(NAME resource_foundation COMMAND resource_tests)
    add_test(NAME level_loading_foundation COMMAND level_tests)
    add_test(NAME level_selection_foundation COMMAND level_selection_tests)
    add_test(NAME material_foundation COMMAND material_tests)
    add_test(NAME image_loading_foundation COMMAND image_tests)
    add_test(NAME geometry_foundation COMMAND geometry_tests)
endif()
