#include "platform.h"
#include "keyboard_reconciliation.h"
#include "mouse_source.h"
#if defined(HTH_HAVE_X11_OBSERVER)
#include "platform_x11.h"
#endif

#include <SDL3/SDL.h>

#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct HTHPlatform {
    SDL_Window *window;
    SDL_MouseID relative_mouse_source_id;
    HTHRelativeMouseFilter relative_mouse_filter;
    HTHKeyboardReconciliation keyboard_reconciliation;
    SDL_Scancode reported_scancodes[HTH_KEY_COUNT];
#if defined(HTH_HAVE_X11_OBSERVER)
    HTHPlatformX11Observer x11_observer;
    uint16_t reported_x11_keycodes[HTH_KEY_COUNT];
    bool x11_observer_active;
#endif
    bool headless;
    bool debug_fps_input;
    bool relative_mouse_source_known;
};

struct HTHPlatformGraphicsContext {
    SDL_GLContext handle;
};

static bool set_graphics_attribute(SDL_GLAttr attribute, int value)
{
    if (SDL_GL_SetAttribute(attribute, value)) {
        return true;
    }

    fprintf(stderr, "SDL OpenGL attribute configuration failed: %s\n",
            SDL_GetError());
    return false;
}

static bool prepare_graphics(void)
{
    return set_graphics_attribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) &&
           set_graphics_attribute(SDL_GL_CONTEXT_MINOR_VERSION, 3) &&
           set_graphics_attribute(SDL_GL_CONTEXT_PROFILE_MASK,
                                  SDL_GL_CONTEXT_PROFILE_CORE) &&
           set_graphics_attribute(SDL_GL_DOUBLEBUFFER, 1) &&
           set_graphics_attribute(SDL_GL_DEPTH_SIZE, 24) &&
           set_graphics_attribute(SDL_GL_STENCIL_SIZE, 8);
}

static HTHKey translate_key(SDL_Scancode scancode)
{
    if (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z) {
        return (HTHKey)(HTH_KEY_A + (scancode - SDL_SCANCODE_A));
    }
    if (scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_9) {
        return (HTHKey)(HTH_KEY_1 + (scancode - SDL_SCANCODE_1));
    }
    if (scancode >= SDL_SCANCODE_F1 && scancode <= SDL_SCANCODE_F12) {
        return (HTHKey)(HTH_KEY_F1 + (scancode - SDL_SCANCODE_F1));
    }

    switch (scancode) {
    case SDL_SCANCODE_0: return HTH_KEY_0;
    case SDL_SCANCODE_ESCAPE: return HTH_KEY_ESCAPE;
    case SDL_SCANCODE_RETURN: return HTH_KEY_ENTER;
    case SDL_SCANCODE_SPACE: return HTH_KEY_SPACE;
    case SDL_SCANCODE_TAB: return HTH_KEY_TAB;
    case SDL_SCANCODE_BACKSPACE: return HTH_KEY_BACKSPACE;
    case SDL_SCANCODE_UP: return HTH_KEY_UP;
    case SDL_SCANCODE_DOWN: return HTH_KEY_DOWN;
    case SDL_SCANCODE_LEFT: return HTH_KEY_LEFT;
    case SDL_SCANCODE_RIGHT: return HTH_KEY_RIGHT;
    case SDL_SCANCODE_LSHIFT: return HTH_KEY_LSHIFT;
    case SDL_SCANCODE_RSHIFT: return HTH_KEY_RSHIFT;
    case SDL_SCANCODE_LCTRL: return HTH_KEY_LCTRL;
    case SDL_SCANCODE_RCTRL: return HTH_KEY_RCTRL;
    case SDL_SCANCODE_LALT: return HTH_KEY_LALT;
    case SDL_SCANCODE_RALT: return HTH_KEY_RALT;
    default: return HTH_KEY_UNKNOWN;
    }
}

static bool valid_translated_key(HTHKey key)
{
    return key > HTH_KEY_UNKNOWN && key < HTH_KEY_COUNT;
}

