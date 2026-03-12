#include "pal_input.h"
#include "bsp/input.h"
#include "bsp/device.h"
#include "esp_log.h"
// Include joystick.h for JOYSTICK_* enum values
// joystick.h includes pal_types.h for Uint32, and defines JOYSTICK_MOVE enum
#include "../game/joystick.h"

static const char* TAG = "pal_input";

static QueueHandle_t s_input_queue = NULL;

// Persistent state for button tracking (to detect edges)
static bool s_jump_pressed   = false;
static bool s_action_pressed = false;
static bool s_pause_pressed  = false;
static bool s_turbo_pressed  = false;

// Navigation key state
static bool s_left  = false;
static bool s_right = false;
static bool s_up    = false;
static bool s_down  = false;

// --- SDL_Event ring buffer for keyboard events ---
#define MAX_SDL_EVENTS 32
static SDL_Event s_sdl_events[MAX_SDL_EVENTS];
static int s_sdl_event_head = 0;
static int s_sdl_event_tail = 0;

static void push_sdl_event(int type, int keysym) {
    int next = (s_sdl_event_head + 1) % MAX_SDL_EVENTS;
    if (next == s_sdl_event_tail) return; // buffer full, drop event
    s_sdl_events[s_sdl_event_head].type = type;
    s_sdl_events[s_sdl_event_head].key.keysym.sym = keysym;
    s_sdl_event_head = next;
}

// Map PC scancode to SDLK value (0 = unmapped)
static int scancode_to_sdlk(uint32_t sc) {
    switch (sc) {
        case BSP_INPUT_SCANCODE_ESC:   return SDLK_ESCAPE;
        case BSP_INPUT_SCANCODE_TAB:   return SDLK_TAB;
        case BSP_INPUT_SCANCODE_ENTER: return SDLK_RETURN;
        case BSP_INPUT_SCANCODE_SPACE: return SDLK_SPACE;
        default: return 0;
    }
}

void PAL_InputInit(QueueHandle_t input_queue) {
    s_input_queue = input_queue;
}

// Drain BSP input queue, updating joystick state AND populating SDL event buffer
static void drain_input_queue(void) {
    if (!s_input_queue) return;

    bsp_input_event_t event;
    while (xQueueReceive(s_input_queue, &event, 0) == pdTRUE) {
        if (event.type == INPUT_EVENT_TYPE_NAVIGATION) {
            bool pressed = event.args_navigation.state;
            uint32_t key = (uint32_t)event.args_navigation.key;

            switch (key) {
                case BSP_INPUT_NAVIGATION_KEY_LEFT:  s_left  = pressed; break;
                case BSP_INPUT_NAVIGATION_KEY_RIGHT: s_right = pressed; break;
                case BSP_INPUT_NAVIGATION_KEY_UP:    s_up    = pressed; break;
                case BSP_INPUT_NAVIGATION_KEY_DOWN:  s_down  = pressed; break;

                case BSP_INPUT_NAVIGATION_KEY_SPACE_M:
                case BSP_INPUT_NAVIGATION_KEY_SPACE_L:
                    if (pressed && !s_jump_pressed) s_jump_pressed = true;
                    else if (!pressed) s_jump_pressed = false;
                    break;

                case BSP_INPUT_NAVIGATION_KEY_RETURN:
                    if (pressed && !s_action_pressed) s_action_pressed = true;
                    else if (!pressed) s_action_pressed = false;
                    break;

                case BSP_INPUT_NAVIGATION_KEY_F1:
                    if (pressed) s_pause_pressed = true;
                    else s_pause_pressed = false;
                    break;

                case BSP_INPUT_NAVIGATION_KEY_F2:
                    if (pressed && !s_turbo_pressed) s_turbo_pressed = true;
                    else if (!pressed) s_turbo_pressed = false;
                    break;

                default:
                    break;
            }
        } else if (event.type == INPUT_EVENT_TYPE_SCANCODE) {
            uint32_t sc = (uint32_t)event.args_scancode.scancode;
            bool released = (sc & BSP_INPUT_SCANCODE_RELEASE_MODIFIER) != 0;
            sc &= ~BSP_INPUT_SCANCODE_RELEASE_MODIFIER;
            int sdlk = scancode_to_sdlk(sc);
            if (sdlk) {
                push_sdl_event(released ? SDL_KEYUP : SDL_KEYDOWN, sdlk);
            }
        } else if (event.type == INPUT_EVENT_TYPE_KEYBOARD) {
            // ASCII key press — generate KEYUP (game checks KEYUP for actions)
            char c = event.args_keyboard.ascii;
            if (c >= 'a' && c <= 'z') {
                push_sdl_event(SDL_KEYUP, (int)c);
            } else if (c >= 'A' && c <= 'Z') {
                push_sdl_event(SDL_KEYUP, (int)(c - 'A' + 'a')); // lowercase SDLK
            } else if (c >= '0' && c <= '9') {
                push_sdl_event(SDL_KEYUP, (int)c);
            }
        }
    }
}

Uint32 PAL_GetJoystickMoves(void) {
    drain_input_queue();

    // Build result bitmask
    Uint32 result = JOYSTICK_NONE;
    if (s_left)          result |= JOYSTICK_LEFT;
    if (s_right)         result |= JOYSTICK_RIGHT;
    if (s_up)            result |= JOYSTICK_UP;
    if (s_down)          result |= JOYSTICK_DOWN;
    if (s_up || s_jump_pressed) result |= JOYSTICK_JUMP;
    if (s_action_pressed) result |= JOYSTICK_ACTION;
    if (s_pause_pressed)  result |= JOYSTICK_PAUSE;
    if (s_turbo_pressed)  result |= JOYSTICK_TURBO;

    return result;
}

int PAL_PollEvent(SDL_Event* e) {
    // First call per frame also drains the queue (in case SDL_PollEvent is called
    // before getJoystickMoves)
    drain_input_queue();

    if (s_sdl_event_tail == s_sdl_event_head) return 0;
    *e = s_sdl_events[s_sdl_event_tail];
    s_sdl_event_tail = (s_sdl_event_tail + 1) % MAX_SDL_EVENTS;
    return 1;
}
