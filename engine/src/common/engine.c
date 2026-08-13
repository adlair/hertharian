#include "hth_engine.h"
#include "hth_camera.h"
#include "hth_input.h"
#include "hth_timing.h"
#include "hth_version.h"
#include "fps_camera_controller.h"
#include "collision_trace.h"
#include "collision_world.h"
#include "input_internal.h"
#include "platform.h"
#include "player_body.h"
#include "player_movement.h"
#include "renderer.h"
#include "timing_internal.h"
#include "view_dynamics.h"

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

struct HTHEnginePhysicalState {
    HTHPlayerBody body;
    HTHMovementConfig movement_config;
    HTHCollisionWorld collision_world;
};

struct HTHEngineViewState {
    HTHViewDynamicsState dynamics;
    HTHViewDynamicsConfig config;
    float base_vertical_fov_radians;
};

static bool physical_state_spawn_is_clear(
    const struct HTHEnginePhysicalState *state)
{
    HTHTrace trace;
    HTHVec3 mins = hth_vec3(-state->body.half_width, 0.0F,
                            -state->body.half_width);
    HTHVec3 maxs = hth_vec3(state->body.half_width, state->body.height,
                            state->body.half_width);

    return hth_collision_world_trace_aabb(
               &state->collision_world, state->body.position,
               state->body.position, mins, maxs, &trace) &&
           !trace.start_solid;
}

static void destroy_physical_state(HTHEngine *engine)
{
    free(engine->physical_state);
    engine->physical_state = NULL;
}

static void destroy_view_state(HTHEngine *engine)
{
    free(engine->view_state);
    engine->view_state = NULL;
}

static void clear_debugged_mouse_delta(HTHEngine *engine, const char *reason)
{
    double delta_x;
    double delta_y;

    if (engine->debug_fps_input) {
        hth_input_mouse_delta(engine->input, &delta_x, &delta_y);
        printf("FPS input: mouse delta clear (%s) dx=%.17g dy=%.17g\n",
               reason, delta_x, delta_y);
    }
    hth_input_clear_mouse_delta(engine->input);
}

static void update_mouse_capture(HTHEngine *engine)
{
    HTHFPSCaptureAction action = hth_fps_camera_controller_capture_action(
        engine->camera_controller, engine->input);
    bool enable;

    if (action == HTH_FPS_CAPTURE_NONE || engine->headless) {
        return;
    }
    enable = action == HTH_FPS_CAPTURE_ENABLE;
    if (engine->debug_fps_input) {
        printf("FPS input: capture request: %s\n",
               enable ? "enable" : "disable");
    }
    if (!hth_platform_set_relative_mouse_mode(engine->platform, enable)) {
        return;
    }
    hth_fps_camera_controller_set_capture(engine->camera_controller, enable);
    clear_debugged_mouse_delta(engine, enable ? "capture enable"
                                              : "capture disable");
    hth_input_begin_capture_transition_discard(engine->input);
    if (engine->debug_fps_input) {
        printf("FPS input: capture confirmed: %s\n",
               enable ? "enabled" : "disabled");
    }
    puts(enable ? "FPS mouse capture enabled."
                : "FPS mouse capture disabled.");
}

const char *hth_engine_version(void)
{
    return HTH_ENGINE_VERSION;
}