static HTHMouseButton translate_mouse_button(uint8_t button)
{
    switch (button) {
    case SDL_BUTTON_LEFT: return HTH_MOUSE_LEFT;
    case SDL_BUTTON_MIDDLE: return HTH_MOUSE_MIDDLE;
    case SDL_BUTTON_RIGHT: return HTH_MOUSE_RIGHT;
    case SDL_BUTTON_X1: return HTH_MOUSE_X1;
    case SDL_BUTTON_X2: return HTH_MOUSE_X2;
    default: return HTH_MOUSE_UNKNOWN;
    }
}

static void print_keyboard_event(const SDL_KeyboardEvent *keyboard,
                                 HTHKey translated_key, bool down)
{
    printf("SDL raw key:\n"
           "  action=%s\n"
           "  scancode=%d\n"
           "  repeat=%s\n"
           "  platform_raw=%" PRIu16 "\n"
           "  windowID=%" PRIu32 "\n"
           "HTH translated key:\n"
           "  key=%d\n"
           "  action=%s\n",
           down ? "down" : "up",
           (int)keyboard->scancode,
           keyboard->repeat ? "true" : "false",
           (uint16_t)keyboard->raw,
           (uint32_t)keyboard->windowID,
           (int)translated_key,
           down ? "down" : "up");
}

static void print_focus_event(const char *focus, const char *action)
{
    printf("SDL focus:\n  %s %s\n", focus, action);
}

static bool observe_sdl_keyboard(bool physical_down[HTH_KEY_COUNT])
{
    const bool *keyboard_state;
    HTHKey key;
    int scancode_count = 0;
    int index;

    keyboard_state = SDL_GetKeyboardState(&scancode_count);
    if (keyboard_state == NULL || scancode_count <= 0) {
        return false;
    }
    for (index = 0; index < scancode_count; ++index) {
        if (keyboard_state[index]) {
            key = translate_key((SDL_Scancode)index);
            if (valid_translated_key(key)) {
                physical_down[key] = true;
            }
        }
    }
    return true;
}

static void make_reconciled_key_up(HTHPlatform *platform,
                                   HTHPlatformEvent *event, HTHKey key)
{
    platform->reported_scancodes[key] = SDL_SCANCODE_UNKNOWN;
#if defined(HTH_HAVE_X11_OBSERVER)
    platform->reported_x11_keycodes[key] = 0U;
#endif
    *event = (HTHPlatformEvent){0};
    event->type = HTH_PLATFORM_EVENT_KEY_UP;
    event->timestamp_ns = SDL_GetTicksNS();
    event->data.keyboard.key = key;
}

static bool reconcile_sdl_keyboard(HTHPlatform *platform,
                                   HTHPlatformEvent *event,
                                   const bool sdl_down[HTH_KEY_COUNT])
{
    HTHKey key;
    SDL_Scancode scancode;

    if (!hth_keyboard_reconciliation_next_release(
            &platform->keyboard_reconciliation, sdl_down, &key)) {
        return false;
    }

    scancode = platform->reported_scancodes[key];
    make_reconciled_key_up(platform, event, key);
    if (platform->debug_fps_input) {
        printf("SDL keyboard reconciliation:\n"
               "  scancode=%d\n"
               "  HTH key=%d\n"
               "  SDL state=up\n"
               "  normalized action=release\n",
               (int)scancode, (int)key);
    }
    return true;
}

