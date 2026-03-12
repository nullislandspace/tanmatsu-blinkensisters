// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#include "globals.h"
#include "colission.h"
#include "levelhandler.h"
#include "engine.h"
#include "monstersprites.h"
#include "gameengine.h"
#include "sound.h"

void handlePlayerTilesColission() {
	if(lhandle.hasMagicTiles) {
		handlePlayerTilesColission_magicTiles();
	}
	handlePlayerTilesColission_down_limited();
	handlePlayerTilesColission_up_limited();
	handlePlayerTilesColission_left();
	handlePlayerTilesColission_right();
	handlePlayerTilesColission_down();
	handlePlayerTilesColission_up();
}

void handlePlayerTilesColission_magicTiles() {
	
	DISPLAYLIST *tthis = lhandle.dlist;
	while(tthis) {
		if(tthis->type == lhandle.magicTileSource) {
			
			Sint32 distx = abs((Sint32)spritex - (Sint32)tthis->x);
			Sint32 disty = abs((Sint32)spritey - (Sint32)tthis->y);
			if(distx <= TILESIZE && disty <= TILESIZE) {
				tthis->type = lhandle.magicTileDestination;
			}
		}
		tthis = tthis->next;
	}
	
}

void handlePlayerTilesColission_down_limited() {
	Sint32 alignedy = ((Sint32)(spritey / TILESIZE)) * TILESIZE;
	if(alignedy == (Sint32)spritey) {
		return;
	}
	
	DISPLAYLIST *tthis = lhandle.dlist;
	bool foundTile = false;
	while(tthis) {
		Sint32 dist = abs((Sint32)spritex - tthis->x);
		if(tthis->y == (alignedy + TILESIZE) && dist < (TILESIZE - SPRITE_MAXSPEED) && tthis->type < 10) {
			foundTile = true;
		}
		tthis = tthis->next;
	}
	
	if(foundTile) {
		spritevy = 0;
		spritey = alignedy;
		isJumping = false;		
	}	
}

void handlePlayerTilesColission_up_limited() {
	Sint32 alignedy = ((Sint32)(spritey / TILESIZE)) * TILESIZE;
	if(alignedy == (Sint32)spritey) {
		return;
	}
	alignedy += TILESIZE;
	
	DISPLAYLIST *tthis = lhandle.dlist;
	bool foundTile = false;
	while(tthis) {
		Sint32 dist = abs((Sint32)spritex - (Sint32)tthis->x);
		if(tthis->y == (alignedy - TILESIZE) && dist < ((TILESIZE / 2) + 1) && tthis->type < 10) {
			foundTile = true;
		}
		tthis = tthis->next;
	}
	
	if(foundTile) {
		spritevy = 0;
		spritey = alignedy;
		isJumping = false;		
	}	
}

void handlePlayerTilesColission_down() {
	Sint32 alignedx;
	Sint32 alignedy;

	// Handle DOWN collission
	alignedx = ((Sint32)(spritex / TILESIZE)) * TILESIZE;
	alignedy = ((Sint32)(spritey / TILESIZE)) * TILESIZE;
	if((Sint32)spritey != alignedy) {
		if((Sint32)spritex == alignedx) {
			if(checkForTile(alignedx, alignedy + TILESIZE)) {
				spritevy = 0;
				spritey = alignedy;
				isJumping = false;
			}
		} else {
			if(checkForTile(alignedx, alignedy + TILESIZE) ||
					checkForTile(alignedx + TILESIZE, alignedy + TILESIZE)) {
				spritevy = 0;
				spritey = alignedy;
				isJumping = false;
			}
		}
	}
}

void handlePlayerTilesColission_up() {
	Sint32 alignedx;
	Sint32 alignedy;
	// Handle UP collission
	alignedx = ((Sint32)(spritex / TILESIZE)) * TILESIZE;
	alignedy = ((Sint32)(spritey / TILESIZE)) * TILESIZE;
	if((Sint32)spritey != alignedy) {
		alignedy = ((Sint32)(spritey / TILESIZE) + 1) * TILESIZE;	
		if((Sint32)spritex == alignedx) {
			if(checkForTile(alignedx, alignedy - TILESIZE)) {
				spritevy = 0;
				spritey = alignedy;
			}
		} else {
			if(checkForTile(alignedx, alignedy - TILESIZE) ||
					checkForTile(alignedx + TILESIZE, alignedy - TILESIZE)) {
				spritevy = 0;
				spritey = alignedy;
			}
		}
	}
}

void handlePlayerTilesColission_right() {
	Sint32 alignedx;
	Sint32 alignedy;
	// Handle RIGHT collission
	alignedx = ((Sint32)(spritex / TILESIZE)) * TILESIZE;
	alignedy = ((Sint32)(spritey / TILESIZE)) * TILESIZE;
	if((Sint32)spritex != alignedx) {
		if((Sint32)spritey == alignedy) {
			if(checkForTile(alignedx + TILESIZE, alignedy)) {
				spritevx = 0;
				spritex = alignedx;
			}
		} else {
			if(checkForTile(alignedx + TILESIZE, alignedy) ||
					checkForTile(alignedx + TILESIZE, alignedy + TILESIZE)) {
				spritevx = 0;
				spritex = alignedx;
			}
		}
	}
}

