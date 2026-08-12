#include "hth_engine.h"
#include "hth_version.h"
#include "platform.h"

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
    HTHPlatformConfig platform_config;

    if (engine == NULL || config == NULL) {
        return false;
    }

    puts("Initializing engine...");

    engine->frame_number = 0;
    engine->frame_limit = config->frame_limit;
    engine->initialized = false;
    engine->running = false;
    engine->platform = NULL;

    window_title = config->window_title != NULL
        ? config->window_title
        : "Hertharian";
    window_width = config->window_width != 0 ? config->window_width : 1280;
    window_height = config->window_height != 0 ? config->window_height : 720;

    platform_config.window_title = window_title;
    platform_config.window_width = window_width;
    platform_config.window_height = window_height;

    if (!hth_platform_init(&engine->platform, &platform_config)) {
        return false;
    }

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
            engine->frame_number >= engine->frame_limit) {
            engine->running = false;
        }
    }
}

void hth_engine_frame(HTHEngine *engine)
{
    if (engine == NULL || !engine->initialized || !engine->running) {
        return;
    }

    if (!hth_platform_pump_events(engine->platform)) {
        engine->running = false;
        return;
    }

    /* Temporary bootstrap throttle. Replaced by real frame timing in v0.1.2. */
    hth_platform_sleep_ms(16);

    if (engine->frame_limit > 0) {
        printf("Frame %" PRIu64 "\n", engine->frame_number);
    }
    engine->frame_number++;
}

void hth_engine_shutdown(HTHEngine *engine)
{
    if (engine == NULL || !engine->initialized) {
        return;
    }

    puts("Shutting down...");
    engine->running = false;
    hth_platform_shutdown(engine->platform);
    engine->platform = NULL;
    engine->initialized = false;
    puts("Engine shutdown complete.");
}
