#include "mouse_source.h"

#include <stddef.h>
#include <string.h>

bool hth_mouse_name_is_explicit_relative_source(const char *name)
{
    static const char prefix[] = "xwayland-relative-pointer";
    size_t prefix_length = sizeof(prefix) - 1;

    if (name == NULL || strncmp(name, prefix, prefix_length) != 0) {
        return false;
    }
    return name[prefix_length] == '\0' || name[prefix_length] == ':';
}
