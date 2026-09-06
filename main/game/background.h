// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#ifndef BACKGROUND_H
#define BACKGROUND_H

void initBackground(const char *fname);
void deInitBackground();
void drawBackground(Sint32 xoffs, Sint32 yoffs);

#ifndef DISABLE_BACKGROUND_ART
/* Cover the screen from `src`, repeating it in both directions. Shared with
   the parallax layer in background2.cpp. */
void blitTiledBackground(SDL_Surface* src, Sint32 xoffs, Sint32 yoffs);
#endif
Uint32 getBGWidth();
Uint32 getBGHeight();

#endif // BACKGROUND_H
