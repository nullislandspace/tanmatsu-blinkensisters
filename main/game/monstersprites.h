// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#ifndef MONSTERSPRITES_H
#define MONSTERSPRITES_H

#include "globals.h"

struct MONSTERAI {
	Uint32 turnCounter;
	Uint32 turnDefault;
};

struct MONSTERS {
	Uint32 type;
	Uint32 startx;
	Uint32 starty;
	double monsterx;
	double monstery;
	double monstervx;
	double monstervy;
	double maxspeed;
	bool isAlive;
	bool isDying;
	bool seePlayer;
	bool mustStop;
	Uint32 dyeCount;
	Uint32 score;
	Uint32 spritenum;
	Uint32 spritetick;
	MOVE_TYPE mtype;
	MOVE_TYPE newmtype;
	MONSTERAI ai;
	MONSTERS* next;
};

extern bool hasEatenPixel;

void loadMonsterSprites();
void unloadMonsterSprites();
void addMonster(const Uint32 type, const Uint32 x, const Uint32 y);
void initMonsters();
void freeMonsters();
void blitMonsterSprite(const Sint32 x, const Sint32 y, SDL_Surface* srcSurface, const Uint32 num, const Uint32 maxheight);
void showMonsterSprites(const Uint32 screenXoffs, const Uint32 screenYoffs);
void monsterPhysics(const Uint32 playerx, const Uint32 playery);

#endif // MONSTERSPRITES_H

