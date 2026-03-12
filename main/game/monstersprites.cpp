// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//



#include "globals.h"
#include "monstersprites.h"
#include "levelhandler.h"
#include "colission.h"
#include "errorhandler.h"
#include "convert.h"
#include "engine.h"
#include <stdlib.h> // Try to fix problem with solaris (see also changes in version 1.6)

SDL_Surface *MONSTERSPRITE_Left[MAX_MONSTER_TYPES];
SDL_Surface *MONSTERSPRITE_Right[MAX_MONSTER_TYPES];
bool monstersLoaded = false;
bool hasEatenPixel;

void loadMonsterSprites() {
	char fullfname[MAX_FNAME_LENGTH];
	SDL_Surface* temp;
	
	for(Uint32 i = 0; i < MAX_MONSTER_TYPES; i++) {
		sprintf(fullfname, "%smonster%d_left.bmp", configGetPath(""), (i+1));
		temp = IMG_Load(fullfname);
		if(!temp) {
			DIE(ERROR_IMAGE_READ, fullfname);
		}
		MONSTERSPRITE_Left[i] = convertToBSSurface(temp);
		SDL_FreeSurface(temp);
		
		sprintf(fullfname, "%smonster%d_right.bmp", configGetPath(""), (i+1));
		temp = IMG_Load(fullfname);
		if(!temp) {
			DIE(ERROR_IMAGE_READ, fullfname);
		}
		MONSTERSPRITE_Right[i] = convertToBSSurface(temp);
		SDL_FreeSurface(temp);
		
	}
	monstersLoaded = true;
}


void unloadMonsterSprites() {
	if(!monstersLoaded) {
		return;
	}
	printf("Freeing monster sprites...\n");
	for(Uint32 i = 0; i < MAX_MONSTER_TYPES; i++) {
		SDL_FreeSurface(MONSTERSPRITE_Left[i]);
		SDL_FreeSurface(MONSTERSPRITE_Right[i]);
	}
	monstersLoaded = false;
}

void addMonster(const Uint32 type, const Uint32 x, const Uint32 y) {
	MONSTERS* tthis = (MONSTERS*)malloc(sizeof(MONSTERS));
	tthis->next = 0;
	tthis->type = type;
	tthis->startx = x;
	tthis->starty = y;
	tthis->spritenum = 0;
	if(!lhandle.monsters) {
		lhandle.monsters = tthis;
	} else {
		MONSTERS *last = lhandle.monsters;
		while(last->next) {
			last = last->next;
		}
		last->next = tthis;
	}
	lhandle.numMonsters++;	
}

void initMonsters() {
	MONSTERS* tthis = lhandle.monsters;
	while(tthis) {
		
		tthis->ai.turnDefault = 150;
		if(tthis->type < 3) {
			tthis->maxspeed = 0.1 * (tthis->type + 1);
			tthis->monstervx = -tthis->maxspeed;
		} else if(tthis->type == 3) {
			tthis->maxspeed = 0.3;
			tthis->monstervx = tthis->maxspeed;
		} else if(tthis->type == 4) {
			tthis->maxspeed = 1.6;
			tthis->monstervx = tthis->maxspeed;
			tthis->ai.turnDefault = 100;
		} else {
			tthis->maxspeed = 0.3;
			tthis->monstervx = tthis->maxspeed;			
		}
		tthis->isAlive = 1;
		tthis->score = (tthis->type + 1) * 100;
		tthis->monsterx = tthis->startx;
		tthis->monstery = tthis->starty;
		tthis->mtype = MOVE_LEFT;
		tthis->newmtype = MOVE_LEFT;
		tthis->spritetick = BS_GetTicks();
		tthis->spritenum = 0;
		tthis->isDying = false;
		tthis->dyeCount = TILESIZE;
		tthis->ai.turnCounter = 0;
		tthis = tthis->next;
	}
}

void freeMonsters() {
	lhandle.numMonsters = 0;
	MONSTERS* tthis;
	MONSTERS* tnext = lhandle.monsters;
	tthis = lhandle.monsters; 
	while(tnext) {
		tnext = tthis->next;
		free(tthis);
		tthis = tnext;
	}
	lhandle.monsters = 0;
}

void blitMonsterSprite(const Sint32 x, const Sint32 y, SDL_Surface* srcSurface, const Uint32 num, const Uint32 maxheight) {
	// Ignore sprites we can't display anyway
	if(maxheight <= 0 || maxheight > TILESIZE) {
		return;
	}
	
    static SDL_Rect src;
	static SDL_Rect dest;

    src.x = 0;
    src.y = TILESIZE * num;
    src.w = dest.w = TILESIZE;	
    src.h = dest.h = maxheight;
    dest.x = x;
    dest.y = y + (TILESIZE - maxheight);
    
	SDL_BlitSurface(srcSurface, &src, gScreen, &dest);
}

