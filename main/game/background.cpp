// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#include "globals.h"
#include "background.h"
#include "errorhandler.h"
#include "drawprimitives.h"

#ifdef DISABLE_BACKGROUND_ART

void initBackground(const char *fname) {
	(void)fname;
}

void deInitBackground() {
}

void drawBackground(Sint32 xoffs, Sint32 yoffs) {
	(void)xoffs;
	(void)yoffs;
	drawrect(0, 0, SCR_WIDTH, SCR_HEIGHT, 0x000000);
}

Uint32 getBGWidth() {
	return SCR_WIDTH * 2;
}

Uint32 getBGHeight() {
	return SCR_HEIGHT * 2;
}

#else // DISABLE_BACKGROUND_ART

SDL_Surface *BG_Surface;

/* The background is kept exactly as loaded and drawn with wrap-around, rather
   than copied into a surface padded by one screen in each direction so a
   single blit could run off the edge.
   
   That padding was ruinous here. LostPixels' largest background is 3328x952:
   padded it needs 22.5 MB, and initBackground built it by allocating the
   padded surface AND an immediate duplicate of it (SDL_DisplayFormat, which
   on this port is just a copy) while the 12.1 MB decoded original was still
   alive -- about 57 MB peak on a device with 32 MB of PSRAM. Wrapping at draw
   time costs up to four blits on the frames that straddle an edge, and none
   of that memory. */
void initBackground(const char *fname) {
	BG_Surface = BS_IMG_Load_DisplayFormat(configGetPath(fname), DIE_ON_FILE_ERROR);
	if(!BG_Surface) {
		return;
	}
	/* Static and fully opaque, so whole-screen blits from it can go to the
	   PPA (see BS_SurfaceFinalize). */
	BS_SurfaceFinalize(BG_Surface);
}

void deInitBackground() {
	SDL_FreeSurface(BG_Surface);
	BG_Surface = 0;
}

/* Cover the screen from `src`, repeating it in both directions. Up to four
   blits: one per quadrant the wrap splits the screen into. When the offset
   happens not to straddle an edge -- the common case for a background larger
   than the screen -- this is a single full-screen blit, which is the shape
   the PPA path picks up. */
void blitTiledBackground(SDL_Surface* src, Sint32 xoffs, Sint32 yoffs) {
	if(!src || src->w <= 0 || src->h <= 0) {
		return;
	}
	Sint32 bw = src->w;
	Sint32 bh = src->h;
	xoffs = ((xoffs % bw) + bw) % bw;
	yoffs = ((yoffs % bh) + bh) % bh;

	Sint32 dy = 0;
	while(dy < SCR_HEIGHT) {
		Sint32 sy = (yoffs + dy) % bh;
		Sint32 hh = bh - sy;
		if(hh > SCR_HEIGHT - dy) hh = SCR_HEIGHT - dy;

		Sint32 dx = 0;
		while(dx < SCR_WIDTH) {
			Sint32 sx = (xoffs + dx) % bw;
			Sint32 ww = bw - sx;
			if(ww > SCR_WIDTH - dx) ww = SCR_WIDTH - dx;

			SDL_Rect srcrect, dstrect;
			srcrect.x = sx; srcrect.y = sy; srcrect.w = ww; srcrect.h = hh;
			dstrect.x = dx; dstrect.y = dy; dstrect.w = ww; dstrect.h = hh;
			SDL_BlitSurface(src, &srcrect, gScreen, &dstrect);
			dx += ww;
		}
		dy += hh;
	}
}

void drawBackground(Sint32 xoffs, Sint32 yoffs) {
	blitTiledBackground(BG_Surface, xoffs, yoffs);
}

Uint32 getBGWidth() {
	return BG_Surface ? (Uint32)BG_Surface->w : (Uint32)SCR_WIDTH;
}

Uint32 getBGHeight() {
	return BG_Surface ? (Uint32)BG_Surface->h : (Uint32)SCR_HEIGHT;
}

#endif // DISABLE_BACKGROUND_ART

