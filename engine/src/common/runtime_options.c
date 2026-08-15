#include "runtime_options.h"

#include "level_selection.h"

#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static const char default_level_id[] = "bootstrap";

static bool parse_frame_limit(const char *text, uint64_t *frame_limit)
{
    char *end = NULL;
    uintmax_t value;

    if (text == NULL || frame_limit == NULL || text[0] == '\0' ||
        text[0] == '-') {
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

static bool fail(HTHRuntimeOptionsError *error,
                 HTHRuntimeOptionsErrorCode code, const char *argument)
{
    error->code = code;
    error->argument = argument;
    return false;
}

const char *hth_runtime_options_default_level_id(void)
{
    return default_level_id;
}

bool hth_runtime_options_parse(int argc, char *const argv[],
                               HTHRuntimeOptions *out_options,
                               HTHRuntimeOptionsError *out_error)
{
    HTHRuntimeOptions parsed = {
        .engine = {
            .frame_limit = 0,
            .window_title = "Hertharian",
            .window_width = 1280,
            .window_height = 720,
            .target_fps = 60,
            .headless = false,
            .debug_fps_input = false,
        },
        .level_id = default_level_id,
    };
    bool level_was_explicit = false;
    int index = 1;

    if (out_error == NULL) {
        return false;
    }
    out_error->code = HTH_RUNTIME_OPTIONS_ERROR_NONE;
    out_error->argument = NULL;
    if (argc < 1 || argv == NULL || argv[0] == NULL || out_options == NULL) {
        return fail(out_error, HTH_RUNTIME_OPTIONS_ERROR_INVALID_ARGUMENTS,
                    NULL);
    }

    while (index < argc) {
        if (argv[index] == NULL) {
            return fail(out_error,
                        HTH_RUNTIME_OPTIONS_ERROR_INVALID_ARGUMENTS, NULL);
        }
        if (strcmp(argv[index], "--headless") == 0 &&
            !parsed.engine.headless) {
            parsed.engine.headless = true;
            index++;
        } else if (strcmp(argv[index], "--debug-fps-input") == 0 &&
                   !parsed.engine.debug_fps_input) {
            parsed.engine.debug_fps_input = true;
            index++;
        } else if (strcmp(argv[index], "--frames") == 0 &&
                   index + 1 < argc && parsed.engine.frame_limit == 0 &&
                   parse_frame_limit(argv[index + 1],
                                     &parsed.engine.frame_limit)) {
            index += 2;
        } else if (strcmp(argv[index], "--level") == 0) {
            if (level_was_explicit) {
                return fail(out_error,
                            HTH_RUNTIME_OPTIONS_ERROR_DUPLICATE_LEVEL,
                            argv[index]);
            }
            if (index + 1 >= argc) {
                return fail(out_error,
                            HTH_RUNTIME_OPTIONS_ERROR_MISSING_LEVEL_VALUE,
                            argv[index]);
            }
            if (!hth_level_id_is_valid(argv[index + 1])) {
                return fail(out_error,
                            HTH_RUNTIME_OPTIONS_ERROR_INVALID_LEVEL_ID,
                            argv[index + 1]);
            }
            parsed.level_id = argv[index + 1];
            level_was_explicit = true;
            index += 2;
        } else {
            return fail(out_error,
                        HTH_RUNTIME_OPTIONS_ERROR_INVALID_ARGUMENTS,
                        argv[index]);
        }
    }
    *out_options = parsed;
    return true;
}
