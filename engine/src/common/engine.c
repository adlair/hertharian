#include "hth_engine.h"
#include "hth_version.h"

#include <inttypes.h>
#include <stdio.h>

const char *hth_engine_version(void)
{
    return HTH_ENGINE_VERSION;
}

bool hth_engine_init(HTHEngine *engine, const HTHEngineConfig *config)
{
    if (engine == NULL || config == NULL) {
        return false;
    }

    puts("Initializing engine...");

    engine->frame_number = 0;
    engine->frame_limit = config->frame_limit;
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

    printf("Frame %" PRIu64 "\n", engine->frame_number);
    engine->frame_number++;
}

void hth_engine_shutdown(HTHEngine *engine)
{
    if (engine == NULL || !engine->initialized) {
        return;
    }

    puts("Shutting down...");
    engine->running = false;
    engine->initialized = false;
    puts("Engine shutdown complete.");
}