bool hth_engine_init(HTHEngine *engine, const HTHEngineConfig *config)
{
    const char *window_title;
    uint32_t window_width;
    uint32_t window_height;
    uint64_t counter;
    uint64_t frequency;
    HTHPlatformConfig platform_config;

    if (engine == NULL || config == NULL) {
        return false;
    }

    puts("Initializing engine...");

    engine->frame_limit = config->frame_limit;
    engine->window_width = 0;
    engine->window_height = 0;
    engine->initialized = false;
    engine->running = false;
    engine->headless = config->headless;
    engine->debug_fps_input = config->debug_fps_input;
    engine->platform = NULL;
    engine->renderer = NULL;
    engine->camera_controller = NULL;
    engine->physical_state = NULL;
    engine->view_state = NULL;
    engine->input = NULL;
    engine->timing = NULL;
    hth_camera_init_default(&engine->camera);
    engine->view_state = calloc(1, sizeof(*engine->view_state));
    if (engine->view_state != NULL) {
        engine->view_state->config = hth_view_dynamics_config_default();
        engine->view_state->base_vertical_fov_radians =
            engine->camera.vertical_fov_radians;
    }
    if (engine->view_state == NULL ||
        !hth_view_dynamics_config_is_valid(&engine->view_state->config)) {
        fputs("Failed to initialize view dynamics state.\n", stderr);
        destroy_view_state(engine);
        return false;
    }
    engine->camera_controller =
        hth_fps_camera_controller_create(&engine->camera);
    if (engine->camera_controller == NULL) {
        fputs("Failed to initialize FPS camera controller.\n", stderr);
        destroy_view_state(engine);
        return false;
    }
    engine->physical_state = calloc(1, sizeof(*engine->physical_state));
    if (engine->physical_state != NULL) {
        engine->physical_state->movement_config =
            hth_movement_config_default();
    }
    if (engine->physical_state == NULL ||
        !hth_movement_config_is_valid(
            &engine->physical_state->movement_config) ||
        !hth_player_body_init(&engine->physical_state->body,
                              hth_vec3(0.0F, 0.05F, 3.0F)) ||
        !hth_collision_world_init_bootstrap(
            &engine->physical_state->collision_world) ||
        !physical_state_spawn_is_clear(engine->physical_state) ||
        !hth_player_body_eye_position(&engine->physical_state->body,
                                      &engine->camera.position)) {
        fputs("Failed to initialize player movement physical state.\n",
              stderr);
        destroy_physical_state(engine);
        destroy_view_state(engine);
        hth_fps_camera_controller_destroy(engine->camera_controller);
        engine->camera_controller = NULL;
        return false;
    }

    window_title = config->window_title != NULL
        ? config->window_title
        : "Hertharian";
    window_width = config->window_width != 0 ? config->window_width : 1280;
    window_height = config->window_height != 0 ? config->window_height : 720;

    platform_config.window_title = window_title;
    platform_config.window_width = window_width;
    platform_config.window_height = window_height;
    platform_config.headless = config->headless;
    platform_config.graphics_enabled = !config->headless;
    platform_config.debug_fps_input = config->debug_fps_input;

    if (!hth_platform_init(&engine->platform, &platform_config)) {
        destroy_physical_state(engine);
        destroy_view_state(engine);
        hth_fps_camera_controller_destroy(engine->camera_controller);
        engine->camera_controller = NULL;
        return false;
    }

    if (!config->headless) {
        engine->renderer = hth_renderer_create(engine->platform,
                                               &engine->camera,
                                               &engine->physical_state->collision_world);
        if (engine->renderer == NULL) {
            hth_platform_shutdown(engine->platform);
            engine->platform = NULL;
            destroy_physical_state(engine);
            destroy_view_state(engine);
            hth_fps_camera_controller_destroy(engine->camera_controller);
            engine->camera_controller = NULL;
            return false;
        }
    } else {
        puts("Renderer disabled.");
    }

    engine->input = hth_input_create();
    if (engine->input == NULL) {
        fputs("Failed to initialize input state.\n", stderr);
        hth_renderer_destroy(engine->renderer);
        engine->renderer = NULL;
        hth_platform_shutdown(engine->platform);
        engine->platform = NULL;
        destroy_physical_state(engine);
        destroy_view_state(engine);
        hth_fps_camera_controller_destroy(engine->camera_controller);
        engine->camera_controller = NULL;
        return false;
    }

    frequency = hth_platform_time_frequency();
    counter = hth_platform_time_counter();
    engine->timing = hth_timing_create(config->target_fps, counter, frequency);
    if (engine->timing == NULL) {
        fputs("Failed to initialize timing state.\n", stderr);
        hth_input_destroy(engine->input);
        engine->input = NULL;
        hth_renderer_destroy(engine->renderer);
        engine->renderer = NULL;
        hth_platform_shutdown(engine->platform);
        engine->platform = NULL;
        destroy_physical_state(engine);
        destroy_view_state(engine);
        hth_fps_camera_controller_destroy(engine->camera_controller);
        engine->camera_controller = NULL;
        return false;
    }

    engine->window_width = window_width;
    engine->window_height = window_height;
    engine->initialized = true;
    engine->running = true;
    puts("Engine initialized.");
    return true;
}

