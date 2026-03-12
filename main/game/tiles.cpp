// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#include "globals.h"
#include "tiles.h"
#include "levelhandler.h"
#include "blending.h"
#include "errorhandler.h"
#include "engine.h"
#include "convert.h"

SDL_Surface *TILES_Surface;

void initTiles(const char *fname) {
	char fullfname[MAX_FNAME_LENGTH];
	sprintf(fullfname, "%s", configGetPath(fname));
	SDL_Surface* temp = IMG_Load(fullfname);
	if(!temp) {
		DIE(ERROR_IMAGE_READ, fullfname);
	}
	TILES_Surface = convertToBSSurface(temp);
	SDL_FreeSurface(temp);
}

void deInitTiles() {
	SDL_FreeSurface(TILES_Surface);
}

void paintSingleTile(const Uint32 num, const Sint32 x, const Sint32 y) {
	if(num >= (Uint32)(TILES_Surface->h / TILESIZE)) {
		return; // Illegal tile
	}

	if(x < (1 - TILESIZE) || 
			y < (1 - TILESIZE) ||
			x > SCR_WIDTH || 
			y > SCR_HEIGHT) {
		return; // Tile is invisible so we ignore it
	}
	
    static SDL_Rect src;
	static SDL_Rect dest;

    src.x = 0;
    src.y = TILESIZE * num;
    src.w = dest.w = src.h = dest.h = TILESIZE;	
    dest.x = x;
    dest.y = y;
    
	SDL_BlitSurface(TILES_Surface, &src, gScreen, &dest);

	return;
}

Uint32 getNumOfTiles() {
	return (TILES_Surface->h / TILESIZE + 1);
}


void paintLevelTiles(const Uint32 xoffs, const Uint32 yoffs) {
	DISPLAYLIST *tthis = lhandle.dlist;
	Sint32 xpos;
	Sint32 ypos;

	if (SDL_MUSTLOCK(TILES_Surface))
		if (SDL_LockSurface(TILES_Surface) < 0)
			return;	
	
	while(tthis) {
		xpos = tthis->x - xoffs;
		ypos = tthis->y - yoffs;		
		if(tthis->type < 10 && xpos > -TILESIZE && xpos < SCR_WIDTH && ypos > -TILESIZE && ypos < SCR_HEIGHT) {
			paintSingleTile(tthis->type, xpos, ypos);
		}
		tthis = tthis->next;
	}
	
	// Unlock if needed
	if (SDL_MUSTLOCK(TILES_Surface))
		SDL_UnlockSurface(TILES_Surface);
	
	
}
