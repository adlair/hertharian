#include "gf_engine.h"
#include "gf_version.h"

#include <inttypes.h>
#include <stdio.h>

const char *gf_engine_version(void)
{
    return GF_ENGINE_VERSION;
}

bool gf_engine_init(GFEngine *engine, const GFEngineConfig *config)
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

void gf_engine_run(GFEngine *engine)
{
    if (engine == NULL || !engine->initialized) {
        return;
    }

    while (engine->running) {
        gf_engine_frame(engine);

        if (engine->frame_limit > 0 &&
            engine->frame_number >= engine->frame_limit) {
            engine->running = false;
        }
    }
}

void gf_engine_frame(GFEngine *engine)
{
    if (engine == NULL || !engine->initialized || !engine->running) {
        return;
    }

    printf("Frame %" PRIu64 "\n", engine->frame_number);
    engine->frame_number++;
}

void gf_engine_shutdown(GFEngine *engine)
{
    if (engine == NULL || !engine->initialized) {
        return;
    }

    puts("Shutting down...");
    engine->running = false;
    engine->initialized = false;
    puts("Engine shutdown complete.");
}