void showMonsterSprites(const Uint32 screenXoffs, const Uint32 screenYoffs) {
	MONSTERS* monster = lhandle.monsters;
	SDL_Surface* monstersprite = 0;
	Uint32 spritetick = BS_GetTicks();
	Sint32 scr_x;
	Sint32 scr_y;
	while(monster) {
		if(!monster->isAlive && !monster->isDying) {
			monster = monster->next;
			continue;
		}
		switch (monster->newmtype) {
			case MOVE_LEFT:
				monstersprite = MONSTERSPRITE_Left[monster->type];
				break;
			case MOVE_RIGHT:
				monstersprite = MONSTERSPRITE_Right[monster->type];
				break;
			case JUMP_LEFT:
				break;
			case JUMP_RIGHT:
				break;
			default:
				break;
		}
		if(monster->mtype != monster->newmtype) {
			monster->spritenum = 0;
		} else {
			if(monster->spritetick < spritetick) {
				if(!turboMode) {
					monster->spritetick = spritetick + SPRITE_SWITCHWAIT;
				} else {
					monster->spritetick = spritetick + SPRITE_SWITCHWAIT / 4;
				}
				if(monster->isDying) {
					monster->dyeCount -= MONSTER_DIESPEED;
					if(monster->dyeCount <= 0) {
						monster->isDying = false;
						continue;
					}
				} else {
					if(!monster->mustStop) {
						monster->spritenum++;
					}
					if(monster->spritenum >= (Uint32)(monstersprite->h / TILESIZE)) {
						monster->spritenum = 0;
					}
				}
			}
		}
		
		scr_x = (Uint32)monster->monsterx - screenXoffs; 
		scr_y = (Uint32)monster->monstery - screenYoffs;

		
		if(scr_x > -TILESIZE && scr_x < SCR_WIDTH && scr_y > -TILESIZE && scr_y < SCR_HEIGHT) {
			//printf("sn: %d\n", monster->spritenum);
			blitMonsterSprite(scr_x, scr_y, monstersprite, monster->spritenum, monster->dyeCount);	
		}
		
		monster->mtype = monster->newmtype;
		monster = monster->next;
	}	
}

