// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#include "globals.h"
#include "background2.h"
#include "errorhandler.h"
#include "convert.h"
#include "background.h"
#include "drawprimitives.h"

#ifdef DISABLE_BACKGROUND_ART

void initBackground2(const char *fname) {
	(void)fname;
}

void deInitBackground2() {
}

void drawBackground2(Sint32 xoffs, Sint32 yoffs) {
	(void)xoffs;
	(void)yoffs;
}

Uint32 getBG2Width() {
	return SCR_WIDTH * 2;
}

Uint32 getBG2Height() {
	return SCR_HEIGHT * 2;
}

#else // DISABLE_BACKGROUND_ART

SDL_Surface *BG_Surface2;

/* Same story as background.cpp: keep the layer as loaded and wrap at draw
   time, instead of copying it into a screen-padded surface. This one is NOT
   marked with BS_SurfaceFinalize -- convertToBSSurface turns its green key
   into alpha, so it has transparent pixels and must go through the CPU
   blitter that honours them. */
void initBackground2(const char *fname) {
	char fullfname[MAX_FNAME_LENGTH];
	sprintf(fullfname, "%s", configGetPath(fname));
	SDL_Surface* temp = IMG_Load(fullfname);
	if(!temp) {
		DIE(ERROR_IMAGE_READ, fullfname);
	}
	/* Green-to-alpha for the parallax layer's transparency. */
	BG_Surface2 = convertToBSSurface(temp);
	SDL_FreeSurface(temp);
}

void deInitBackground2() {
	SDL_FreeSurface(BG_Surface2);
	BG_Surface2 = 0;
}

void drawBackground2(Sint32 xoffs, Sint32 yoffs) {
	blitTiledBackground(BG_Surface2, xoffs, yoffs);
}

Uint32 getBG2Width() {
	return BG_Surface2 ? (Uint32)BG_Surface2->w : (Uint32)SCR_WIDTH;
}

Uint32 getBG2Height() {
	return BG_Surface2 ? (Uint32)BG_Surface2->h : (Uint32)SCR_HEIGHT;
}

#endif // DISABLE_BACKGROUND_ART

