#include "hth_engine.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool parse_frame_limit(const char *text, uint64_t *frame_limit)
{
    char *end = NULL;
    uintmax_t value;

    if (text == NULL || frame_limit == NULL || text[0] == '\0' || text[0] == '-') {
        return false;
    }

    errno = 0;
    value = strtoumax(text, &end, 10);
    if (errno == ERANGE || *end != '\0' || value == 0 || value > UINT64_MAX) {
        return false;
    }

    *frame_limit = (uint64_t)value;
    return true;
}

static bool parse_arguments(int argc, char **argv, HTHEngineConfig *config)
{
    int index = 1;

    while (index < argc) {
        if (strcmp(argv[index], "--headless") == 0 && !config->headless) {
            config->headless = true;
            index++;
        } else if (strcmp(argv[index], "--debug-fps-input") == 0 &&
                   !config->debug_fps_input) {
            config->debug_fps_input = true;
            index++;
        } else if (strcmp(argv[index], "--frames") == 0 &&
                   index + 1 < argc && config->frame_limit == 0 &&
                   parse_frame_limit(argv[index + 1], &config->frame_limit)) {
            index += 2;
        } else {
            return false;
        }
    }
    return true;
}

int main(int argc, char **argv)
{
    HTHEngineConfig config = {
        .frame_limit = 0,
        .window_title = "Hertharian",
        .window_width = 1280,
        .window_height = 720,
        .target_fps = 60,
        .headless = false,
        .debug_fps_input = false,
    };
    HTHEngine engine = {0};

    if (!parse_arguments(argc, argv, &config)) {
        fprintf(stderr, "Usage: %s [--headless] [--frames N] "
                "[--debug-fps-input]\n", argv[0]);
        return EXIT_FAILURE;
    }

    printf("Hertharian Engine %s\n", hth_engine_version());

    if (!hth_engine_init(&engine, &config)) {
        fputs("Failed to initialize engine.\n", stderr);
        return EXIT_FAILURE;
    }

    hth_engine_run(&engine);
    hth_engine_shutdown(&engine);

    return EXIT_SUCCESS;
}
