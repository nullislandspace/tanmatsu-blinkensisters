// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#ifndef PLAYERSPRITE_H
#define PLAYERSPRITE_H

#include "globals.h"

void initPlayerSprite();
void deInitPlayerSprite();

typedef enum _PLAYERSPRITETYPE {
	PLAYERSPRITE_NOMOVE = 0,
	PLAYERSPRITE_LEFT,
	PLAYERSPRITE_LEFTUP,
	PLAYERSPRITE_LEFTDOWN,
	PLAYERSPRITE_RIGHT,
	PLAYERSPRITE_RIGHTUP,
	PLAYERSPRITE_RIGHTDOWN,
	PLAYERSPRITE_UP,
	PLAYERSPRITE_DOWN,
	PLAYERSPRITE_MAX
} PLAYERSPRITETYPE;

void showSprite(const Uint32 x, const Uint32 y);
void updateSpriteGFX(const PLAYERSPRITETYPE mtype);
void resetSpriteGFX();
Uint32 getSpriteGFX(const PLAYERSPRITETYPE mtype);
bool setSpriteGFX(const PLAYERSPRITETYPE mtype, Uint32 gfxid);

extern Uint32 playerspritegfx[];
extern Uint32 playersprite_srcoffset;
extern SDL_Surface* playersprite;

#endif // PLAYERSPRITE_H



