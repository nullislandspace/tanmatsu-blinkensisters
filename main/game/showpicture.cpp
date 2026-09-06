// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#include "globals.h"
#include "showpicture.h"
#include "errorhandler.h"
#include "engine.h"
#include "drawprimitives.h"
#include "bsscreen.h"

void showPicture(const char *fname, Uint32 ticks) {
	//Uint32 lasttick = BS_GetTicks() + ticks;

	if(enableColor3D) {
		BS_Set3DMode(COLOR3D_NONE);
	}
	
#ifdef DISABLE_BACKGROUND_ART
	(void)fname;
	drawrect(0, 0, SCR_WIDTH, SCR_HEIGHT, 0x000000);
#else
	SDL_Surface* temp2 = BS_IMG_Load_Fullscreen(configGetPath(fname),DIE_ON_FILE_ERROR);
	SDL_BlitSurface(temp2, NULL, gScreen, NULL);
	SDL_FreeSurface(temp2);
#endif

	// Tell SDL to update the whole screen
	BS_Flip(gScreen);
	SDL_Delay(ticks);

}
