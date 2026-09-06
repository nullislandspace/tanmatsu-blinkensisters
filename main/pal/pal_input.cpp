#include "pal_input.h"
#include "bsp/input.h"
#include "bsp/device.h"
// Include joystick.h for JOYSTICK_* enum values
// joystick.h includes pal_types.h for Uint32, and defines JOYSTICK_MOVE enum
#include "../game/joystick.h"

static QueueHandle_t s_input_queue = NULL;

// --- Button level state -------------------------------------------------
// These track "is the key down right now" and drive PAL_GetJoystickMoves(),
// which gameplay uses for continuous movement.
static bool s_jump_pressed   = false;
static bool s_action_pressed = false;
static bool s_pause_pressed  = false;
static bool s_turbo_pressed  = false;

// Navigation key state
static bool s_left  = false;
static bool s_right = false;
static bool s_up    = false;
static bool s_down  = false;

// --- Button release edges ----------------------------------------------
// Menus and one-shot toggles (pause) must fire when a key is RELEASED, not
// while it is held: acting on the press means the matching release is still
// queued when the next screen opens, and that screen then consumes it as its
// own input (pressing "Play" used to run straight through the addon list and
// launch the first entry). Releases accumulate here and are consumed — and
// cleared — by PAL_GetJoystickReleases().
static Uint32 s_released_edges = 0;

// Apply a press/release to a button's level state, recording a release edge.
static void update_button(bool* state, bool pressed, Uint32 button) {
    if (*state && !pressed) {
        s_released_edges |= button;
    }
    *state = pressed;
}

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

