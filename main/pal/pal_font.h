#pragma once
#include <stddef.h>   // for NULL
#include "pal_types.h"
#include "pal_surface.h"

#ifdef __cplusplus
extern "C" {
#endif

// Opaque font type - replaces TTF_Font
// We use size as the "font" (Hershey scales by pixel height)
typedef struct { int size; } BS_Font;
typedef BS_Font TTF_Font;

// Font instances matching original fonthandler.h
extern TTF_Font* FONT_menufont_50;
extern TTF_Font* FONT_menufont_30;
extern TTF_Font* FONT_textfont_30;
extern TTF_Font* FONT_menufont_20;
extern TTF_Font* FONT_textfont_20;
extern TTF_Font* FONT_textfont_12;
extern TTF_Font* FONT_textfont_8;

// Init/deinit
void initFontHandler(void);
void deInitFontHandler(void);

// Render text to gScreen using Hershey vector font
// x,y: top-left position in logical screen space
// text: string (may contain '\n' for newlines)
// fontcolor: SDL_Color
// hcentered: center horizontally
// vcentered: center vertically
// renderfont: font size specification
void renderFontHandlerText(Sint32 x, Sint32 y, const char* text,
                           SDL_Color fontcolor, bool hcentered, bool vcentered,
                           TTF_Font* renderfont);

// Width in pixels of `text` in `renderfont`, for the widest line if it has
// several. The Hershey glyphs are considerably wider than the TTF faces the
// original layouts were measured against, so anything laid out in fixed
// columns has to ask rather than assume.
int fontHandlerTextWidth(const char* text, TTF_Font* renderfont);

// Common colors
extern SDL_Color BS_Color_WHITE;
extern SDL_Color BS_Color_RED;
extern SDL_Color MENUCOLOR_ACTIVE;
extern SDL_Color MENUCOLOR_INACTIVE;

// TTF stubs
// TTF_RenderText_Blended: returns NULL (text rendered via Hershey, not TTF surfaces)
static inline BS_Surface* TTF_RenderText_Blended(TTF_Font* f, const char* text, SDL_Color c) {
    (void)f; (void)text; (void)c; return NULL;
}
static inline int TTF_Init(void) { return 0; }
static inline int TTF_WasInit(void) { return 1; }
static inline TTF_Font* TTF_OpenFont(const char* f, int s) { (void)f; (void)s; return NULL; }
static inline void TTF_CloseFont(TTF_Font* f) { (void)f; }
static inline void TTF_Quit(void) {}
static inline const char* TTF_GetError(void) { return "no TTF"; }
static inline int TTF_FontLineSkip(TTF_Font* f) { return f ? (int)(f->size * 1.3f) : 20; }

#ifdef __cplusplus
}
#endif
