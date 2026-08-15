#include "hth_engine.h"
#include "engine_internal.h"
#include "runtime_options.h"

#include <stdio.h>
#include <stdlib.h>

static void print_usage(const char *program)
{
    fprintf(stderr, "Usage: %s [--headless] [--frames N] "
            "[--debug-fps-input] [--level ID]\n", program);
}

int main(int argc, char **argv)
{
    HTHRuntimeOptions options;
    HTHRuntimeOptionsError error;
    HTHEngine engine = {0};

    if (!hth_runtime_options_parse(argc, argv, &options, &error)) {
        if (error.code == HTH_RUNTIME_OPTIONS_ERROR_INVALID_LEVEL_ID) {
            fprintf(stderr, "Invalid level ID '%s'.\n", error.argument);
        } else if (error.code ==
                   HTH_RUNTIME_OPTIONS_ERROR_DUPLICATE_LEVEL) {
            fputs("Duplicate --level option.\n", stderr);
        } else if (error.code ==
                   HTH_RUNTIME_OPTIONS_ERROR_MISSING_LEVEL_VALUE) {
            fputs("Missing value for --level.\n", stderr);
        }
        print_usage(argv != NULL && argv[0] != NULL ? argv[0]
                                                   : "hertharian-engine");
        return EXIT_FAILURE;
    }

    printf("Hertharian Engine %s\n", hth_engine_version());

    if (!hth_engine_init_with_level_id(
            &engine, &options.engine, options.level_id)) {
        fputs("Failed to initialize engine.\n", stderr);
        return EXIT_FAILURE;
    }

    hth_engine_run(&engine);
    hth_engine_shutdown(&engine);

    return EXIT_SUCCESS;
}
