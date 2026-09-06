#ifndef DRAWPRIMITIVES_H
#define DRAWPRIMITIVES_H
#include "globals.h"

void drawrect(const Sint32 x, const Sint32 y, const Sint32 width, const Sint32 height, const Uint32 color);
Uint32 getpixel(SDL_Surface* screen, int x, int y);
Uint32 SDL_color_to_Uint32(SDL_Color sdc);

#define DIE_ON_FILE_ERROR  true
#define IGNORE_FILE_ERROR  false
SDL_Surface* BS_IMG_Load_DisplayFormat(const char* filename, bool die_on_error);

/* Bilinear rescale into a new surface. Returns NULL on failure; the source is
   left alone. */
SDL_Surface* BS_ScaleSurface(const SDL_Surface* src, Sint32 dst_w, Sint32 dst_h);

/* Load a full-screen backdrop, stretched to the screen if it is not already
   that size. The artwork was drawn for a 4:3 display and this one is wider,
   so menus, the loading screen and the end screens would otherwise sit in a
   letterboxed 640x480 patch. */
SDL_Surface* BS_IMG_Load_Fullscreen(const char* filename, bool die_on_error);

#endif // DRAWPRIMITIVES_H
