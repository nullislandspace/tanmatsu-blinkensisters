#pragma once
#include "pal_types.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// =====================================================================
//  Surfaces are stored as RGB565, 16 bits per pixel.
//
//  The frame is bandwidth-bound: on this hardware the PPA moves about
//  124 MB/s, and at 32 bits per pixel the full-screen background copy and
//  the logical->panel rotation alone move 5.7 MB, which is 46 ms of a
//  56 ms frame. Halving the pixel halves both.
//
//  COLOURS IN THE GAME CODE STAY 32-BIT. Everything above this layer --
//  blending, the Lua bindings, drawrect, SDL_Color -- keeps passing
//  0xAABBGGRR values around; only the storage is narrow. BS_GetPixel()
//  hands back a 32-bit colour and BS_SetPixel() takes one, so the pack
//  and unpack happen at the one boundary where pixels are touched.
//
//  TRANSPARENCY. RGB565 has no alpha channel, and the old blitter only
//  ever used alpha as a binary "skip this pixel" test, so nothing is
//  lost: a fully transparent pixel becomes BS_TRANSPARENT, a reserved
//  colour the blitter skips. Loaders nudge any opaque pixel that happens
//  to land on that exact value, so it can never be mistaken for a real
//  one. Explicit colour keys (BS_SetColorKey) work as before.
// =====================================================================

typedef uint16_t BS_Pixel;

// Reserved "nothing here" colour: pure magenta. An opaque pixel that lands
// on it is shifted to the neighbouring blue, a change of one part in 32.
#define BS_TRANSPARENT ((BS_Pixel)0xF81F)

// Named format struct to avoid anonymous struct type mismatch in C++
typedef struct BS_Surface_Format {
    Sint32 BytesPerPixel;  // always 2
} BS_Surface_Format;

// BS_Surface replaces SDL_Surface
typedef struct {
    Sint32    w, h;
    Sint32    pitch;      // bytes per row = w * 2
    BS_Pixel *pixels;     // RGB565 pixels in PSRAM
    BS_Pixel  colorkey;   // colorkey pixel value (if colorkey_enabled)
    bool      colorkey_enabled;
    Uint32    format_flags; // unused, kept for compat
    bool      ppa_src_ready; // pixels have been flushed for PPA DMA reads
    BS_Surface_Format *format;
    BS_Surface_Format  _format_data;
} BS_Surface;

// SDL_Surface typedef
typedef BS_Surface SDL_Surface;

// --- Pixel packing ----------------------------------------------------
// The game's 32-bit colour is 0xAABBGGRR (byte order R,G,B,A); RGB565 is
// R in bits 15:11, G in 10:5, B in 4:0. Unpacking replicates the high bits
// down into the low ones so full-scale values stay full-scale.

static inline BS_Pixel BS_Pack(Uint32 c) {
    return (BS_Pixel)((((c      ) & 0xF8) << 8) |   // R
                      (((c >>  8) & 0xFC) << 3) |   // G
                      (((c >> 16) & 0xF8) >> 3));   // B
}

static inline Uint32 BS_Unpack(BS_Pixel p) {
    Uint32 r = (Uint32)((p >> 11) & 0x1F); r = (r << 3) | (r >> 2);
    Uint32 g = (Uint32)((p >>  5) & 0x3F); g = (g << 2) | (g >> 4);
    Uint32 b = (Uint32)( p        & 0x1F); b = (b << 3) | (b >> 2);
    return 0xff000000u | (b << 16) | (g << 8) | r;
}

// Pack an opaque colour, keeping it distinguishable from BS_TRANSPARENT.
static inline BS_Pixel BS_PackOpaque(Uint32 c) {
    BS_Pixel p = BS_Pack(c);
    return (p == BS_TRANSPARENT) ? (BS_Pixel)(BS_TRANSPARENT - 1) : p;
}

// Surface management
BS_Surface* BS_CreateSurface(Sint32 w, Sint32 h);
void        BS_FreeSurface(BS_Surface* s);
BS_Surface* BS_DupSurface(const BS_Surface* src);

// "I am done drawing into this surface, and every pixel of it is opaque."
// Pushes its pixels out of the CPU cache and marks it usable as a PPA source,
// which lets BS_BlitSurface hand whole-screen copies from it to the hardware.
//
// Both halves of that promise matter. The flush is only valid until the next
// CPU write, so a surface redrawn each frame must not be marked. And the PPA
// copy is a plain block move: it does not honour the colour key or the
// BS_TRANSPARENT test the CPU blitter applies, so marking a surface that has
// transparent pixels would paint them opaque. Backgrounds qualify; sprites
// and the green-to-transparent parallax layer do not.
void BS_SurfaceFinalize(BS_Surface* s);

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

// Fill. `color` is a 32-bit game colour.
int BS_FillRect(BS_Surface* dst, const SDL_Rect* rect, Uint32 color);
static inline int SDL_FillRect(BS_Surface* dst, const SDL_Rect* rect, Uint32 color) {
    return BS_FillRect(dst, rect, color);
}

// Color key. `key` is a 32-bit game colour.
int BS_SetColorKey(BS_Surface* s, Uint32 flag, Uint32 key);
static inline int SDL_SetColorKey(BS_Surface* s, Uint32 flag, Uint32 key) {
    return BS_SetColorKey(s, flag, key);
}
#define SDL_SRCCOLORKEY 1

// MapRGB - build the 32-bit game colour (not the storage format)
static inline Uint32 SDL_MapRGB(void* fmt, Uint8 r, Uint8 g, Uint8 b) {
    (void)fmt;
    return (Uint32)0xff000000 | ((Uint32)b << 16) | ((Uint32)g << 8) | r;
}

static inline Uint32 BS_MapRGBA(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    return ((Uint32)a << 24) | ((Uint32)b << 16) | ((Uint32)g << 8) | r;
}

// Pixel access helpers, in the 32-bit game colour. A transparent pixel reads
// back as 0, so the existing "is anything here" tests keep working.
static inline Uint32 BS_GetPixel(const BS_Surface* s, int x, int y) {
    if (x < 0 || x >= s->w || y < 0 || y >= s->h) return 0;
    BS_Pixel p = s->pixels[y * s->w + x];
    return (p == BS_TRANSPARENT) ? 0u : BS_Unpack(p);
}

static inline void BS_SetPixel(BS_Surface* s, int x, int y, Uint32 color) {
    if (x < 0 || x >= s->w || y < 0 || y >= s->h) return;
    s->pixels[y * s->w + x] = BS_PackOpaque(color);
}

#ifdef __cplusplus
}
#endif
