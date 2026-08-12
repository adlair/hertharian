#include "gf_engine.h"

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

static bool parse_arguments(int argc, char **argv, GFEngineConfig *config)
{
    if (argc == 3 && strcmp(argv[1], "--frames") == 0) {
        return parse_frame_limit(argv[2], &config->frame_limit);
    }

    return false;
}

int main(int argc, char **argv)
{
    GFEngineConfig config = {0};
    GFEngine engine = {0};

    if (!parse_arguments(argc, argv, &config)) {
        fprintf(stderr, "Usage: %s --frames N\n", argv[0]);
        return EXIT_FAILURE;
    }

    printf("GF Engine %s\n", gf_engine_version());

    if (!gf_engine_init(&engine, &config)) {
        fputs("Failed to initialize engine.\n", stderr);
        return EXIT_FAILURE;
    }

    gf_engine_run(&engine);
    gf_engine_shutdown(&engine);

    return EXIT_SUCCESS;
}
