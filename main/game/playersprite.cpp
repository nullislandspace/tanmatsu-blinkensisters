// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#include "globals.h"
#include "playersprite.h"
#include "errorhandler.h"
#include "engine.h"
#include "convert.h"
#include "fgobjects.h"

Uint32 playerspritegfx[PLAYERSPRITE_MAX];
SDL_Surface* playersprite;
PLAYERSPRITETYPE lastmove = PLAYERSPRITE_NOMOVE;
PLAYERSPRITETYPE playerstartmove;
Uint32 spritenum = 0;
Uint32 spritelasttick;
Uint32 spritetick;
Uint32 sprite_srcoffset = 0;
Uint32 playersprite_srcoffset;

void initPlayerSprite() {
	lastmove = PLAYERSPRITE_NOMOVE;
	
	// addFGObjGFX returns PSEUDO_FGOBJECTGFXNUM if loading fails
	playerspritegfx[PLAYERSPRITE_NOMOVE] = addFGObjGFX("sister_movenone.bmp", true);
	playerspritegfx[PLAYERSPRITE_LEFT] = addFGObjGFX("sister_moveleft.bmp", true);
	playerspritegfx[PLAYERSPRITE_LEFTUP] = addFGObjGFX("sister_moveleftup.bmp", true);
	playerspritegfx[PLAYERSPRITE_LEFTDOWN] = addFGObjGFX("sister_moveleftdown.bmp", true);
	playerspritegfx[PLAYERSPRITE_RIGHT] = addFGObjGFX("sister_moveright.bmp", true);
	playerspritegfx[PLAYERSPRITE_RIGHTUP] = addFGObjGFX("sister_moverightup.bmp", true);
	playerspritegfx[PLAYERSPRITE_RIGHTDOWN] = addFGObjGFX("sister_moverightdown.bmp", true);
	playerspritegfx[PLAYERSPRITE_UP] = addFGObjGFX("sister_moveup.bmp", true);
	playerspritegfx[PLAYERSPRITE_DOWN] = addFGObjGFX("sister_movedown.bmp", true);
	
	playersprite = 0;
	if(playerspritegfx[PLAYERSPRITE_LEFT] != PSEUDO_FGOBJECTGFXNUM) {
		// Default to "classic" behavior: Display move_left as first frame
		playersprite = getFGSurface(playerspritegfx[PLAYERSPRITE_LEFT]);
		lastmove = PLAYERSPRITE_LEFT;
	} else {
		// Try all directions for a useable graphic
		for(Uint32 i = 0; i < PLAYERSPRITE_MAX; i++) {
			if(playerspritegfx[i] != PSEUDO_FGOBJECTGFXNUM) {
				playersprite = getFGSurface(playerspritegfx[i]);
				lastmove = (PLAYERSPRITETYPE)i;
			}
		}
	}
	if(playersprite == 0) {
		// No useable grafics for playersprite? Let's just give up for now...
		DIE(ERROR_PLAYERSPRITE, "Couldn't load ANY playersprite graphics!");
	}
	
	// Remember our start sprite for consistency
	playerstartmove = lastmove;
	
	// More "classic" behavior: set default up/down GFX if none was loaded
	if(playerspritegfx[PLAYERSPRITE_LEFTUP] == PSEUDO_FGOBJECTGFXNUM) playerspritegfx[PLAYERSPRITE_LEFTUP] = playerspritegfx[PLAYERSPRITE_LEFT];
	if(playerspritegfx[PLAYERSPRITE_LEFTDOWN] == PSEUDO_FGOBJECTGFXNUM) playerspritegfx[PLAYERSPRITE_LEFTDOWN] = playerspritegfx[PLAYERSPRITE_LEFT];
	if(playerspritegfx[PLAYERSPRITE_RIGHTUP] == PSEUDO_FGOBJECTGFXNUM) playerspritegfx[PLAYERSPRITE_RIGHTUP] = playerspritegfx[PLAYERSPRITE_RIGHT];
	if(playerspritegfx[PLAYERSPRITE_RIGHTDOWN] == PSEUDO_FGOBJECTGFXNUM) playerspritegfx[PLAYERSPRITE_RIGHTDOWN] = playerspritegfx[PLAYERSPRITE_RIGHT];
	
	spritetick = spritelasttick = BS_GetTicks();
	
}

void deInitPlayerSprite() {
	playersprite = 0;
}

Uint32 getSpriteGFX(const PLAYERSPRITETYPE mtype) {
	return playerspritegfx[mtype];
}

bool setSpriteGFX(const PLAYERSPRITETYPE mtype, Uint32 gfxid) {
	playerspritegfx[mtype] = gfxid;
	
	// Check if we need to update visible playersprite
	if(mtype == lastmove) {
		playersprite = getFGSurface(playerspritegfx[mtype]);
		spritenum = 0;
	}
	
	return true;
}

void resetSpriteGFX() {
	lastmove = playerstartmove;
	playersprite = getFGSurface(playerspritegfx[lastmove]);
	spritenum = 0;	
}

void updateSpriteGFX(const PLAYERSPRITETYPE mtype) {
	spritetick = BS_GetTicks();
	if((mtype != lastmove && playerspritegfx[mtype] !=  PSEUDO_FGOBJECTGFXNUM && playerspritegfx[mtype] != playerspritegfx[lastmove])) {
		lastmove = mtype;
		spritenum = 0;
		playersprite = getFGSurface(playerspritegfx[mtype]);
		if(!turboMode) {
			spritelasttick = spritetick + SPRITE_SWITCHWAIT;
		} else {
			spritelasttick = spritetick + SPRITE_SWITCHWAIT / 4;
		}
	} else {
		playersprite = getFGSurface(playerspritegfx[lastmove]); // Need to do this for anims!
		if(spritelasttick < spritetick) {
			if(mtype != PLAYERSPRITE_NOMOVE && playerspritegfx[lastmove] < MAX_FGOBJECTGFX) {
				spritenum++;
				if(spritenum >= (Uint32)(playersprite->h / TILESIZE)) {
					spritenum = 0;
				}
			} else {
				spritenum = 0;
			}
			if(!turboMode) {
				spritelasttick = spritetick + SPRITE_SWITCHWAIT;
			} else {
				spritelasttick = spritetick + SPRITE_SWITCHWAIT / 4;
			}
		}
	}
}

void showSprite(const Uint32 x, const Uint32 y) {
	static SDL_Rect src;
	static SDL_Rect dest;

	playersprite_srcoffset = spritenum * TILESIZE;
	
	if(playersprite == 0) {
		DIE(ERROR_PLAYERSPRITE, "showSprite called with NULL pointer!");
	}
	
	src.x = 0;
    src.y = playersprite_srcoffset;
    src.w = dest.w = src.h = dest.h = TILESIZE;
    dest.x = x;
    dest.y = y;
    
	SDL_BlitSurface(playersprite, &src, gScreen, &dest);	
	
}

