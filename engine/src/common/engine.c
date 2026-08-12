#include "hth_engine.h"
#include "hth_input.h"
#include "hth_timing.h"
#include "hth_version.h"
#include "input_internal.h"
#include "platform.h"
#include "renderer.h"
#include "timing_internal.h"

#include <inttypes.h>
#include <stdio.h>

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
    engine->platform = NULL;
    engine->renderer = NULL;
    engine->input = NULL;
    engine->timing = NULL;

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

    if (!hth_platform_init(&engine->platform, &platform_config)) {
        return false;
    }

    if (!config->headless) {
        engine->renderer = hth_renderer_create(engine->platform);
        if (engine->renderer == NULL) {
            hth_platform_shutdown(engine->platform);
            engine->platform = NULL;
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
    uint64_t counter;
    uint64_t sleep_ns;

    if (engine == NULL || !engine->initialized || !engine->running) {
        return;
    }

    counter = hth_platform_time_counter();
    hth_timing_begin_frame(engine->timing, counter);
    hth_input_begin_frame(engine->input);

    while (hth_platform_poll_event(engine->platform, &event)) {
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
        }

        hth_input_handle_event(engine->input, &event);
    }

    if (!engine->running) {
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
    hth_timing_finish_frame(engine->timing, hth_platform_time_counter());
}

void hth_engine_shutdown(HTHEngine *engine)
{
    if (engine == NULL || !engine->initialized) {
        return;
    }

    puts("Shutting down...");
    engine->running = false;
    hth_timing_destroy(engine->timing);
    engine->timing = NULL;
    hth_input_destroy(engine->input);
    engine->input = NULL;
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