void handlePlayerTilesColission_left() {
	Sint32 alignedx;
	Sint32 alignedy;
	
	// Handle LEFT collission
	alignedx = ((Sint32)(spritex / TILESIZE)) * TILESIZE;
	alignedy = ((Sint32)(spritey / TILESIZE)) * TILESIZE;
	if((Sint32)spritex != alignedx) {
		alignedx = ((Sint32)(spritex / TILESIZE) + 1) * TILESIZE;	
		if((Sint32)spritey == alignedy) {
			if(checkForTile(alignedx - TILESIZE, alignedy)) {
				spritevx = 0;
				spritex = alignedx;
			}
		} else {
			if(checkForTile(alignedx - TILESIZE, alignedy) ||
			   checkForTile(alignedx - TILESIZE, alignedy + TILESIZE)) {
				spritevx = 0;
				spritex = alignedx;
			}
		}
	}
	
}

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// FG Object colission is in fgobjects.cpp
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

Uint32 getPlayerMonsterColission() {
	Uint32 cols = COL_NONE;
	
	Sint32 playerx = (Sint32)spritex;
	Sint32 playery = (Sint32)spritey;
	
	Sint32 playerxmax = playerx + TILESIZE;
	Sint32 playerymax = playery + TILESIZE- 1;
	Sint32 playerxmid = playerx + TILESIZE / 2;
	Sint32 playerymid = playery + TILESIZE / 2;
	
	MONSTERS *tthis = lhandle.monsters;
	while(tthis) {
		if(tthis->isAlive) {
			// COL_DOWN
			if(playerxmax > tthis->monsterx && playerx < (tthis->monsterx + TILESIZE) && playerymax > tthis->monstery && playerymid < tthis->monstery && spritevy > 0) {
				tthis->isAlive = false;
				tthis->isDying = true;
				gamedata.player->score += tthis->score;
				cols =  cols | COL_DOWN;
				soundPlayFX(FX_KILL_MONSTER);
			}
			
			// COL_UP
			else if(playerxmax > tthis->monsterx && playerx < (tthis->monsterx + TILESIZE) && playery < (tthis->monstery + TILESIZE) && playerymid > tthis->monstery && spritevy < 0) {
				cols = cols | COL_UP;
			}
			
			// COL_RIGHT
			else if(playerymax > tthis->monstery && playery < (tthis->monstery + TILESIZE) && playerxmax >= tthis->monsterx && playerxmid < tthis->monsterx) {
				cols = cols | COL_RIGHT;
			}
			// COL_LEFT
			else if(playerymax > tthis->monstery && playery < (tthis->monstery + TILESIZE) && playerx <= (tthis->monsterx + TILESIZE) && playerxmid > tthis->monsterx) {
				cols = cols | COL_LEFT;
			}
		}
		tthis = tthis->next;
	}
	
	return (COL_TYPE)cols;
}


bool handlePixelCollission() {
	Sint32 playerx = (Sint32)spritex;
	Sint32 playery = (Sint32)spritey;
	
	DISPLAYLIST *tthis = lhandle.pixels;
	Sint32 playerxmax = playerx + TILESIZE;
	Sint32 playerymax = playery + TILESIZE;
	hasEatenPixel = false;
	bool collissionWithExit = false;
	int numberofexit = 0 ;
	
	while(tthis) {
		if(tthis->type) {
			if(playerxmax >= tthis->x && playerx < (tthis->x + TILESIZE) && playerymax >= tthis->y && playery < (tthis->y + TILESIZE)) {
				if(tthis->type == PIXELTYPE_PIXEL) {
					foundPixels++;
					gamedata.player->score += PIXEL_SCORE;
					soundPlayFX(FX_COLLECT_PIXEL);
				} else if(tthis->type == PIXELTYPE_POWER) {
					foundPixels++;
					gamedata.player->score += PIXEL_SCORE * 10;
					soundPlayFX(FX_COLLECT_PIXEL);
					hasEatenPixel = true;
				} else if(tthis->type == PIXELTYPE_LIVE) {
					gamedata.player->lives++;
					soundPlayFX(FX_LEVEL_FINISHED);
				} else if(tthis->type == PIXELTYPE_POINTS) {
					gamedata.player->score += EXTRAPOINTS_VALUE;
					soundPlayFX(FX_LEVEL_FINISHED);

				} else if(tthis->type == PIXELTYPE_TIMEBONUS) {
					lhandle.remaintime += lhandle.timebonuspixel ;
					if (lhandle.remaintime < 0) lhandle.remaintime = 0 ; // CAN this happen?
					//if (lhandle.remaintime > lhandle.leveltime) lhandle.remaintime = lhandle.leveltime ; /* time cannot be greater than total level time - WHY NOT? */
					soundPlayFX(FX_LEVEL_FINISHED);
				
				} else if(tthis->type == PIXELTYPE_RESPAWN) {
					if(tthis->x != lhandle.respawn_x || tthis->y != lhandle.respawn_y) {
						lhandle.respawn_x = tthis->x;
						lhandle.respawn_y = tthis->y;
						soundPlayFX(FX_RESPAWNPOINT);
					}
				} else if(tthis->type == PIXELTYPE_EXIT) {
					collissionWithExit = true;
					lhandle.currentexit = numberofexit;
				}
				
				if(tthis->type != PIXELTYPE_RESPAWN && tthis->type != PIXELTYPE_EXIT) {
					tthis->type = PIXELTYPE_NONE;
				}
			}
		}
		if(tthis->type == PIXELTYPE_EXIT) numberofexit++ ;
		tthis = tthis->next;
	}
	return collissionWithExit;
}
