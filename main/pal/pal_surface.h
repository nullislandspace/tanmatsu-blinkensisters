#pragma once
#include "pal_types.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Named format struct to avoid anonymous struct type mismatch in C++
typedef struct BS_Surface_Format {
    Sint32 BytesPerPixel;  // always 4
} BS_Surface_Format;

// BS_Surface replaces SDL_Surface
typedef struct {
    Sint32  w, h;
    Sint32  pitch;      // bytes per row = w * 4
    Uint32 *pixels;     // RGBA32 pixels in PSRAM
    Uint32  colorkey;   // colorkey pixel value (if colorkey_enabled)
    bool    colorkey_enabled;
    Uint32  format_flags; // unused, kept for compat
    BS_Surface_Format *format;
    BS_Surface_Format  _format_data;
} BS_Surface;

// SDL_Surface typedef
typedef BS_Surface SDL_Surface;

// Surface management
BS_Surface* BS_CreateSurface(Sint32 w, Sint32 h);
void        BS_FreeSurface(BS_Surface* s);
BS_Surface* BS_DupSurface(const BS_Surface* src);

// SDL API wrappers
static inline BS_Surface* SDL_CreateRGBSurface(Uint32 flags, Sint32 w, Sint32 h,
    Sint32 depth, Uint32 rmask, Uint32 gmask, Uint32 bmask, Uint32 amask) {
    (void)flags; (void)depth; (void)rmask; (void)gmask; (void)bmask; (void)amask;
    return BS_CreateSurface(w, h);
}
static inline void SDL_FreeSurface(BS_Surface* s) { BS_FreeSurface(s); }
static inline BS_Surface* SDL_DisplayFormat(BS_Surface* s) { return BS_DupSurface(s); }
static inline BS_Surface* SDL_DisplayFormatAlpha(BS_Surface* s) { return BS_DupSurface(s); }

// IMG_Load: SDL_image replacement - forward declare actual impl from drawprimitives.cpp
#ifdef __cplusplus
BS_Surface* BS_IMG_Load_DisplayFormat(const char* filename, bool die_on_error);
static inline BS_Surface* IMG_Load(const char* filename) {
    return BS_IMG_Load_DisplayFormat(filename, false);
}
#endif

// Blitting
int BS_BlitSurface(BS_Surface* src, const SDL_Rect* srcrect,
                   BS_Surface* dst, SDL_Rect* dstrect);
static inline int SDL_BlitSurface(BS_Surface* src, const SDL_Rect* srcrect,
                                   BS_Surface* dst, SDL_Rect* dstrect) {
    return BS_BlitSurface(src, srcrect, dst, dstrect);
}

// Fill
int BS_FillRect(BS_Surface* dst, const SDL_Rect* rect, Uint32 color);
static inline int SDL_FillRect(BS_Surface* dst, const SDL_Rect* rect, Uint32 color) {
    return BS_FillRect(dst, rect, color);
}

// Color key
int BS_SetColorKey(BS_Surface* s, Uint32 flag, Uint32 key);
static inline int SDL_SetColorKey(BS_Surface* s, Uint32 flag, Uint32 key) {
    return BS_SetColorKey(s, flag, key);
}
#define SDL_SRCCOLORKEY 1

// MapRGB - pack R,G,B into our pixel format (RGBA, A=255)
static inline Uint32 SDL_MapRGB(void* fmt, Uint8 r, Uint8 g, Uint8 b) {
    (void)fmt;
    return (Uint32)0xff000000 | ((Uint32)b << 16) | ((Uint32)g << 8) | r;
}

static inline Uint32 BS_MapRGBA(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    return ((Uint32)a << 24) | ((Uint32)b << 16) | ((Uint32)g << 8) | r;
}

// Pixel access helper
static inline Uint32 BS_GetPixel(const BS_Surface* s, int x, int y) {
    if (x < 0 || x >= s->w || y < 0 || y >= s->h) return 0;
    return s->pixels[y * s->w + x];
}

static inline void BS_SetPixel(BS_Surface* s, int x, int y, Uint32 color) {
    if (x < 0 || x >= s->w || y < 0 || y >= s->h) return;
    s->pixels[y * s->w + x] = color;
}

#ifdef __cplusplus
}
#endif
