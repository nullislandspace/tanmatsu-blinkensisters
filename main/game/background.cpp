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

void initBackground(const char *fname) {
	SDL_Surface* bgtemp = BS_IMG_Load_DisplayFormat(configGetPath(fname),DIE_ON_FILE_ERROR);

	/* Create a 32-bit surface with the bytes of each pixel in R,G,B,A order,
		as expected by OpenGL for textures */
	Uint32 rmask, gmask, bmask, amask;

	/* SDL interprets each pixel as a 32-bit number, so our masks must depend
		on the endianness (byte order) of the machine */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
	amask = 0x000000ff;
#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0xff000000;
#endif

	SDL_Surface* temp = SDL_CreateRGBSurface(SDL_SWSURFACE, bgtemp->w + SCR_WIDTH, bgtemp->h + SCR_HEIGHT, 32, rmask, gmask, bmask, amask);
	BG_Surface = SDL_DisplayFormat(temp);
	SDL_FreeSurface(temp);

	// Lock Surfaces if needed
	if (SDL_MUSTLOCK(BG_Surface))
		if (SDL_LockSurface(BG_Surface) < 0)
			return;
	if (SDL_MUSTLOCK(bgtemp))
		if (SDL_LockSurface(bgtemp) < 0)
			return;

	Uint32 pitch_tmp = bgtemp->pitch / 4;
	Uint32 pitch_bg = BG_Surface->pitch / 4;
	Uint32 i, j;

	// Copy the background into the bigger surface
	for(i=0; i < (Uint32)bgtemp->w; i++) {
		for(j=0; j < (Uint32)bgtemp->h; j++) {
			((Uint32 *)BG_Surface->pixels)[j * pitch_bg + i] =
			((Uint32 *)bgtemp->pixels)[j * pitch_tmp + i];
		}
	}

	// Replicate the first SCR_WIDTH pixels to the right side of the surface for BlitSurfac
	Sint32 ii = 0;
	for(i=0; i < (unsigned int)SCR_WIDTH; i++) {
		ii++;
		if(ii == bgtemp->w) {
			ii = 0;
		}
		for(j=0; j < (Uint32)bgtemp->h; j++) {
			((Uint32 *)BG_Surface->pixels)[j * pitch_bg + i + bgtemp->w] =
				((Uint32 *)bgtemp->pixels)[j * pitch_tmp + ii];
		}
	}

	// Replicate the first SCR_HEIGHT pixels to the bottom for BlitSurface
	Sint32 jj = 0;
	for(j = bgtemp->h; j < (Uint32)BG_Surface->h; j++) {
		for(i = 0; i < (Uint32)BG_Surface->w; i++) {
			((Uint32 *)BG_Surface->pixels)[j * pitch_bg + i] =
			((Uint32 *)BG_Surface->pixels)[jj * pitch_bg + i];
		}
		jj++;
		if(jj == bgtemp->h) {
			jj = 0;
		}
	}



	// Unlock Surfaces if needed
	if (SDL_MUSTLOCK(bgtemp))
		SDL_UnlockSurface(bgtemp);
	if (SDL_MUSTLOCK(BG_Surface))
		SDL_UnlockSurface(BG_Surface);


	SDL_FreeSurface(bgtemp);

	// The background is built once and then only read, which is exactly the
	// case the PPA can take over: mark it so the per-frame full-screen blit
	// runs on the hardware instead of a per-pixel CPU loop.
	BS_SurfaceFinalize(BG_Surface);
}

void deInitBackground() {
	SDL_FreeSurface(BG_Surface);
}

void drawBackground(Sint32 xoffs, Sint32 yoffs) {

	// Background
	xoffs = xoffs % (BG_Surface->w - SCR_WIDTH);
	yoffs = yoffs % (BG_Surface->h - SCR_HEIGHT);

	SDL_Rect src, dest;
	dest.x = dest.y = 0;
	src.w = dest.w = SCR_WIDTH;
	src.h = dest.h = SCR_HEIGHT;
	src.x = xoffs;
	src.y = yoffs;

	SDL_BlitSurface(BG_Surface, &src, gScreen, &dest);

}

Uint32 getBGWidth() {
	return BG_Surface->w;
}

Uint32 getBGHeight() {
	return BG_Surface->h;
}

#endif // DISABLE_BACKGROUND_ART