#if defined(HTH_HAVE_X11_OBSERVER)
static bool reconcile_x11_keyboard(HTHPlatform *platform,
                                   HTHPlatformEvent *event)
{
    bool armed_before[HTH_KEY_COUNT];
    bool x11_down[HTH_X11_KEYCODE_COUNT];
    bool observed_down[HTH_KEY_COUNT] = {false};
    bool sdl_down[HTH_KEY_COUNT] = {false};
    HTHKey key;
    SDL_Scancode scancode;
    uint16_t x11_keycode;
    size_t index;

    if (!hth_platform_x11_query_keyboard(&platform->x11_observer,
                                         x11_down)) {
        return false;
    }

    for (index = (size_t)HTH_KEY_UNKNOWN + 1U;
         index < (size_t)HTH_KEY_COUNT; ++index) {
        x11_keycode = platform->reported_x11_keycodes[index];
        if (x11_keycode != 0U && x11_keycode < HTH_X11_KEYCODE_COUNT) {
            observed_down[index] = x11_down[x11_keycode];
        } else {
            /* An unassociated key cannot be authoritatively reconciled. */
            observed_down[index] = true;
        }
    }

    memcpy(armed_before,
           platform->keyboard_reconciliation.release_armed,
           sizeof(armed_before));

    if (!hth_keyboard_reconciliation_next_release(
            &platform->keyboard_reconciliation, observed_down, &key)) {
        if (platform->debug_fps_input) {
            (void)observe_sdl_keyboard(sdl_down);
            for (index = (size_t)HTH_KEY_UNKNOWN + 1U;
                 index < (size_t)HTH_KEY_COUNT; ++index) {
                if (platform->keyboard_reconciliation.reported_down[index] &&
                    !observed_down[index]) {
                    printf("X11 keyboard reconciliation observation:\n"
                           "  HTH key=%zu\n"
                           "  reported=down\n"
                           "  X11 state=up\n"
                           "  SDL cached state=%s\n"
                           "  release armed before=%s\n"
                           "  release armed after=%s\n"
                           "  normalized action=none\n",
                           index,
                           sdl_down[index] ? "down" : "up",
                           armed_before[index] ? "true" : "false",
                           platform->keyboard_reconciliation
                                   .release_armed[index]
                               ? "true" : "false");
                }
            }
        }
        return false;
    }

    scancode = platform->reported_scancodes[key];
    x11_keycode = platform->reported_x11_keycodes[key];
    if (platform->debug_fps_input) {
        (void)observe_sdl_keyboard(sdl_down);
        printf("X11 keyboard reconciliation:\n"
               "  HTH key=%d\n"
               "  SDL scancode=%d\n"
               "  X11 keycode=%" PRIu16 "\n"
               "  reported=down\n"
               "  X11 state=up\n"
               "  SDL cached state=%s\n"
               "  release armed before=%s\n"
               "  release armed after=false\n"
               "  normalized action=release\n"
               "HTH translated/reconciled key:\n"
               "  key=%d\n"
               "  action=up\n",
               (int)key, (int)scancode, x11_keycode,
               sdl_down[key] ? "down" : "up",
               armed_before[key] ? "true" : "false", (int)key);
    }
    make_reconciled_key_up(platform, event, key);
    return true;
}
#endif

static void print_mouse_device(const char *label, SDL_MouseID id)
{
    const char *name = SDL_GetMouseNameForID(id);

    printf("%s:\n  id=%" PRIu32 "\n  name=\"%s\"\n",
           label, (uint32_t)id, name != NULL ? name : "unavailable");
}

static void refresh_relative_mouse_source(HTHPlatform *platform,
                                          bool print_inventory)
{
    SDL_MouseID *mice;
    SDL_MouseID previous_id = platform->relative_mouse_source_id;
    const char *name;
    bool previous_known = platform->relative_mouse_source_known;
    int count = 0;
    int index;

    platform->relative_mouse_source_id = 0;
    platform->relative_mouse_source_known = false;
    mice = SDL_GetMice(&count);
    if (mice == NULL) {
        if (platform->debug_fps_input) {
            fprintf(stderr, "SDL mouse device enumeration failed: %s\n",
                    SDL_GetError());
        }
        return;
    }
    for (index = 0; index < count; ++index) {
        name = SDL_GetMouseNameForID(mice[index]);
        if (platform->debug_fps_input && print_inventory) {
            print_mouse_device("SDL mouse device", mice[index]);
        }
        if (!platform->relative_mouse_source_known &&
            hth_mouse_name_is_explicit_relative_source(name)) {
            platform->relative_mouse_source_id = mice[index];
            platform->relative_mouse_source_known = true;
        }
    }
    SDL_free(mice);

    if (previous_known != platform->relative_mouse_source_known ||
        (previous_known && previous_id != platform->relative_mouse_source_id)) {
        hth_relative_mouse_filter_reset(&platform->relative_mouse_filter);
    }

    if (platform->debug_fps_input &&
        (print_inventory ||
         previous_known != platform->relative_mouse_source_known ||
         (previous_known && previous_id !=
          platform->relative_mouse_source_id))) {
        if (platform->relative_mouse_source_known) {
            print_mouse_device("SDL relative mouse source",
                               platform->relative_mouse_source_id);
        } else {
            puts("SDL relative mouse source: generic SDL fallback");
        }
    }
}