void monsterPhysics(const Uint32 playerx, const Uint32 playery) {;
	MONSTERS* monster = lhandle.monsters;
	
	bool leftWalkTile, leftStopTile, rightWalkTile, rightStopTile, exactOnTile;
	
	while(monster) {
		monster->seePlayer = false;
		monster->mustStop = false;
		
		
		if(monster->isAlive) {
			// Look ahead to the next floor tile in our direction and change direction if it's missing
			Sint32 alignedx = ((Sint32)(monster->monsterx / TILESIZE)) * TILESIZE;
			Sint32 alignedy = ((Sint32)(monster->monstery / TILESIZE)) * TILESIZE;
			if(monster->ai.turnCounter > 0) {
				monster->ai.turnCounter--;
			}
			
			// Get walking and blocking tiles
			if(abs((Sint32)(alignedx - monster->monsterx)) <= abs((Sint32)monster->monstervx)) {
				// Monster more or less *exactly* on top of tile
				exactOnTile = true;
				leftWalkTile = checkForTile(alignedx - TILESIZE, alignedy + TILESIZE);
				rightWalkTile = checkForTile(alignedx + TILESIZE, alignedy + TILESIZE);
				leftStopTile = checkForTile(alignedx - TILESIZE, alignedy);
				rightStopTile = checkForTile(alignedx + TILESIZE, alignedy);
			} else {
				exactOnTile = false;
				leftWalkTile = checkForTile(alignedx, alignedy + TILESIZE);
				rightWalkTile = checkForTile(alignedx + TILESIZE, alignedy + TILESIZE);
				leftStopTile = checkForTile(alignedx, alignedy);
				rightStopTile = checkForTile(alignedx + TILESIZE, alignedy);
			}
				
				
			if(monster->type == 0) {
				// No special for brown monster
			} else if(monster->type == 1) {
				// Blue monster runs away when it sees the player eating a pixel
				if(  hasEatenPixel && abs((int)(monster->monstery - playery)) < (TILESIZE * 5)
					 && abs((int)(monster->monsterx - playerx)) < (TILESIZE * 15)) {
					monster->seePlayer = true;
					if(playerx < monster->monsterx && monster->monstervx < 0) {
						monster->monstervx = monster->maxspeed * 4.0;
						monster->ai.turnCounter = monster->ai.turnDefault;						
					} else if(playerx > monster->monsterx && monster->monstervx > 0) {
						monster->monstervx = -monster->maxspeed * 4.0;
						monster->ai.turnCounter = monster->ai.turnDefault;
					}
				}
			} else if(monster->type == 2) {
				// No special handling of yellow monster
				
			} else if(monster->type == 3) {
				// Eye-monster can try to switch to player direction when it "sees" it e.g. Player is at about same Y coords as monster
				if(abs((int)(monster->monstery - playery)) < (TILESIZE * 2) && abs((int)(monster->monsterx - playerx)) < (TILESIZE * 10)) {
					monster->seePlayer = true;
					if(playerx < monster->monsterx) {
						monster->monstervx = -monster->maxspeed;
						monster->ai.turnCounter = monster->ai.turnDefault;
					} else {
						monster->monstervx = monster->maxspeed;
						monster->ai.turnCounter = monster->ai.turnDefault;
					}
				}
				
			} else if(monster->type == 4) {
				// Buggy monster patrols a small place, except when it sees the player
				// Player in sight?
				if(abs((int)(monster->monstery - playery)) < (TILESIZE * 3) && abs((int)(monster->monsterx - playerx)) < (TILESIZE * 3)) {
					monster->seePlayer = true;
					if((playerx+1) < monster->monsterx) {
						monster->monstervx = -monster->maxspeed;
						monster->ai.turnCounter = 100;
					} else if((playerx-1) > monster->monsterx) {
						monster->monstervx = monster->maxspeed;
						monster->ai.turnCounter = 100;
					} else {
						monster->mustStop = true;
					}
				} else if(monster->ai.turnCounter == 0) {
					// Need to do a turn
					monster->monstervx = -monster->monstervx;
					monster->monsterx += monster->monstervx;
					monster->ai.turnCounter = 100;
				}
			}
			

			
			// Check crossing of tile-end to the left
			if(monster->monstervx < 0 && (monster->monsterx + monster->monstervx) < alignedx &&
			   !checkForTile((Uint32)alignedx - TILESIZE, (Uint32)alignedy + TILESIZE)) {
				if(monster->type > 2 && monster->seePlayer) {
					monster->mustStop = true;
				} else {
					monster->monstervx = monster->maxspeed;
					monster->ai.turnCounter = monster->ai.turnDefault;
				}
			}
			
			// Check crossing of tile-end to the right
			if(monster->monstervx > 0 && (monster->monsterx + monster->monstervx) > (alignedx + (double)TILESIZE) &&
			   !checkForTile((Uint32)alignedx + TILESIZE*2, (Uint32)alignedy + TILESIZE)) {
				if(monster->type > 2 && monster->seePlayer) {
					monster->mustStop = true;
				} else {
					monster->monstervx = -monster->maxspeed;
					monster->ai.turnCounter = monster->ai.turnDefault;
				}				
			}
			
			// This is an ugly workaround for monsters that move off the allocated plattforms
			if(exactOnTile && (!leftWalkTile || leftStopTile) && (!rightWalkTile || rightStopTile)) {
				monster->mustStop = true;
			} else if(!monster->mustStop && !exactOnTile && monster->monstervx > 0.0 && !rightWalkTile) {
				monster->monstervx = -monster->maxspeed;
			} else if(!monster->mustStop && !exactOnTile && monster->monstervx < 0.0 && !leftWalkTile) {
				monster->monstervx = monster->maxspeed;
			}
			
			if(!monster->mustStop) {
				
				monster->monsterx += monster->monstervx;
				if((monster->monstervx < 0 && checkForTile((Sint32)alignedx, (Sint32)alignedy)) || 
				   (monster->monstervx > 0 && checkForTile((Sint32)alignedx + TILESIZE, (Sint32)alignedy))) {
					if(monster->type != 1) {
						monster->monstervx = -monster->monstervx;
					} else if(monster->monstervx > 0) {
						monster->monstervx = -monster->maxspeed;
					} else {
						monster->monstervx = monster->maxspeed;
					}
					monster->monsterx += monster->monstervx;
					monster->ai.turnCounter = monster->ai.turnDefault;;
				}
			}
			
			if(monster->monstervx > 0) {
				monster->newmtype = MOVE_RIGHT;
			} else {
				monster->newmtype = MOVE_LEFT;
			}
		}
		monster = monster->next;
	}
}
