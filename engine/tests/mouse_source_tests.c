#include "mouse_source.h"

#include <assert.h>
#include <stddef.h>

int main(void)
{
    assert(hth_mouse_name_is_explicit_relative_source(
        "xwayland-relative-pointer:16"));
    assert(hth_mouse_name_is_explicit_relative_source(
        "xwayland-relative-pointer:2048"));
    assert(hth_mouse_name_is_explicit_relative_source(
        "xwayland-relative-pointer"));
    assert(!hth_mouse_name_is_explicit_relative_source(
        "xwayland-pointer:16"));
    assert(!hth_mouse_name_is_explicit_relative_source(
        "xwayland-pointer-gestures:16"));
    assert(!hth_mouse_name_is_explicit_relative_source(
        "xwayland-relative-pointer-extra:16"));
    assert(!hth_mouse_name_is_explicit_relative_source("Generic Mouse"));
    assert(!hth_mouse_name_is_explicit_relative_source(NULL));
    return 0;
}