// Map a PC scancode to its SDLK value (0 = unmapped).
//
// Scancodes are the only input source that reports both press AND release for
// every key, so they are the sole source of SDL key events here. The BSP's
// INPUT_EVENT_TYPE_KEYBOARD (ASCII) stream is deliberately ignored: it fires
// on press only and auto-repeats while held, which the SDL-era game code reads
// as a storm of spurious keystrokes.
static int scancode_to_sdlk(uint32_t sc) {
    switch (sc) {
        case BSP_INPUT_SCANCODE_ESC:        return SDLK_ESCAPE;
        case BSP_INPUT_SCANCODE_TAB:        return SDLK_TAB;
        case BSP_INPUT_SCANCODE_ENTER:      return SDLK_RETURN;
        case BSP_INPUT_SCANCODE_BACKSPACE:  return SDLK_BACKSPACE;
        case BSP_INPUT_SCANCODE_MINUS:      return SDLK_MINUS;
        case BSP_INPUT_SCANCODE_EQUAL:      return SDLK_PLUS;
        case BSP_INPUT_SCANCODE_RIGHTBRACE: return SDLK_RIGHTBRACKET;

        case BSP_INPUT_SCANCODE_ESCAPED_GREY_UP:    return SDLK_UP;
        case BSP_INPUT_SCANCODE_ESCAPED_GREY_DOWN:  return SDLK_DOWN;
        case BSP_INPUT_SCANCODE_ESCAPED_GREY_LEFT:  return SDLK_LEFT;
        case BSP_INPUT_SCANCODE_ESCAPED_GREY_RIGHT: return SDLK_RIGHT;
        case BSP_INPUT_SCANCODE_ESCAPED_KPENTER:    return SDLK_RETURN;

        case BSP_INPUT_SCANCODE_A: return SDLK_a;
        case BSP_INPUT_SCANCODE_B: return SDLK_b;
        case BSP_INPUT_SCANCODE_C: return SDLK_c;
        case BSP_INPUT_SCANCODE_D: return SDLK_d;
        case BSP_INPUT_SCANCODE_E: return SDLK_e;
        case BSP_INPUT_SCANCODE_F: return SDLK_f;
        case BSP_INPUT_SCANCODE_G: return SDLK_g;
        case BSP_INPUT_SCANCODE_H: return SDLK_h;
        case BSP_INPUT_SCANCODE_I: return SDLK_i;
        case BSP_INPUT_SCANCODE_J: return SDLK_j;
        case BSP_INPUT_SCANCODE_K: return SDLK_k;
        case BSP_INPUT_SCANCODE_L: return SDLK_l;
        case BSP_INPUT_SCANCODE_M: return SDLK_m;
        case BSP_INPUT_SCANCODE_N: return SDLK_n;
        case BSP_INPUT_SCANCODE_O: return SDLK_o;
        case BSP_INPUT_SCANCODE_P: return SDLK_p;
        case BSP_INPUT_SCANCODE_Q: return SDLK_q;
        case BSP_INPUT_SCANCODE_R: return SDLK_r;
        case BSP_INPUT_SCANCODE_S: return SDLK_s;
        case BSP_INPUT_SCANCODE_T: return SDLK_t;
        case BSP_INPUT_SCANCODE_U: return SDLK_u;
        case BSP_INPUT_SCANCODE_V: return SDLK_v;
        case BSP_INPUT_SCANCODE_W: return SDLK_w;
        case BSP_INPUT_SCANCODE_X: return SDLK_x;
        case BSP_INPUT_SCANCODE_Y: return SDLK_y;
        case BSP_INPUT_SCANCODE_Z: return SDLK_z;

        case BSP_INPUT_SCANCODE_0: return '0';
        case BSP_INPUT_SCANCODE_1: return '1';
        case BSP_INPUT_SCANCODE_2: return '2';
        case BSP_INPUT_SCANCODE_3: return '3';
        case BSP_INPUT_SCANCODE_4: return '4';
        case BSP_INPUT_SCANCODE_5: return '5';
        case BSP_INPUT_SCANCODE_6: return '6';
        case BSP_INPUT_SCANCODE_7: return '7';
        case BSP_INPUT_SCANCODE_8: return '8';
        case BSP_INPUT_SCANCODE_9: return '9';

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
                case BSP_INPUT_NAVIGATION_KEY_LEFT:
                    update_button(&s_left, pressed, JOYSTICK_LEFT);
                    break;
                case BSP_INPUT_NAVIGATION_KEY_RIGHT:
                    update_button(&s_right, pressed, JOYSTICK_RIGHT);
                    break;
                case BSP_INPUT_NAVIGATION_KEY_UP:
                    update_button(&s_up, pressed, JOYSTICK_UP);
                    break;
                case BSP_INPUT_NAVIGATION_KEY_DOWN:
                    update_button(&s_down, pressed, JOYSTICK_DOWN);
                    break;

                case BSP_INPUT_NAVIGATION_KEY_SPACE_M:
                case BSP_INPUT_NAVIGATION_KEY_SPACE_L:
                case BSP_INPUT_NAVIGATION_KEY_SPACE_R:
                    // The space bars report no scancode, so synthesise the SDL
                    // key events the game expects for SDLK_SPACE here.
                    update_button(&s_jump_pressed, pressed, JOYSTICK_JUMP);
                    push_sdl_event(pressed ? SDL_KEYDOWN : SDL_KEYUP, SDLK_SPACE);
                    break;

                case BSP_INPUT_NAVIGATION_KEY_RETURN:
                    update_button(&s_action_pressed, pressed, JOYSTICK_ACTION);
                    break;

                case BSP_INPUT_NAVIGATION_KEY_F1:
                    update_button(&s_pause_pressed, pressed, JOYSTICK_PAUSE);
                    break;

                case BSP_INPUT_NAVIGATION_KEY_F2:
                    update_button(&s_turbo_pressed, pressed, JOYSTICK_TURBO);
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
        }
        // INPUT_EVENT_TYPE_KEYBOARD is intentionally dropped: see
        // scancode_to_sdlk() above.
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

Uint32 PAL_GetJoystickReleases(void) {
    drain_input_queue();
    Uint32 edges = s_released_edges;
    s_released_edges = 0;
    return edges;
}

void PAL_InputFlush(void) {
    // Throw away everything queued and every edge, but keep the level state in
    // sync with reality by re-reading it from the events we drop.
    drain_input_queue();
    s_released_edges = 0;
    s_sdl_event_head = 0;
    s_sdl_event_tail = 0;
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
