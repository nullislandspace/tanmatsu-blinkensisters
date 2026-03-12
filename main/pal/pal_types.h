#pragma once
#include <stdint.h>

// SDL primitive type aliases
typedef uint8_t  Uint8;
typedef uint16_t Uint16;
typedef uint32_t Uint32;
typedef int8_t   Sint8;
typedef int16_t  Sint16;
typedef int32_t  Sint32;

// SDL_Color
typedef struct { Uint8 r, g, b, unused; } SDL_Color;

// SDL_Rect
typedef struct { Sint16 x, y; Uint16 w, h; } SDL_Rect;

// SDL surface flags (stubs)
#define SDL_SWSURFACE   0
#define SDL_FULLSCREEN  0x80000000

// Misc SDL macros used in game code
#define SDL_min(a,b)    ((a) < (b) ? (a) : (b))
#define SDL_max(a,b)    ((a) > (b) ? (a) : (b))
#define SDL_BYTEORDER   1234      // SDL_LIL_ENDIAN
#define SDL_LIL_ENDIAN  1234
#define SDL_BIG_ENDIAN  4321

// Stub for SDL_MUSTLOCK - always false (no lock needed)
#define SDL_MUSTLOCK(s) (0)
#define SDL_LockSurface(s)   (0)
#define SDL_UnlockSurface(s) ((void)0)

// RWops stub (used by sound.cpp for video fx - not used in Tanmatsu build)
typedef void SDL_RWops;
