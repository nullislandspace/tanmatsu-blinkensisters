// SDL_gfxPrimitives stub for Tanmatsu port
#pragma once
#include "pal/pal_types.h"
#include "pal/pal_surface.h"
#include "pal/pal_screen.h"

// pixelRGBA: draw a single pixel at (x,y) with RGBA color
static inline int pixelRGBA(SDL_Surface* dst, Sint16 x, Sint16 y,
                             Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    (void)a;
    if (!dst) return -1;
    Uint32 color = 0xff000000 | ((Uint32)b << 16) | ((Uint32)g << 8) | r;
    BS_SetPixel(dst, x, y, color);
    return 0;
}

// circleColor: draw circle outline using midpoint algorithm
static inline int circleColor(SDL_Surface* dst, Sint16 cx, Sint16 cy, Sint16 rad, Uint32 color) {
    if (!dst || rad <= 0) return -1;
    // Extract RGBA from packed color (stored as 0xRRGGBBAA in SDL_gfx convention)
    Uint8 r = (Uint8)((color >> 24) & 0xff);
    Uint8 g = (Uint8)((color >> 16) & 0xff);
    Uint8 b = (Uint8)((color >> 8)  & 0xff);
    // Our format: R=bits0-7, G=bits8-15, B=bits16-23, A=bits24-31
    Uint32 px = 0xff000000 | ((Uint32)b << 16) | ((Uint32)g << 8) | r;
    // Midpoint circle algorithm
    Sint16 x = 0, y = rad, d = 1 - rad;
    while (x <= y) {
        BS_SetPixel(dst, cx+x, cy+y, px); BS_SetPixel(dst, cx-x, cy+y, px);
        BS_SetPixel(dst, cx+x, cy-y, px); BS_SetPixel(dst, cx-x, cy-y, px);
        BS_SetPixel(dst, cx+y, cy+x, px); BS_SetPixel(dst, cx-y, cy+x, px);
        BS_SetPixel(dst, cx+y, cy-x, px); BS_SetPixel(dst, cx-y, cy-x, px);
        if (d < 0) { d += 2*x + 3; }
        else       { d += 2*(x-y) + 5; y--; }
        x++;
    }
    return 0;
}

// IMG_Load is defined in pal/pal_surface.h (included via globals.h)
