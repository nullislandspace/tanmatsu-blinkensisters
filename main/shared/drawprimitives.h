#ifndef DRAWPRIMITIVES_H
#define DRAWPRIMITIVES_H
#include "globals.h"

void drawrect(const Sint32 x, const Sint32 y, const Sint32 width, const Sint32 height, const Uint32 color);
Uint32 getpixel(SDL_Surface* screen, int x, int y);
Uint32 SDL_color_to_Uint32(SDL_Color sdc);

#define DIE_ON_FILE_ERROR  true
#define IGNORE_FILE_ERROR  false
SDL_Surface* BS_IMG_Load_DisplayFormat(const char* filename, bool die_on_error);

#endif // DRAWPRIMITIVES_H
