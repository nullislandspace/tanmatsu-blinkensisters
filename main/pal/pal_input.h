#pragma once
#include "pal_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "bsp/input.h"

// JOYSTICK_MOVE is defined in game/joystick.h (kept unchanged)
// PAL_GetJoystickMoves() returns the same bitmask, using uint32_t for PAL layer

#ifdef __cplusplus
extern "C" {
#endif

// Initialize input system with BSP event queue handle
void PAL_InputInit(QueueHandle_t input_queue);

// Drain BSP event queue, return current joystick bitmask (JOYSTICK_* values).
// This is LEVEL state ("held right now") and is what gameplay movement uses.
uint32_t PAL_GetJoystickMoves(void);

// Buttons RELEASED since the last call, as a JOYSTICK_* bitmask; the edges are
// cleared by reading them. Menus and one-shot toggles use this instead of
// PAL_GetJoystickMoves() so an action fires once, on release, and the release
// cannot leak into whatever screen the action opened.
uint32_t PAL_GetJoystickReleases(void);

// Discard all queued key events and pending release edges. Call when switching
// screens so stale input from the previous screen is not acted on.
void PAL_InputFlush(void);

// SDL_Event stubs - SDL_PollEvent always returns 0; input via PAL_GetJoystickMoves
#define SDLK_UP        273
#define SDLK_DOWN      274
#define SDLK_LEFT      275
#define SDLK_RIGHT     276
#define SDLK_RETURN    13
#define SDLK_ESCAPE    27
#define SDLK_TAB       9
#define SDLK_SPACE     32
#define SDLK_BACKSPACE 8
// Printable ASCII keys (SDL uses ASCII values for these)
#define SDLK_MINUS        45
#define SDLK_PLUS         43
#define SDLK_RIGHTBRACKET 93
#define SDLK_a  97
#define SDLK_b  98
#define SDLK_c  99
#define SDLK_d  100
#define SDLK_e  101
#define SDLK_f  102
#define SDLK_g  103
#define SDLK_h  104
#define SDLK_i  105
#define SDLK_j  106
#define SDLK_k  107
#define SDLK_l  108
#define SDLK_m  109
#define SDLK_n  110
#define SDLK_o  111
#define SDLK_p  112
#define SDLK_q  113
#define SDLK_r  114
#define SDLK_s  115
#define SDLK_t  116
#define SDLK_u  117
#define SDLK_v  118
#define SDLK_w  119
#define SDLK_x  120
#define SDLK_y  121
#define SDLK_z  122
// Numpad keys (SDL values)
#define SDLK_KP0  256
#define SDLK_KP1  257
#define SDLK_KP2  258
#define SDLK_KP3  259
#define SDLK_KP4  260
#define SDLK_KP5  261
#define SDLK_KP6  262
#define SDLK_KP7  263
#define SDLK_KP8  264
#define SDLK_KP9  265

typedef struct {
    struct { int sym; } keysym;
} SDL_KeyboardEvent;

typedef struct {
    int type;
    SDL_KeyboardEvent key;
} SDL_Event;

#define SDL_KEYUP    3
#define SDL_KEYDOWN  2
#define SDL_QUIT     12

int PAL_PollEvent(SDL_Event* e);
#define SDL_PollEvent PAL_PollEvent

#define SDL_DISABLE 0
#define SDL_ENABLE  1
static inline int SDL_ShowCursor(int s) { (void)s; return 0; }

#ifdef __cplusplus
}
#endif
