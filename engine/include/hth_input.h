#ifndef HTH_INPUT_H
#define HTH_INPUT_H

#include <stdbool.h>

typedef struct HTHInput HTHInput;

typedef enum {
    HTH_KEY_UNKNOWN = 0,
    HTH_KEY_A, HTH_KEY_B, HTH_KEY_C, HTH_KEY_D, HTH_KEY_E, HTH_KEY_F,
    HTH_KEY_G, HTH_KEY_H, HTH_KEY_I, HTH_KEY_J, HTH_KEY_K, HTH_KEY_L,
    HTH_KEY_M, HTH_KEY_N, HTH_KEY_O, HTH_KEY_P, HTH_KEY_Q, HTH_KEY_R,
    HTH_KEY_S, HTH_KEY_T, HTH_KEY_U, HTH_KEY_V, HTH_KEY_W, HTH_KEY_X,
    HTH_KEY_Y, HTH_KEY_Z,
    HTH_KEY_0, HTH_KEY_1, HTH_KEY_2, HTH_KEY_3, HTH_KEY_4,
    HTH_KEY_5, HTH_KEY_6, HTH_KEY_7, HTH_KEY_8, HTH_KEY_9,
    HTH_KEY_ESCAPE, HTH_KEY_ENTER, HTH_KEY_SPACE, HTH_KEY_TAB,
    HTH_KEY_BACKSPACE,
    HTH_KEY_UP, HTH_KEY_DOWN, HTH_KEY_LEFT, HTH_KEY_RIGHT,
    HTH_KEY_LSHIFT, HTH_KEY_RSHIFT, HTH_KEY_LCTRL, HTH_KEY_RCTRL,
    HTH_KEY_LALT, HTH_KEY_RALT,
    HTH_KEY_F1, HTH_KEY_F2, HTH_KEY_F3, HTH_KEY_F4, HTH_KEY_F5,
    HTH_KEY_F6, HTH_KEY_F7, HTH_KEY_F8, HTH_KEY_F9, HTH_KEY_F10,
    HTH_KEY_F11, HTH_KEY_F12,
    HTH_KEY_COUNT
} HTHKey;

typedef enum {
    HTH_MOUSE_UNKNOWN = 0,
    HTH_MOUSE_LEFT,
    HTH_MOUSE_MIDDLE,
    HTH_MOUSE_RIGHT,
    HTH_MOUSE_X1,
    HTH_MOUSE_X2,
    HTH_MOUSE_BUTTON_COUNT
} HTHMouseButton;

bool hth_input_key_down(const HTHInput *input, HTHKey key);
bool hth_input_key_pressed(const HTHInput *input, HTHKey key);
bool hth_input_key_released(const HTHInput *input, HTHKey key);
bool hth_input_mouse_button_down(const HTHInput *input, HTHMouseButton button);
bool hth_input_mouse_button_pressed(const HTHInput *input, HTHMouseButton button);
bool hth_input_mouse_button_released(const HTHInput *input, HTHMouseButton button);
void hth_input_mouse_position(const HTHInput *input, double *x, double *y);
void hth_input_mouse_delta(const HTHInput *input, double *x, double *y);
void hth_input_mouse_wheel(const HTHInput *input, double *x, double *y);

#endif
