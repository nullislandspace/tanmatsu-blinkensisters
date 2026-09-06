// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#include "globals.h"
#include "showloading.h"
#include "errorhandler.h"
#include "drawprimitives.h"
#include "bsscreen.h"

SDL_Surface *LOADING_Surface;

void initShowLoading(const char *fname) {
#ifndef DISABLE_BACKGROUND_ART
	LOADING_Surface = BS_IMG_Load_Fullscreen(configGetPath(fname), false);
#else
	(void)fname;
#endif
}


void showLoading() {
	if(enableColor3D) {
		BS_Set3DMode(COLOR3D_NONE);
	}

#ifdef DISABLE_BACKGROUND_ART
	drawrect(0, 0, SCR_WIDTH, SCR_HEIGHT, 0x000000);
#else
	SDL_BlitSurface(LOADING_Surface, NULL, gScreen, NULL);
#endif

	// Tell SDL to update the whole screen
	BS_Flip(gScreen);

}

void deInitShowLoading() {
#ifndef DISABLE_BACKGROUND_ART
	SDL_FreeSurface(LOADING_Surface);
#endif
}
