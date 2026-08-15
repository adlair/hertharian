#ifndef HTH_LEVEL_SELECTION_H
#define HTH_LEVEL_SELECTION_H

#include <stdbool.h>

typedef struct {
    char *level_id;
    char *resource_id;
} HTHLevelSelection;

bool hth_level_id_is_valid(const char *level_id);
bool hth_level_selection_init(HTHLevelSelection *selection,
                              const char *level_id);
void hth_level_selection_destroy(HTHLevelSelection *selection);

#endif