bool hth_platform_init(HTHPlatform **platform, const HTHPlatformConfig *config)
{
    const char *video_driver;
    HTHPlatform *state;

    if (platform == NULL || config == NULL ||
        (!config->headless && (config->window_title == NULL ||
         config->window_width == 0 || config->window_width > INT_MAX ||
         config->window_height == 0 || config->window_height > INT_MAX))) {
        fputs("Invalid platform configuration.\n", stderr);
        return false;
    }

    *platform = NULL;
    puts("Initializing platform...");

    state = calloc(1, sizeof(*state));
    if (state == NULL) {
        fputs("Failed to allocate platform state.\n", stderr);
        return false;
    }

    if (!SDL_Init(config->headless ? SDL_INIT_EVENTS : SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        free(state);
        return false;
    }

    state->headless = config->headless;
    state->debug_fps_input = config->debug_fps_input;
    if (config->headless) {
        puts("Headless mode.");
        *platform = state;
        puts("Platform initialized.");
        return true;
    }

    if (config->graphics_enabled && !prepare_graphics()) {
        SDL_Quit();
        free(state);
        return false;
    }

    state->window = SDL_CreateWindow(
        config->window_title,
        (int)config->window_width,
        (int)config->window_height,
        config->graphics_enabled
            ? SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
            : SDL_WINDOW_RESIZABLE
    );
    if (state->window == NULL) {
        fprintf(stderr, "SDL window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        free(state);
        return false;
    }

    if (!SDL_ShowWindow(state->window)) {
        fprintf(stderr, "SDL window show failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(state->window);
        SDL_Quit();
        free(state);
        return false;
    }

    video_driver = SDL_GetCurrentVideoDriver();
#if defined(HTH_HAVE_X11_OBSERVER)
    if (video_driver != NULL && SDL_strcmp(video_driver, "x11") == 0) {
        SDL_PropertiesID properties = SDL_GetWindowProperties(state->window);
        void *display = SDL_GetPointerProperty(
            properties, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);

        state->x11_observer_active = hth_platform_x11_observer_init(
            &state->x11_observer, display);
        if (state->debug_fps_input && !state->x11_observer_active) {
            fprintf(stderr, "X11 keyboard observer unavailable: %s\n",
                    SDL_GetError());
        }
    }
#endif
    refresh_relative_mouse_source(state, state->debug_fps_input);

    printf("SDL video driver: %s\n",
           video_driver != NULL ? video_driver : "unknown");

    *platform = state;
    puts("Platform initialized.");
    return true;
}

void hth_platform_shutdown(HTHPlatform *platform)
{
    if (platform == NULL) {
        return;
    }

    SDL_DestroyWindow(platform->window);
    SDL_Quit();
    free(platform);
    puts("Platform shutdown complete.");
}

bool hth_platform_poll_event(HTHPlatform *platform, HTHPlatformEvent *event)
{
    SDL_Event native_event;
    HTHRelativeMouseMotionDecision mouse_motion_decision;
    double corrected_delta_x;
    double corrected_delta_y;
    double wheel_direction;

    if (platform == NULL || event == NULL) {
        return false;
    }

    while (SDL_PollEvent(&native_event)) {
        *event = (HTHPlatformEvent){0};
        event->timestamp_ns = native_event.common.timestamp;

        switch (native_event.type) {
        case SDL_EVENT_QUIT:
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            event->type = HTH_PLATFORM_EVENT_QUIT;
            return true;
        case SDL_EVENT_KEY_DOWN:
            event->type = HTH_PLATFORM_EVENT_KEY_DOWN;
            event->data.keyboard.key = translate_key(native_event.key.scancode);
            event->data.keyboard.repeat = native_event.key.repeat;
            hth_keyboard_reconciliation_report_down(
                &platform->keyboard_reconciliation,
                event->data.keyboard.key);
            if (valid_translated_key(event->data.keyboard.key)) {
                platform->reported_scancodes[event->data.keyboard.key] =
                    native_event.key.scancode;
#if defined(HTH_HAVE_X11_OBSERVER)
                if (platform->x11_observer_active) {
                    platform->reported_x11_keycodes[
                        event->data.keyboard.key] = native_event.key.raw;
                }
#endif
            }
            if (platform->debug_fps_input) {
                print_keyboard_event(&native_event.key,
                                     event->data.keyboard.key, true);
            }
            return true;
        case SDL_EVENT_KEY_UP:
            event->type = HTH_PLATFORM_EVENT_KEY_UP;
            event->data.keyboard.key = translate_key(native_event.key.scancode);
            event->data.keyboard.repeat = false;
            hth_keyboard_reconciliation_report_up(
                &platform->keyboard_reconciliation,
                event->data.keyboard.key);
            if (valid_translated_key(event->data.keyboard.key)) {
                platform->reported_scancodes[event->data.keyboard.key] =
                    SDL_SCANCODE_UNKNOWN;
#if defined(HTH_HAVE_X11_OBSERVER)
                platform->reported_x11_keycodes[
                    event->data.keyboard.key] = 0U;
#endif
            }
            if (platform->debug_fps_input) {
                print_keyboard_event(&native_event.key,
                                     event->data.keyboard.key, false);
            }
            return true;
        case SDL_EVENT_MOUSE_MOTION:
            if (platform->debug_fps_input) {
                SDL_WindowFlags flags = SDL_GetWindowFlags(platform->window);

                printf("SDL raw mouse:\n"
                       "  timestamp_ns=%" PRIu64 "\n"
                       "  x=%.17g\n"
                       "  y=%.17g\n"
                       "  xrel=%.17g\n"
                       "  yrel=%.17g\n"
                       "  which=%" PRIu32 "\n"
                       "  relative_mode=%s\n"
                       "  mouse_focus=%s\n"
                       "  input_focus=%s\n",
                       (uint64_t)native_event.motion.timestamp,
                       (double)native_event.motion.x,
                       (double)native_event.motion.y,
                       (double)native_event.motion.xrel,
                       (double)native_event.motion.yrel,
                       (uint32_t)native_event.motion.which,
                       SDL_GetWindowRelativeMouseMode(platform->window)
                           ? "true" : "false",
                       (flags & SDL_WINDOW_MOUSE_FOCUS) != 0
                           ? "true" : "false",
                       (flags & SDL_WINDOW_INPUT_FOCUS) != 0
                           ? "true" : "false");
            }
            mouse_motion_decision = hth_relative_mouse_filter_motion(
                &platform->relative_mouse_filter,
                SDL_GetWindowRelativeMouseMode(platform->window),
                platform->relative_mouse_source_known,
                (uint32_t)platform->relative_mouse_source_id,
                (uint32_t)native_event.motion.which,
                (double)native_event.motion.xrel,
                (double)native_event.motion.yrel,
                &corrected_delta_x,
                &corrected_delta_y);
            if (mouse_motion_decision != HTH_RELATIVE_MOUSE_MOTION_ACCEPT) {
                if (platform->debug_fps_input) {
                    if (mouse_motion_decision ==
                        HTH_RELATIVE_MOUSE_MOTION_DISCARD_FOREIGN_SOURCE) {
                        puts("FPS input: mouse motion discarded: "
                             "non-relative source during relative mode; "
                             "re-entry compensation armed");
                    } else {
                        puts("FPS input: mouse motion discarded: "
                             "relative re-entry compensation; "
                             "relative input primed");
                        printf("FPS input: reconstructed relative sample: "
                               "dx=%.17g dy=%.17g\n",
                               corrected_delta_x, corrected_delta_y);
                    }
                }
                break;
            }
            if (platform->debug_fps_input) {
                printf("FPS input: mouse motion accepted by platform: "
                       "dx=%.17g dy=%.17g\n",
                       corrected_delta_x, corrected_delta_y);
            }
            event->type = HTH_PLATFORM_EVENT_MOUSE_MOTION;
            event->data.motion.x = native_event.motion.x;
            event->data.motion.y = native_event.motion.y;
            event->data.motion.delta_x = corrected_delta_x;
            event->data.motion.delta_y = corrected_delta_y;
            return true;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (platform->debug_fps_input) {
                printf("SDL raw mouse button:\n"
                       "  action=%s\n"
                       "  button=%" PRIu8 "\n"
                       "  which=%" PRIu32 "\n",
                       native_event.type == SDL_EVENT_MOUSE_BUTTON_DOWN
                           ? "down" : "up",
                       (uint8_t)native_event.button.button,
                       (uint32_t)native_event.button.which);
            }
            event->type = native_event.type == SDL_EVENT_MOUSE_BUTTON_DOWN
                ? HTH_PLATFORM_EVENT_MOUSE_BUTTON_DOWN
                : HTH_PLATFORM_EVENT_MOUSE_BUTTON_UP;
            event->data.mouse_button.button =
                translate_mouse_button(native_event.button.button);
            event->data.mouse_button.x = native_event.button.x;
            event->data.mouse_button.y = native_event.button.y;
            return true;
        case SDL_EVENT_MOUSE_ADDED:
        case SDL_EVENT_MOUSE_REMOVED:
            refresh_relative_mouse_source(platform, false);
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            wheel_direction = native_event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED
                ? -1.0 : 1.0;
            event->type = HTH_PLATFORM_EVENT_MOUSE_WHEEL;
            event->data.wheel.x = (double)native_event.wheel.x * wheel_direction;
            event->data.wheel.y = (double)native_event.wheel.y * wheel_direction;
            return true;
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
            hth_relative_mouse_filter_cancel_transition(
                &platform->relative_mouse_filter);
            if (platform->debug_fps_input) {
                print_focus_event("input_focus", "gained");
            }
            event->type = HTH_PLATFORM_EVENT_FOCUS_GAINED;
            return true;
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            hth_relative_mouse_filter_cancel_transition(
                &platform->relative_mouse_filter);
            if (platform->debug_fps_input) {
                print_focus_event("input_focus", "lost");
            }
            event->type = HTH_PLATFORM_EVENT_FOCUS_LOST;
            hth_keyboard_reconciliation_reset(
                &platform->keyboard_reconciliation);
            memset(platform->reported_scancodes, 0,
                   sizeof(platform->reported_scancodes));
#if defined(HTH_HAVE_X11_OBSERVER)
            memset(platform->reported_x11_keycodes, 0,
                   sizeof(platform->reported_x11_keycodes));
#endif
            return true;
        case SDL_EVENT_WINDOW_MOUSE_ENTER:
            hth_relative_mouse_filter_cancel_transition(
                &platform->relative_mouse_filter);
            if (platform->debug_fps_input) {
                print_focus_event("mouse_focus", "gained");
            }
            break;
        case SDL_EVENT_WINDOW_MOUSE_LEAVE:
            hth_relative_mouse_filter_cancel_transition(
                &platform->relative_mouse_filter);
            if (platform->debug_fps_input) {
                print_focus_event("mouse_focus", "lost");
            }
            break;
        case SDL_EVENT_WINDOW_RESIZED:
            event->type = HTH_PLATFORM_EVENT_WINDOW_RESIZED;
            event->data.window.width = native_event.window.data1 > 0
                ? (uint32_t)native_event.window.data1 : 0;
            event->data.window.height = native_event.window.data2 > 0
                ? (uint32_t)native_event.window.data2 : 0;
            return true;
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            event->type = HTH_PLATFORM_EVENT_FRAMEBUFFER_RESIZED;
            event->data.window.width = native_event.window.data1 > 0
                ? (uint32_t)native_event.window.data1 : 0;
            event->data.window.height = native_event.window.data2 > 0
                ? (uint32_t)native_event.window.data2 : 0;
            return true;
        default:
            break;
        }
    }

#if defined(HTH_HAVE_X11_OBSERVER)
    if (platform->x11_observer_active) {
        return reconcile_x11_keyboard(platform, event);
    }
#endif
    {
        bool sdl_down[HTH_KEY_COUNT] = {false};

        return observe_sdl_keyboard(sdl_down) &&
               reconcile_sdl_keyboard(platform, event, sdl_down);
    }
}

uint64_t hth_platform_time_counter(void)
{
    return SDL_GetPerformanceCounter();
}

uint64_t hth_platform_time_frequency(void)
{
    return SDL_GetPerformanceFrequency();
}

void hth_platform_sleep_ns(uint64_t nanoseconds)
{
    SDL_DelayNS(nanoseconds);
}

bool hth_platform_set_relative_mouse_mode(HTHPlatform *platform,
                                          bool enabled)
{
    if (platform == NULL || platform->window == NULL || platform->headless) {
        return false;
    }
    if (!SDL_SetWindowRelativeMouseMode(platform->window, enabled)) {
        fprintf(stderr, "SDL relative mouse mode %s failed: %s\n",
                enabled ? "enable" : "disable", SDL_GetError());
        return false;
    }
    hth_relative_mouse_filter_reset(&platform->relative_mouse_filter);
    return true;
}

HTHPlatformGraphicsContext *hth_platform_graphics_create_context(
    HTHPlatform *platform)
{
    HTHPlatformGraphicsContext *context;

    if (platform == NULL || platform->window == NULL || platform->headless) {
        return NULL;
    }

    context = calloc(1, sizeof(*context));
    if (context == NULL) {
        fputs("Failed to allocate graphics context state.\n", stderr);
        return NULL;
    }

    context->handle = SDL_GL_CreateContext(platform->window);
    if (context->handle == NULL) {
        fprintf(stderr, "SDL OpenGL context creation failed: %s\n",
                SDL_GetError());
        free(context);
        return NULL;
    }

    return context;
}

void hth_platform_graphics_destroy_context(
    HTHPlatformGraphicsContext *context)
{
    if (context == NULL) {
        return;
    }

    if (!SDL_GL_DestroyContext(context->handle)) {
        fprintf(stderr, "SDL OpenGL context destruction failed: %s\n",
                SDL_GetError());
    }
    free(context);
}

bool hth_platform_graphics_make_current(
    HTHPlatform *platform, HTHPlatformGraphicsContext *context)
{
    if (platform == NULL || platform->window == NULL || context == NULL) {
        return false;
    }
    if (!SDL_GL_MakeCurrent(platform->window, context->handle)) {
        fprintf(stderr, "SDL OpenGL make-current failed: %s\n", SDL_GetError());
        return false;
    }
    return true;
}

bool hth_platform_graphics_swap(HTHPlatform *platform)
{
    if (platform == NULL || platform->window == NULL) {
        return false;
    }
    if (!SDL_GL_SwapWindow(platform->window)) {
        fprintf(stderr, "SDL OpenGL presentation failed: %s\n", SDL_GetError());
        return false;
    }
    return true;
}

bool hth_platform_graphics_set_swap_interval(int interval)
{
    if (!SDL_GL_SetSwapInterval(interval)) {
        fprintf(stderr, "SDL OpenGL swap interval configuration failed: %s\n",
                SDL_GetError());
        return false;
    }
    return true;
}

bool hth_platform_graphics_context_info(int *major, int *minor,
                                        bool *core_profile, int *double_buffer,
                                        int *depth_bits, int *stencil_bits)
{
    int profile;

    if (major == NULL || minor == NULL || core_profile == NULL ||
        double_buffer == NULL || depth_bits == NULL || stencil_bits == NULL ||
        !SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major) ||
        !SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor) ||
        !SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &profile) ||
        !SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, double_buffer) ||
        !SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, depth_bits) ||
        !SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, stencil_bits)) {
        fprintf(stderr, "SDL OpenGL context query failed: %s\n", SDL_GetError());
        return false;
    }

    *core_profile = (profile & SDL_GL_CONTEXT_PROFILE_CORE) != 0;
    return true;
}

bool hth_platform_framebuffer_size(HTHPlatform *platform,
                                   uint32_t *width, uint32_t *height)
{
    int pixel_width;
    int pixel_height;

    if (platform == NULL || platform->window == NULL ||
        width == NULL || height == NULL) {
        return false;
    }
    if (!SDL_GetWindowSizeInPixels(platform->window,
                                   &pixel_width, &pixel_height)) {
        fprintf(stderr, "SDL framebuffer size query failed: %s\n",
                SDL_GetError());
        return false;
    }
    if (pixel_width < 0 || pixel_height < 0) {
        return false;
    }

    *width = (uint32_t)pixel_width;
    *height = (uint32_t)pixel_height;
    return true;
}

HTHGraphicsProcedure hth_platform_graphics_get_proc_address(
    HTHPlatform *platform, const char *name)
{
    if (platform == NULL || platform->window == NULL || name == NULL) {
        return NULL;
    }
    return SDL_GL_GetProcAddress(name);
}