void hth_engine_run(HTHEngine *engine)
{
    if (engine == NULL || !engine->initialized) {
        return;
    }

    while (engine->running) {
        hth_engine_frame(engine);

        if (engine->frame_limit > 0 &&
            hth_timing_frame_number(engine->timing) >= engine->frame_limit) {
            engine->running = false;
        }
    }
}

void hth_engine_frame(HTHEngine *engine)
{
    HTHPlatformEvent event;
    HTHPlayerMovementIntent movement_intent;
    HTHPlayerMovementResult movement_result;
    HTHViewDynamicsInput view_input;
    HTHViewDynamicsOutput view_output;
    HTHVec3 camera_right;
    HTHVec3 physical_eye;
    uint64_t counter;
    uint64_t sleep_ns;
    bool discard_mouse_delta = false;

    if (engine == NULL || !engine->initialized || !engine->running) {
        return;
    }

    counter = hth_platform_time_counter();
    hth_timing_begin_frame(engine->timing, counter);
    hth_input_begin_frame(engine->input);

    while (hth_platform_poll_event(engine->platform, &event)) {
        if (engine->debug_fps_input) {
            if (event.type == HTH_PLATFORM_EVENT_MOUSE_MOTION) {
                printf("HTH translated mouse:\n  dx=%.17g\n  dy=%.17g\n",
                       event.data.motion.delta_x,
                       event.data.motion.delta_y);
                if (hth_input_mouse_motion_discard_active(engine->input)) {
                    puts("FPS input: mouse motion discarded after capture "
                         "transition");
                }
            } else if (event.type == HTH_PLATFORM_EVENT_MOUSE_BUTTON_DOWN &&
                       event.data.mouse_button.button == HTH_MOUSE_LEFT) {
                puts("FPS input: mouse left pressed");
            } else if (event.type == HTH_PLATFORM_EVENT_MOUSE_BUTTON_UP &&
                       event.data.mouse_button.button == HTH_MOUSE_LEFT) {
                puts("FPS input: mouse left released");
            } else if (event.type == HTH_PLATFORM_EVENT_KEY_DOWN &&
                       event.data.keyboard.key == HTH_KEY_ESCAPE) {
                puts("FPS input: Escape pressed");
            } else if (event.type == HTH_PLATFORM_EVENT_FOCUS_GAINED) {
                puts("FPS input: focus gained");
            } else if (event.type == HTH_PLATFORM_EVENT_FOCUS_LOST) {
                puts("FPS input: focus lost");
            }
        }
        if (event.type == HTH_PLATFORM_EVENT_QUIT) {
            engine->running = false;
        } else if (event.type == HTH_PLATFORM_EVENT_WINDOW_RESIZED) {
            engine->window_width = event.data.window.width;
            engine->window_height = event.data.window.height;
            if (engine->renderer != NULL &&
                !hth_renderer_resize(engine->renderer)) {
                fputs("Renderer resize failed.\n", stderr);
                engine->running = false;
            }
        } else if (event.type == HTH_PLATFORM_EVENT_FRAMEBUFFER_RESIZED &&
                   engine->renderer != NULL &&
                   !hth_renderer_resize(engine->renderer)) {
            fputs("Renderer framebuffer resize failed.\n", stderr);
            engine->running = false;
        } else if (event.type == HTH_PLATFORM_EVENT_FOCUS_GAINED ||
                   event.type == HTH_PLATFORM_EVENT_FOCUS_LOST) {
            discard_mouse_delta = true;
        }

        hth_input_handle_event(engine->input, &event);
    }

    if (!engine->running) {
        return;
    }

    if (discard_mouse_delta) {
        clear_debugged_mouse_delta(engine, "focus transition");
    }
    update_mouse_capture(engine);
    hth_fps_camera_controller_update(
        engine->camera_controller, &engine->camera, engine->input,
        engine->debug_fps_input);
    if (!hth_player_movement_build_intent(
            engine->input, engine->camera.forward, engine->camera.up,
            hth_fps_camera_controller_capture_active(
                engine->camera_controller),
            &movement_intent) ||
        !hth_player_movement_step_with_result(
            &engine->physical_state->body,
            &engine->physical_state->collision_world,
            &engine->physical_state->movement_config, &movement_intent,
            hth_timing_delta_seconds(engine->timing), &movement_result) ||
        !hth_player_body_eye_position(&engine->physical_state->body,
                                      &physical_eye)) {
        fputs("Player movement update failed.\n", stderr);
        engine->running = false;
        return;
    }

    view_input.physical_eye_position = physical_eye;
    view_input.horizontal_speed = sqrtf(
        engine->physical_state->body.velocity.x *
            engine->physical_state->body.velocity.x +
        engine->physical_state->body.velocity.z *
            engine->physical_state->body.velocity.z);
    view_input.speed_reference =
        engine->physical_state->movement_config.max_ground_speed;
    view_input.grounded = engine->physical_state->body.grounded;
    view_input.landed = movement_result.landed;
    view_input.landing_speed = movement_result.landing_speed;
    if (!hth_view_dynamics_update(
            &engine->view_state->dynamics, &engine->view_state->config,
            &view_input, hth_timing_delta_seconds(engine->timing),
            &view_output) ||
        !hth_vec3_normalize(
            hth_vec3_cross(engine->camera.forward, engine->camera.up),
            &camera_right)) {
        fputs("View dynamics update failed.\n", stderr);
        engine->running = false;
        return;
    }
    engine->camera.position = hth_vec3_add(
        physical_eye,
        hth_vec3_add(
            hth_vec3(0.0F, view_output.vertical_offset, 0.0F),
            hth_vec3_scale(camera_right, view_output.lateral_offset)));
    engine->camera.vertical_fov_radians =
        engine->view_state->base_vertical_fov_radians +
        view_output.fov_offset_radians;

    if (engine->renderer != NULL &&
        !hth_renderer_set_camera(engine->renderer, &engine->camera)) {
        fputs("Renderer camera update failed.\n", stderr);
        engine->running = false;
        return;
    }

    if (engine->frame_limit > 0) {
        printf("Frame %" PRIu64 "\n",
               hth_timing_frame_number(engine->timing));
    }

    if (engine->renderer != NULL && !hth_renderer_frame(engine->renderer)) {
        engine->running = false;
        return;
    }

    counter = hth_platform_time_counter();
    hth_timing_measure_work(engine->timing, counter);
    sleep_ns = hth_timing_remaining_ns(engine->timing);
    if (sleep_ns > 0) {
        hth_platform_sleep_ns(sleep_ns);
    }
    hth_input_end_frame(engine->input);
    hth_timing_finish_frame(engine->timing, hth_platform_time_counter());
}

void hth_engine_shutdown(HTHEngine *engine)
{
    if (engine == NULL || !engine->initialized) {
        return;
    }

    puts("Shutting down...");
    engine->running = false;
    if (hth_fps_camera_controller_capture_active(engine->camera_controller)) {
        (void)hth_platform_set_relative_mouse_mode(engine->platform, false);
        hth_fps_camera_controller_set_capture(engine->camera_controller,
                                              false);
    }
    hth_timing_destroy(engine->timing);
    engine->timing = NULL;
    hth_input_destroy(engine->input);
    engine->input = NULL;
    hth_fps_camera_controller_destroy(engine->camera_controller);
    engine->camera_controller = NULL;
    destroy_physical_state(engine);
    destroy_view_state(engine);
    hth_renderer_destroy(engine->renderer);
    engine->renderer = NULL;
    hth_platform_shutdown(engine->platform);
    engine->platform = NULL;
    engine->initialized = false;
    puts("Engine shutdown complete.");
}

const HTHInput *hth_engine_input(const HTHEngine *engine)
{
    return engine != NULL ? engine->input : NULL;
}

const HTHTiming *hth_engine_timing(const HTHEngine *engine)
{
    return engine != NULL ? engine->timing : NULL;
}
