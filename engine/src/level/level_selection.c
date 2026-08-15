#include "level_selection.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static const char resource_prefix[] = "levels/";
static const char resource_suffix[] = ".hthlevel";

static bool level_id_character_is_valid(unsigned char character)
{
    return (character >= (unsigned char)'a' &&
            character <= (unsigned char)'z') ||
           (character >= (unsigned char)'0' &&
            character <= (unsigned char)'9') ||
           character == (unsigned char)'_' ||
           character == (unsigned char)'-';
}

bool hth_level_id_is_valid(const char *level_id)
{
    const unsigned char *cursor;

    if (level_id == NULL || level_id[0] == '\0') {
        return false;
    }
    cursor = (const unsigned char *)level_id;
    while (*cursor != (unsigned char)'\0') {
        if (!level_id_character_is_valid(*cursor)) {
            return false;
        }
        cursor++;
    }
    return true;
}

bool hth_level_selection_init(HTHLevelSelection *selection,
                              const char *level_id)
{
    size_t level_id_length;
    size_t prefix_length = sizeof(resource_prefix) - 1U;
    size_t resource_length;
    size_t suffix_length = sizeof(resource_suffix) - 1U;

    if (selection == NULL || selection->level_id != NULL ||
        selection->resource_id != NULL || !hth_level_id_is_valid(level_id)) {
        return false;
    }
    level_id_length = strlen(level_id);
    if (level_id_length == SIZE_MAX ||
        prefix_length > SIZE_MAX - level_id_length) {
        return false;
    }
    resource_length = prefix_length + level_id_length;
    if (suffix_length > SIZE_MAX - resource_length) {
        return false;
    }
    resource_length += suffix_length;
    if (resource_length == SIZE_MAX) {
        return false;
    }

    selection->level_id = malloc(level_id_length + 1U);
    selection->resource_id = malloc(resource_length + 1U);
    if (selection->level_id == NULL || selection->resource_id == NULL) {
        hth_level_selection_destroy(selection);
        return false;
    }
    memcpy(selection->level_id, level_id, level_id_length + 1U);
    memcpy(selection->resource_id, resource_prefix, prefix_length);
    memcpy(selection->resource_id + prefix_length, level_id, level_id_length);
    memcpy(selection->resource_id + prefix_length + level_id_length,
           resource_suffix, suffix_length + 1U);
    return true;
}

void hth_level_selection_destroy(HTHLevelSelection *selection)
{
    if (selection == NULL) {
        return;
    }
    free(selection->resource_id);
    free(selection->level_id);
    selection->resource_id = NULL;
    selection->level_id = NULL;
}
