// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-08 Rene Schickbauer, Wolfgang Dautermann
//
// See LICENSE for licensing information
//


#include "globals.h"
#include "engine.h"
#include "levelhandler.h"
#include "background.h"
#include "background2.h"
#include "drawprimitives.h"
#include "debug.h"
#include "tiles.h"
#include "playersprite.h"
#include "colission.h"
#include "showloading.h"
#include "gameengine.h"
#include "showpicture.h"
#include "joystick.h"
#ifdef ALLOW_ACTIONCAPTURE
#include "actioncapture.h"
#endif // ALLOW_ACTIONCAPTURE
#include "sound.h"
#include "fgobjects.h"
#include "bl_lua.h"
#include "fonthandler.h"
#include "fginlay.h"
#include "bmfconvert.h"
#include "errorhandler.h"
#include "triggers.h"
#include "outputfilter.h"
#include "blending.h"
#include "bl_lua_objbindings.h"
#ifndef DISABLE_NETWORK
#include "blpstream.h"
#include "mcufstream.h"
#endif // DISABLE_NETWORK
#include "bsgui.h"
#include "fonthandler.h"
#include "bsscreen.h"

#include <math.h>
#include <string.h>

Uint32 fgXoffs = 0;
Uint32 fgYoffs = 0;
double fgXoffsOld = 0;
double fgYoffsOld = 0;
double fgXoffsTmp = 0;
double fgYoffsTmp = 0;

Uint32 spriterelx;
Uint32 spriterely;
PLAYERSPRITETYPE mtype = PLAYERSPRITE_NOMOVE;
double spritex, spritey, spritevx, spritevy;
Uint32 mcoltype;
bool isJumping;
bool gameRunning;
double blinkdir = BLINKSPEED;
double blinkbright = 100;
Uint32 foundPixels = 0;
char errorMessage[100];
Sint32 leftright = 0;
Sint32 updown = 0;
bool leftpressed = false;
bool rightpressed = false;
bool uppressed = false;
bool downpressed = false;
bool lockedBG;
bool hasGravity;
bool freeMovement;
bool touchFGObject;
bool firstLoop;
bool canPaintSpecialTiles;
bool allowedToExit;
bool displayPlayerCoords;
bool isEditMode;
bool isGodMode;
bool attracktModeRunning;
bool killPlayer;
bool turboMode;
bool turboKeyPressed;
bool isPauseMode = false;
bool hasScriptPhysics;
bool enableQuickDraw = false;
Uint32 BS_MyTick = 0;
Uint32 BS_MyLastTick = 0;

FILE* poscapfh = 0;
#ifdef ALLOW_POSITIONCAPTURE
Uint32 poscapcount;
#endif // ALLOW_POSITIONCAPTURE
bool isAttrackMode;
SDL_Color PLAYMECOLOR_ACTIVE	= { 0xff, 0xff, 0xff, 0 };
SDL_Color TEXTINFO_COLOR	= { 0xe0, 0xe0, 0xe0, 0 };
SDL_Color TEXTINFO_TIMEALERT	= { 0xff, 0x00, 0x00, 0 };
void initEngine(bool initForAttractMode)
{
	lockedBG = false;
	hasGravity = true;
	freeMovement = false;
	allowedToExit = false;
	canPaintSpecialTiles = true;
	leftpressed = false;
	rightpressed = false;
	uppressed = false;
	downpressed = false;
	turboMode = false;
	turboKeyPressed = false;
	hasScriptPhysics = false;

	loadMonsterSprites();
	initLevel(gamedata.player->level);
	initMonsters();
#ifndef DISABLE_NETWORK
	if (blpport != 0) {
		initBLPFrames();
	} ;
	if (mcufport != 0) {
		initMCUFFrames();
	} ;
#endif // DISABLE_NETWORK

	spritex = lhandle.respawn_x;
	spritey = lhandle.respawn_y;
	spritevx = 0;
	spritevy = 0;
	leftright = 0;
	updown = 0;
	isJumping = false;
	gameRunning = true;
	foundPixels = 0;
	firstLoop = true;
	hasEatenPixel = false;
	resetFGInlay();
	isAttrackMode = false;
	displayPlayerCoords = false;
	isEditMode = false;
	isGodMode = false;
	killPlayer = false;
	poscapfh = 0;

#ifdef ALLOW_POSITIONCAPTURE
	if(!initForAttractMode) {
		char poscapname[100];
		sprintf(poscapname, "poscap_level_%d.bin", gamedata.player->level);
		poscapfh = fopen(configGetPath(poscapname), "wb");
		if(!poscapfh) {
			DIE(ERROR_FILE_WRITE, configGetPath(poscapname));
		}
		poscapcount = 0;
	}
#endif // ALLOW_POSITIONCAPTURE

}

void deInitEngine ()
{
#ifndef DISABLE_NETWORK
	if (blpport != 0) {
		deInitBLPFrames();
	} ;
	if (mcufport != 0) {
		deInitMCUFFrames();
	} ;
#endif // DISABLE_NETWORK
	if(poscapfh != 0) {
		fclose(poscapfh);
		poscapfh = 0;
	}
	deInitLevel();
	unloadMonsterSprites();

	if(enableColor3D) {
		BS_Set3DMode(COLOR3D_NONE);
	}
}


void engineFullPhysics() {
	// Full Physics-Engine
	if(!turboMode) {
		gLastTick += 1000 / PHYSICSFPS;
	} else {
		gLastTick += 1000 / (PHYSICSFPS*4);
	}
	
	if(lhandle.leveltime > 0) {
		if (!isEditMode) {
			if(!turboMode) {
				lhandle.remaintime -= 0.01;
			} else {
				lhandle.remaintime -= 0.04;
			}
		}
		if(lhandle.remaintime <= 0) {
			displaymessage(displaymessage_INFO, "Level timeout - play faster!!!", 800);
			spritevx = 0;
			spritevy = 0;
			spritex = lhandle.respawn_x;
			spritey = lhandle.respawn_y;
			gamedata.player->lives--;
			leftpressed = false;
			rightpressed = false;
			uppressed = false;
			downpressed = false;
			firstLoop = true;
			hasEatenPixel = false;
			hasGravity = true;
			isGodMode = false;
			killPlayer = false;
			turboMode = false;
			turboKeyPressed = false;
			lhandle.remaintime = (double)lhandle.leveltime;
			// it costs some score if kate looses a life
			gamedata.player->score -= lhandle.timemalusifkatedies;
			if (gamedata.player->score < 0) gamedata.player->score = 0 ; /* no negative score? why not? :-) */
			// Reset sprite display to default starting position
			resetSpriteGFX();
			
			
			// OO-LUA LUA Livelost Callback
			if(lhandle.luaCB[CB_PLAYER_LIVELOST].cbName[0] != 0) {
				blLuaCall(lhandle.blOOLuaState, lhandle.luaCB[CB_PLAYER_LIVELOST].cbName, "");
			}
			soundPlayFX(FX_KILL_PLAYER);
			showPicture("livelost.jpg", 2000);
#ifdef ALLOW_ACTIONCAPTURE
			stopActionCapture();
#endif // ALLOW_ACTIONCAPTURE
			gLastTick = BS_GetTicks();
		}
	}

	// Blinking of pixels
	blinkbright += blinkdir;
	if(blinkbright < 10 || blinkbright > 250) {
		blinkdir = 0 - blinkdir;
	}
	advanceFGObjAnim();
	
	// Inlay message
	calcFGInlay();
	
	// Monsters
	monsterPhysics((Uint32)spritex, (Uint32)spritey);
	if(lhandle.hasScript) {
		blLuaCall(lhandle.blLuaState, "scriptPhysics", "");
	}
	
	// OO-LUA LUA Physics Callback
	if(lhandle.luaCB[CB_ENGINE_PHYSICS].cbName[0] != 0) {
		blLuaCall(lhandle.blOOLuaState, lhandle.luaCB[CB_ENGINE_PHYSICS].cbName, "");
	}
	
	if(lhandle.hasScript || lhandle.hasOOScript) {
		handleTriggers();
	}
	
	if(lhandle.hasOOScript) {
		handlePlayerFGObjCallbacks();
	}
	
	// Left/Right movement with smooth acceleration/braking
	
	if(leftright > 0) {
		spritevx += SPRITE_ACCEL;
	} else if(leftright < 0) {
		spritevx -= SPRITE_ACCEL;
	} else {
		if(spritevx > SPRITE_ACCEL) {
			spritevx -= SPRITE_ACCEL;
		} else if(spritevx < -SPRITE_ACCEL) {
			spritevx += SPRITE_ACCEL;
		} else {
			spritevx = 0;
		}
	}
	
	if(freeMovement) {
		if(updown > 0) {
			spritevy += SPRITE_ACCEL;
		} else if(updown < 0) {
			spritevy -= SPRITE_ACCEL;
		} else {
			if(spritevy > SPRITE_ACCEL) {
				spritevy -= SPRITE_ACCEL;
			} else if(spritevy < -SPRITE_ACCEL) {
				spritevy += SPRITE_ACCEL;
			} else {
				spritevy = 0;
			}
		}
	}
	
	
	// Gravity
	if(hasGravity && !freeMovement) {
		spritevy += SPRITE_GRAVITY;
	}
	
	// Limit speed
	if(spritevx > SPRITE_MAXSPEED) {
		spritevx = SPRITE_MAXSPEED;
	} else if(spritevx < -SPRITE_MAXSPEED) {
		spritevx = -SPRITE_MAXSPEED;
	}
	spritex += spritevx;
	
	if(spritevy > SPRITE_MAXSPEED) {
		spritevy = SPRITE_MAXSPEED;
	} else if(spritevy < -SPRITE_MAXSPEED) {
		spritevy = -SPRITE_MAXSPEED;
	}
	
	spritey += spritevy;
	
	// Limit movement to stay within game-area
	if(spritex > (lhandle.width - 32)) {
		spritevx = 0;
		spritex = lhandle.width - 32;
	} else if(spritex < 0) {
		spritevx = 0;
		spritex = 0;
	}
	
	
	if(spritey > (lhandle.height - 32)) {
		spritevy = 0;
		spritey = lhandle.height - 32;
	} else if(spritey < 0) {
		spritevy = 0;
		spritey = 0;
	}
	
	
	// Decide on playersprite movement/gfx selection
	mtype = PLAYERSPRITE_NOMOVE;
	
	if(leftright > 0) {
		if(updown > 0) {
			mtype = PLAYERSPRITE_RIGHTDOWN;
		} else if(updown < 0) {
			mtype = PLAYERSPRITE_RIGHTUP;
		} else {
			mtype = PLAYERSPRITE_RIGHT;
		}
	} else if(leftright < 0) {
		if(updown > 0) {
			mtype = PLAYERSPRITE_LEFTDOWN;
		} else if(updown < 0) {
			mtype = PLAYERSPRITE_LEFTUP;
		} else {
			mtype = PLAYERSPRITE_LEFT;
		}
	} else {
		if(updown > 0) {
			mtype = PLAYERSPRITE_DOWN;
		} else if(updown < 0) {
			mtype = PLAYERSPRITE_UP;
		} else {
			mtype = PLAYERSPRITE_NOMOVE;
		}
	}
	
	if(isAttrackMode) {
		spritex = (double)bmfReadInt(poscapfh);
		spritey = (double)bmfReadInt(poscapfh);
		mtype = (PLAYERSPRITETYPE)bmfReadInt(poscapfh);
		if(spritex == 0 && spritey == 0) {
			fclose(poscapfh);
			poscapfh = 0;
			gamedata.player->lives = 0;
			gameRunning = false;
			return;
		}
	}
	
#ifdef ALLOW_POSITIONCAPTURE
	if(!isAttrackMode && poscapfh) {
		bmfWriteInt(poscapfh, (Uint32)spritex);
		bmfWriteInt(poscapfh, (Uint32)spritey);
		bmfWriteInt(poscapfh, (Uint32)mtype);
		poscapcount++;
		if(poscapcount == 1700) {
			bmfWriteInt(poscapfh, (Uint32)0);
			bmfWriteInt(poscapfh, (Uint32)0);
			bmfWriteInt(poscapfh, (Uint32)0);
			fclose(poscapfh);
			poscapfh = 0;
		}
	}
#endif // ALLOW_POSITIONCAPTURE
	
	updateSpriteGFX(mtype);
	
	handlePlayerTilesColission();
	handlePlayerFGColission();
	
	mcoltype = getPlayerMonsterColission();
	if(mcoltype & COL_UP || mcoltype & COL_LEFT || mcoltype & COL_RIGHT || getPlayerFGKillColission() || killPlayer) {
		if (!isGodMode) {
			spritevx = 0;
			spritevy = 0;
			spritex = lhandle.respawn_x;
			spritey = lhandle.respawn_y;
			gamedata.player->lives--;
			leftpressed = false;
			rightpressed = false;
			uppressed = false;
			downpressed = false;
			firstLoop = true;
			hasEatenPixel = false;
			hasGravity = true;
			freeMovement = isEditMode;
			isGodMode = false;
			killPlayer = false;
			turboMode = false;
			turboKeyPressed = false;
			lhandle.remaintime = (double)lhandle.leveltime;
			//it costs some score if kate looses a life
			gamedata.player->score -= lhandle.timemalusifkatedies;
			if (gamedata.player->score < 0) gamedata.player->score = 0 ; /* no negative score? why not? :-) */
			// Reset sprite display to default starting position
			resetSpriteGFX();
			
			// OO-LUA LUA Livelost Callback
			if(lhandle.luaCB[CB_PLAYER_LIVELOST].cbName[0] != 0) {
				blLuaCall(lhandle.blOOLuaState, lhandle.luaCB[CB_PLAYER_LIVELOST].cbName, "");
			}
			soundPlayFX(FX_KILL_PLAYER);
			showPicture("livelost.jpg", 2000);
#ifdef ALLOW_ACTIONCAPTURE
			stopActionCapture();
#endif // ALLOW_ACTIONCAPTURE
			gLastTick = BS_GetTicks();
			return;
		}
	}
	
	touchFGObject = false;
	
	// Check if the player has enough pixels to exit
	if(foundPixels == lhandle.numPixels) {
		allowedToExit = true;
	}
	
	bool colWithExit = handlePixelCollission();
	if(colWithExit) {
		if(allowedToExit) {
			printf("LEVEL FINISHED\n");
			gameRunning = false;
		} else {
			sprintf(errorMessage, "Level not finished!");
		}
	}
	
	
	// Calculate relative positions of sprite, tiles and background
	spriterelx = SCR_WIDTH / 2;
	spriterely = SCR_HEIGHT / 2;
	
	if(spritex < spriterelx) {
		fgXoffs = 0;
		spriterelx = (Uint32)spritex;
	} else if(spritex > (lhandle.width - spriterelx)) {
		spriterelx = SCR_WIDTH - (lhandle.width - (Uint32)spritex);
		fgXoffs = lhandle.width - SCR_WIDTH;
	} else {
		fgXoffs = (Uint32)spritex - spriterelx;
	}
	
	if(spritey < spriterely) {
		fgYoffs = 0;
		spriterely = (Uint32)spritey;
	} else if(spritey > (lhandle.height - spriterely)) {
		spriterely = SCR_HEIGHT - (lhandle.height - (Uint32)spritey);
		fgYoffs = lhandle.height - SCR_HEIGHT;
	} else {
		fgYoffs = (Uint32)spritey - spriterely;
	}
	
	
#ifdef ALLOW_SMOOTHPANNING
	// --- Start of smooth panning feature
	if(firstLoop) {
		firstLoop = false;
		fgXoffsOld = (double)fgXoffs;
		fgYoffsOld = (double)fgYoffs;
	}
	
	fgXoffsTmp = fgXoffsOld - (fgXoffsOld - (double)fgXoffs) / SMOOTHPANNING_FACTOR;
	if(fgXoffsTmp < 0.0) {
		fgXoffsTmp = 0.0;
	}
	
	fgYoffsTmp = fgYoffsOld - (fgYoffsOld - (double)fgYoffs) / SMOOTHPANNING_FACTOR;
	if(fgYoffsTmp < 0.0) {
		fgYoffsTmp = 0.0;
	}
	
	//spriterelx = spriterelx + (Uint32)(fgXoffsTmp) - fgXoffs;
	//spriterely = spriterely + ((Uint32)(fgYoffsTmp) - fgYoffs);
	
	fgXoffsOld = fgXoffsTmp;
	fgYoffsOld = fgYoffsTmp;
	
	Sint32 tmpSX = (Sint32)spritex - (Sint32)fgXoffsTmp;
	if(tmpSX < 0) {
		tmpSX = 0;
	} else if(tmpSX > SCR_WIDTH - TILESIZE) {
		tmpSX = SCR_WIDTH - TILESIZE;
	}
	spriterelx = tmpSX;
	
	Sint32 tmpSY = (Sint32)spritey - (Sint32)fgYoffsTmp;
	if(tmpSY < 0) {
		tmpSY = 0;
	} else if(tmpSY > SCR_HEIGHT - TILESIZE) {
		tmpSY = SCR_HEIGHT - TILESIZE;
	}
	spriterely = tmpSY;
	
	//printf("X: %d Y: %d\n", spriterelx, spriterely);
	
	
	fgXoffs = (Uint32)fgXoffsTmp;
	fgYoffs = (Uint32)fgYoffsTmp;
	
	// --- End of smooth panning feature
#endif // ALLOW_SMOOTHPANNING
	

}

void engineMinimalPhysics() {
	// Minimal Physics-Engine
	if(!turboMode) {
		gLastTick += 1000 / PHYSICSFPS;
	} else {
		gLastTick += 1000 / (PHYSICSFPS*4);
	}
		
	advanceFGObjAnim();
	
	// Inlay message
	calcFGInlay();
	
	if(lhandle.hasScript) {
		blLuaCall(lhandle.blLuaState, "scriptPhysics", "");
	}
	
	// OO-LUA LUA Physics Callback
	if(lhandle.luaCB[CB_ENGINE_PHYSICS].cbName[0] != 0) {
		blLuaCall(lhandle.blOOLuaState, lhandle.luaCB[CB_ENGINE_PHYSICS].cbName, "");
	}
	
	if(lhandle.hasScript || lhandle.hasOOScript) {
		handleTriggers();
	}
	
	if(lhandle.hasOOScript) {
		handlePlayerFGObjCallbacks();
	}
	
	
	
	// Decide on playersprite movement/gfx selection
/*
	mtype = PLAYERSPRITE_NOMOVE;
	
	if(leftright > 0) {
		if(updown > 0) {
			mtype = PLAYERSPRITE_RIGHTDOWN;
		} else if(updown < 0) {
			mtype = PLAYERSPRITE_RIGHTUP;
		} else {
			mtype = PLAYERSPRITE_RIGHT;
		}
	} else if(leftright < 0) {
		if(updown > 0) {
			mtype = PLAYERSPRITE_LEFTDOWN;
		} else if(updown < 0) {
			mtype = PLAYERSPRITE_LEFTUP;
		} else {
			mtype = PLAYERSPRITE_LEFT;
		}
	} else {
		if(updown > 0) {
			mtype = PLAYERSPRITE_DOWN;
		} else if(updown < 0) {
			mtype = PLAYERSPRITE_UP;
		} else {
			mtype = PLAYERSPRITE_NOMOVE;
		}
	}
*/	
	
	
	
	
}

void renderEngine() {

	errorMessage[0] = 0;

	// Ask SDL for the time in milliseconds
	Uint32 tick = BS_GetTicks();

	// Delay a bit if we're too fast
	if(!isPauseMode) {
		while (tick <= gLastTick) {
			SDL_Delay(1);
			tick = BS_GetTicks();
		}
	}

	while (gameRunning && gLastTick < tick) {
		if(!hasScriptPhysics) {
			engineFullPhysics();
		} else {
			engineMinimalPhysics();
		}
	}
		
	

	// We are painting; allow LUA paint calls
	allowLUAPaint = true;




	if(enableColor3D) {
		BS_Set3DMode(COLOR3D_LEFT);
		engineDoRender(COLOR3D_LEFT);
		BS_Set3DMode(COLOR3D_RIGHT);
		engineDoRender(COLOR3D_RIGHT);
		// BS_Set3DMode(COLOR3D_NONE); // Don't need this?
	} else {
		engineDoRender(COLOR3D_NONE);
	}

	
	allowLUAPaint = false;

	
	// Tell SDL to update the whole screen
	BS_Flip(gScreen);

#ifdef ALLOW_ACTIONCAPTURE
	renderActionCapture(spriterelx, spriterely);
#endif // ALLOW_ACTIONCAPTURE

#ifndef DISABLE_NETWORK
	if (blpport != 0) {
		sendBLPFrame();
	} ;
	if (mcufport != 0) {
		sendMCUFFrame();
	} ;
#endif // DISABLE_NETWORK


}

void engineDoRender(COLOR3D mode3D)
{

	Uint32 bg2offs, bgoffs, fgoffs, txtoffs;

	if(mode3D == COLOR3D_NONE) {
		bg2offs = bgoffs = fgoffs = txtoffs = 0;
	} else if(mode3D == COLOR3D_LEFT) {
		bg2offs = bgoffs = 0;
		fgoffs = 5;
		txtoffs = 2;
	} else {
		bg2offs = 20;
		bgoffs = 10;
		fgoffs = txtoffs = 0;
	}


	if(!lockedBG) {
		if(!lhandle.hasBG2) {
			drawBackground((fgXoffs / 2) + bgoffs, fgYoffs / 2);
		} else {
			drawBackground((fgXoffs / 4) + bg2offs, fgYoffs / 4);
			drawBackground2((fgXoffs / 2) + bgoffs, fgYoffs / 2);
		}
	} else {
		drawBackground(fgXoffs + bgoffs, fgYoffs);
	}

	// LUA Painting - STAGE 1
	if(lhandle.luaCB[CB_RENDER_STAGE_1].cbName[0] != 0) {
      	// Lock screen if needed
    	if (SDL_MUSTLOCK(gScreen))
    		if (SDL_LockSurface(gScreen) < 0)
    			return;
		blLuaCall(lhandle.blOOLuaState, lhandle.luaCB[CB_RENDER_STAGE_1].cbName, "");
    	// Unlock screen if needed
    	if (SDL_MUSTLOCK(gScreen))
    		SDL_UnlockSurface(gScreen);
	}

	paintFGObjs(fgXoffs, fgYoffs, false);
	paintLevelTiles(fgXoffs, fgYoffs);

	// LUA Painting - STAGE 2
	if(lhandle.luaCB[CB_RENDER_STAGE_2].cbName[0] != 0) {
       	// Lock screen if needed
    	if (SDL_MUSTLOCK(gScreen))
    		if (SDL_LockSurface(gScreen) < 0)
    			return;
		blLuaCall(lhandle.blOOLuaState, lhandle.luaCB[CB_RENDER_STAGE_2].cbName, "");
    	// Unlock screen if needed
    	if (SDL_MUSTLOCK(gScreen))
    		SDL_UnlockSurface(gScreen);
	}

	if(!hasScriptPhysics) {
		// Paint pixels only if it's internal engine physics
		paintLevelPixels(fgXoffs, fgYoffs, (Uint32)blinkbright);
	}

	// LUA Painting - STAGE 3
	if(lhandle.luaCB[CB_RENDER_STAGE_3].cbName[0] != 0) {
       	// Lock screen if needed
    	if (SDL_MUSTLOCK(gScreen))
    		if (SDL_LockSurface(gScreen) < 0)
    			return;
		blLuaCall(lhandle.blOOLuaState, lhandle.luaCB[CB_RENDER_STAGE_3].cbName, "");
    	// Unlock screen if needed
    	if (SDL_MUSTLOCK(gScreen))
    		SDL_UnlockSurface(gScreen);
	}
	
	if(!hasScriptPhysics) {
		// Paint sprites only if it's internal engine physics
		showMonsterSprites(fgXoffs, fgYoffs);
		showSprite(spriterelx, spriterely);
	}

	// LUA Painting - STAGE 4
	if(lhandle.luaCB[CB_RENDER_STAGE_4].cbName[0] != 0) {
       	// Lock screen if needed
    	if (SDL_MUSTLOCK(gScreen))
    		if (SDL_LockSurface(gScreen) < 0)
    			return;
		blLuaCall(lhandle.blOOLuaState, lhandle.luaCB[CB_RENDER_STAGE_4].cbName, "");
    	// Unlock screen if needed
    	if (SDL_MUSTLOCK(gScreen))
    		SDL_UnlockSurface(gScreen);
	}

	// LUA Painting - STAGE 5
	if(lhandle.luaCB[CB_RENDER_STAGE_5].cbName[0] != 0) {
       	// Lock screen if needed
    	if (SDL_MUSTLOCK(gScreen))
    		if (SDL_LockSurface(gScreen) < 0)
    			return;
		blLuaCall(lhandle.blOOLuaState, lhandle.luaCB[CB_RENDER_STAGE_5].cbName, "");
    	// Unlock screen if needed
    	if (SDL_MUSTLOCK(gScreen))
    		SDL_UnlockSurface(gScreen);
	}


	// HUD ("Head-up-display")
		// LUA Painting - STAGE HUD
	if(lhandle.luaCB[CB_RENDER_STAGE_HUD].cbName[0] != 0) {
       	// Lock screen if needed
    	if (SDL_MUSTLOCK(gScreen))
    		if (SDL_LockSurface(gScreen) < 0)
    			return;
		blLuaCall(lhandle.blOOLuaState, lhandle.luaCB[CB_RENDER_STAGE_HUD].cbName, "");
    	// Unlock if needed
    	if (SDL_MUSTLOCK(gScreen))
    		SDL_UnlockSurface(gScreen);
	} else {
		if(!enableQuickDraw) {
			if(!turboMode) {
				if(lhandle.leveltime <= 0) {
					blend_darkenRect(3, 3, SCR_WIDTH-6, 24, 0x00606060);
				} else {
					blend_darkenRect(3, 3, SCR_WIDTH-6, 48, 0x00606060);
				}
			} else {
				if(lhandle.leveltime <= 0) {
					blend_darkenRect(3, 3, SCR_WIDTH-6, 24, 0x00808080);
					blend_brightenRect(3, 3, SCR_WIDTH-6, 24, 0x00500000);
				} else {
					blend_darkenRect(3, 3, SCR_WIDTH-6, 48, 0x00808080);
					blend_brightenRect(3, 3, SCR_WIDTH-6, 48, 0x00500000);
				}
			}
		}
	}


	char printString[100];
	sprintf(printString, "Pixels: %u (%u)", foundPixels, lhandle.numPixels);
	renderFontHandlerText(20 + txtoffs, 2, printString, TEXTINFO_COLOR, false, false, FONT_textfont_20);

	sprintf(printString, "Lives: %u", gamedata.player->lives);
	renderFontHandlerText(190 + txtoffs, 2, printString, TEXTINFO_COLOR, false, false, FONT_textfont_20);

	sprintf(printString, "Level: %u", gamedata.player->level);
	renderFontHandlerText(310 + txtoffs, 2, printString, TEXTINFO_COLOR, false, false, FONT_textfont_20);

	sprintf(printString, "Score: %u", gamedata.player->score);
	renderFontHandlerText(440 + txtoffs, 2, printString, TEXTINFO_COLOR, false, false, FONT_textfont_20);

	if(enteringCheat) {
		if(!enableQuickDraw) {
			blend_darkenRect(5, SCR_HEIGHT-35, SCR_WIDTH-10, 30, 0x00808080);
			blend_brightenRect(5, SCR_HEIGHT-35, SCR_WIDTH-10, 30, 0x00500000);
		}
		if (SDL_GetTicks() % 200 < 100) { // blinking cursor
			sprintf(printString, "CHEAT: %s_", cheatCode);
		} else {
			sprintf(printString, "CHEAT: %s ", cheatCode);
		} ;
		renderFontHandlerText(20 + txtoffs, SCR_HEIGHT-30, printString, TEXTINFO_COLOR, false, false, FONT_textfont_20);
	}

	if(lhandle.leveltime > 0) {
		sprintf(printString, "Rem. Time: %d", (Uint32)lhandle.remaintime);
		if(lhandle.remaintime <= 10) { /* red font, if the leveltime <= 10 sec */
			renderFontHandlerText(20 + txtoffs, 25, printString, TEXTINFO_TIMEALERT, false, false, FONT_textfont_20);
		} else {
			renderFontHandlerText(20 + txtoffs, 25, printString, TEXTINFO_COLOR, false, false, FONT_textfont_20) ;
		} ;
		Uint32 timebar = 0;
		if(lhandle.remaintime <= lhandle.leveltime) {
			timebar = (Uint32)(((lhandle.leveltime - lhandle.remaintime) * 300.0) / lhandle.leveltime);
			drawrect(200, 35, timebar, 7, 0xff0000);
			drawrect(200 + timebar, 35, 300-timebar, 7, 0x00ff00);
		} else {
			drawrect(200, 35, 300, 7, 0x00ff00);
			drawrect(201, 36, 298, 5, (Uint32)(blinkbright + blinkbright * 256 + blinkbright * 65536));
			SDL_Color blibri1 = {(Uint8)blinkbright, (Uint8)blinkbright, (Uint8)blinkbright, 0};
			SDL_Color blibri2 = {(Uint8)(255 - blinkbright), (Uint8)(255 - blinkbright), (Uint8)(255 - blinkbright), (Uint8)blinkbright};
			renderFontHandlerText(520 + txtoffs, 27, "Bonus", blibri1, false, false, FONT_textfont_8);
			renderFontHandlerText(530 + txtoffs, 33, "Time", blibri2, false, false, FONT_textfont_8) ;
		}

	}

	renderFontHandlerText(20 + txtoffs, 45, errorMessage, TEXTINFO_TIMEALERT, true, true, FONT_menufont_20);

	paintFGObjs(fgXoffs + fgoffs, fgYoffs, true);



#ifdef ALLOW_POSITIONCAPTURE
	if(!isAttrackMode && poscapfh) {
		renderFontHandlerText(20 + txtoffs, 60, "Position Capture (smile, please)", TEXTINFO_COLOR, false, false, FONT_textfont_20);
	}
#endif // ALLOW_POSITIONCAPTURE


#ifdef DISPLAY_PLAYERCOORDS
	if(displayPlayerCoords && !enteringCheat) {
		if(!enableQuickDraw) {
			blend_darkenRect(5 + txtoffs, 445, SCR_WIDTH-10, 30, 0x00808080);
			blend_brightenRect(5 + txtoffs, 445, SCR_WIDTH-10, 30, 0x00500000);
		}
		sprintf(printString, "X: %u", (Uint32)spritex);
		renderFontHandlerText(20 + txtoffs, 450, printString, TEXTINFO_COLOR, false, false, FONT_textfont_20);
		sprintf(printString, "Y: %u", (Uint32)spritey);
		renderFontHandlerText(120 + txtoffs, 450, printString, TEXTINFO_COLOR, false, false, FONT_textfont_20);
		sprintf(printString, "X: %u", (Uint32)((spritex/TILESIZE)+1));
		renderFontHandlerText(330 + txtoffs, 450, printString, TEXTINFO_COLOR, false, false, FONT_textfont_20);
		sprintf(printString, "Y: %u", (Uint32)((spritey/TILESIZE)+1));
		renderFontHandlerText(420 + txtoffs, 450, printString, TEXTINFO_COLOR, false, false, FONT_textfont_20);
	}
#endif // DISPLAY_PLAYERCOORDS


	if(isAttrackMode) {
		// These texts are handled like fgobjects in Color3D
		if((BS_GetTicks() % 10000) < 5000) {
			renderFontHandlerText(50 + fgoffs, 200, "PLAY ME!", PLAYMECOLOR_ACTIVE, true, true, FONT_menufont_50);
		} else {
			renderFontHandlerText(50 + fgoffs, 200, "SPIEL MICH!", PLAYMECOLOR_ACTIVE, true, true, FONT_menufont_50);
		}
	}
#ifdef SHOW_INLAY
	if(!enteringCheat && !displayPlayerCoords) {
		renderFGInlay();
	}
#endif // SHOW_INLAY


	if(isPauseMode) {
		// Handle text like fgobj in Color3D
		if(!enableQuickDraw) {
			blend_darkenRect(10, 10, SCR_WIDTH-20, SCR_HEIGHT-20, 0x00808080);
		}
		renderFontHandlerText(50 + fgoffs, 200, "PAUSE MODE", PLAYMECOLOR_ACTIVE, true, true, FONT_menufont_50);
		SDL_Delay(100);
	} else if(lhandle.leveltime > 0 && lhandle.remaintime <= 10) {
		// Handle text like fgobj in Color3D
		if((int)lhandle.remaintime % 2) {
			renderFontHandlerText(50 + fgoffs, 200, "! HURRY UP !", TEXTINFO_TIMEALERT, true, true, FONT_menufont_50);
		}
	}

#ifdef SHOWFRAMERATE
	if(mode3D == COLOR3D_RIGHT) {
		displayFPSCounter(true);
	} else {
		displayFPSCounter();
	}
#endif // SHOWFRAMERATE

	applyOutputFilter();

}

char cheatCode[MAX_STRING_LENGTH];
bool enteringCheat = false;
void displayEngine(const char* attrackModeFile)
{

	leftpressed = false;
	rightpressed = false;
	SDL_Event event;

	// Special for attrack mode
	if(attrackModeFile) {
		gLastTick = BS_GetTicks();
		isAttrackMode = true;
		poscapfh = fopen(configGetPath(attrackModeFile), "rb");
		if(!poscapfh) {
			DIE(ERROR_FILE_READ, configGetPath(attrackModeFile));
		}

		// Catch all user input. Poll for events, and handle the ones we care about.
		while(gameRunning) {
			while(SDL_PollEvent(&event)) {
				switch (event.type)
				{
					case SDL_KEYUP:
						gameRunning = false;
						attracktModeRunning = false;
						break;
					case SDL_QUIT:
						exit(0);
				}
			}
			Uint32 joymove = getJoystickMoves();
			if(joymove & JOYSTICK_JUMP || joymove & JOYSTICK_ACTION) {
				gameRunning = false;
				attracktModeRunning = false;
			}
			renderEngine();
		}
		return;
	}


	Uint32 cheatTick = 0;
	gLastTick = BS_GetTicks();

	while (gameRunning && gamedata.player->lives > 0)
	{
		Uint32 ticks = BS_GetTicks();
		// Render stuff

		// Get Joystick moves
		Uint32 joymove = getJoystickMoves();
		if(!freeMovement) {
			if((joymove & JOYSTICK_JUMP) && !isJumping && (touchFGObject || (spritevy < SPRITE_FALLSILENTTHRESHOLD))) {
				isJumping = true;
				spritevy = SPRITE_JUMPSPEED;
			}
		}

		// Calculate reftright offset
		if((leftpressed && !rightpressed) || (joymove & JOYSTICK_LEFT)){
			leftright = -1;
		} else if((!leftpressed && rightpressed) || (joymove & JOYSTICK_RIGHT)){
			leftright = 1;
		} else {
			leftright = 0;
		}

		if(freeMovement) {
			// Calculate updown offset
			if((uppressed && !downpressed) || (joymove & JOYSTICK_UP)){
				updown = -1;
			} else if((!uppressed && downpressed) || (joymove & JOYSTICK_DOWN)){
				updown = 1;
			} else {
				updown = 0;
			}
			isJumping = 0;
		} else {
			uppressed = false;
			downpressed = false;
			updown = 0;
		}

		if(joymove & JOYSTICK_ACTION) {
			doFGPlayerAction((Uint32)spritex, (Uint32)spritey);
		}

		if(joymove & JOYSTICK_PAUSE) {
			isPauseMode = !isPauseMode;
		}

		renderEngine();
		if(ticks > cheatTick) {
			cheatTick = BS_GetTicks() + CHEATTIMEOUT;
			cheatCode[0] = 0;
			enteringCheat = false;
		}

		// Poll for events, and handle the ones we care about.
		while (SDL_PollEvent(&event) && gameRunning)
		{
			switch (event.type)
			{
				case SDL_KEYDOWN:
					if (event.key.keysym.sym == SDLK_LEFT ||
							event.key.keysym.sym == SDLK_KP4) {
						//leftright -= 1;
						leftpressed = true;
					} else if (event.key.keysym.sym == SDLK_RIGHT ||
								event.key.keysym.sym == SDLK_KP6) {
						//leftright += 1;
						rightpressed = true;
					} else if (event.key.keysym.sym == SDLK_UP ||
								event.key.keysym.sym == SDLK_KP8) {
						if(!freeMovement && event.key.keysym.sym != SDLK_KP8) {
							if(touchFGObject || (!isJumping && spritevy < SPRITE_FALLSILENTTHRESHOLD)) {
								isJumping = true;
								spritevy = SPRITE_JUMPSPEED;
							}
						} else {
							uppressed = true;
						}
					} else if (event.key.keysym.sym == SDLK_KP1) {
						if(!freeMovement) {
							if(touchFGObject || (!isJumping && spritevy < SPRITE_FALLSILENTTHRESHOLD)) {
								isJumping = true;
								spritevy = SPRITE_JUMPSPEED;
							}
						}

					} else if (event.key.keysym.sym == SDLK_DOWN ||
								event.key.keysym.sym == SDLK_KP2) {
						if(freeMovement) {
							downpressed = true;
						}
					} else if (event.key.keysym.sym == SDLK_PLUS ||
                              event.key.keysym.sym == SDLK_RIGHTBRACKET) {
						turboKeyPressed = true;
					}


					break;
				case SDL_KEYUP:
					CHECK_BOSSKEY ;
					if (event.key.keysym.sym == SDLK_LEFT ||
							event.key.keysym.sym == SDLK_KP4) {
						//leftright += 1;
						leftpressed = false;
					} else if (event.key.keysym.sym == SDLK_RIGHT ||
								event.key.keysym.sym == SDLK_KP6) {
						rightpressed = false;
						//leftright -= 1;
					} else if (event.key.keysym.sym == SDLK_UP ||
								event.key.keysym.sym == SDLK_KP8) {
						if(freeMovement || event.key.keysym.sym == SDLK_KP8) {
							uppressed = false;
						}
					} else if (event.key.keysym.sym == SDLK_DOWN ||
								event.key.keysym.sym == SDLK_KP2) {
						if(freeMovement) {
							downpressed = false;
						}
					} else if (event.key.keysym.sym == SDLK_PLUS ||
						event.key.keysym.sym == SDLK_RIGHTBRACKET) {
						turboKeyPressed = false;
					} else if (enteringCheat == false && event.key.keysym.sym == SDLK_RETURN) {
 	   					if(lhandle.musicPlaying == true) {
				 			lhandle.musicPlaying = false;
				 			soundStopMusic();
						} else {
							lhandle.musicPlaying = true;
							soundStartMusic(lhandle.sndfile);
						}
#ifdef ENABLE_FULLSCREEN
					} else if ((event.key.keysym.sym == SDLK_f) && (enteringCheat == false)) {
						/* Switching between fullscreen and
						* windowing not allowed, if we are entering a cheatcode now
						* (otherwise a cheatcode may not contain the character 'f' */
						isFullscreen = !isFullscreen;
						if(isFullscreen) {
							gScreen = BS_SetVideoMode(SCR_WIDTH, SCR_HEIGHT, 32, SDL_SWSURFACE | SDL_FULLSCREEN);
						} else {
							gScreen = BS_SetVideoMode(SCR_WIDTH, SCR_HEIGHT, 32, SDL_SWSURFACE);
						}
#endif // ENABLE_FULLSCREEN
#ifdef ALLOW_ACTIONCAPTURE
					} else if (event.key.keysym.sym == SDLK_y) {
						startActionCapture();
					} else if (event.key.keysym.sym == SDLK_x) {
						stopActionCapture();
#endif // ALLOW_ACTIONCAPTURE
					} else if (!enteringCheat && (event.key.keysym.sym == SDLK_SPACE ||
								event.key.keysym.sym == SDLK_KP3)) {
						doFGPlayerAction((Uint32)spritex, (Uint32)spritey);
#ifdef ALLOW_CHEATCODES
					} else if (!enteringCheat && event.key.keysym.sym == SDLK_q) {
					    if(guiYesNoDialog("End the running game?", "Are you sure?", false)) {
					        gamedata.player->lives = 0;
							gameRunning = false;;
                        }
					} else if (!enteringCheat && event.key.keysym.sym == SDLK_p) {
						isPauseMode = !isPauseMode;
					} else if (event.key.keysym.sym == SDLK_TAB && !isPauseMode) {
						enteringCheat = !enteringCheat;
						cheatCode[0] = 0;
					} else if (enteringCheat == true && event.key.keysym.sym == SDLK_RETURN) {
						if(strcmp(cheatCode, "nuke") == 0) {
							printf("Nuking all monsters and adding them to your score...\n");
							MONSTERS* monster = lhandle.monsters;
							while(monster) {
								if(monster->isAlive) {
									gamedata.player->score += monster->score;
									monster->isAlive = false;
									monster->isDying = true;
								}
								monster = monster->next;
							}
							gamedata.bphuc = true;
						} else if(strcmp(cheatCode, "skip") == 0) {
							gameRunning = false;
							gamedata.bphuc = true;
							gamedata.player->score = 0;
							showLoading();
						} else if(strncmp(cheatCode, "goto ", 5) == 0) {
							Uint32 lnum = (Uint32)atoi(((char*)cheatCode)+sizeof("goto"));
							if(lnum > 0) {
								gamedata.player->level = lnum - 1;
								lhandle.nextlevels[0] = lnum;
								gameRunning = false;
							}
							gamedata.bphuc = true;
							gamedata.player->score = 0;
							showLoading();
						} else if(strcmp(cheatCode, "lives") == 0) {
							gamedata.player->lives = 99;
							gamedata.bphuc = true;
						} else if(strcmp(cheatCode, "die") == 0) {
							gamedata.player->lives = 0;
							gameRunning = false;
							showLoading();
						} else if(strcmp(cheatCode, "quit") == 0) {
							printf("Trying fast exit....\n");
							displaymessage(displaymessage_INFO, "Exit using cheat code", 1000);
							exit(0);
						} else if(strcmp(cheatCode, "coords") == 0) {
							displayPlayerCoords = !displayPlayerCoords;
						} else if(strcmp(cheatCode, "freemode") == 0) {
							freeMovement = !freeMovement;
							gamedata.bphuc = true;
						} else if(strcmp(cheatCode, "godmode") == 0) {
							isGodMode = !isGodMode;
							gamedata.bphuc = true;
						} else if(strcmp(cheatCode, "timebonus") == 0) {
							if (gamedata.player->score > 5000) {
								gamedata.player->score -= 5000;
							} else {
								gamedata.player->score = 0;
							}
							lhandle.remaintime = lhandle.remaintime + 50;
							gamedata.bphuc = true;
						} else if(strcmp(cheatCode, "edit") == 0) {
							gamedata.player->score = 0;
							isEditMode = !isEditMode;
							freeMovement = isEditMode;
							isGodMode = isEditMode;
							displayPlayerCoords = isEditMode;
							soundPlayFX(FX_KILL_PLAYER);
							showPicture("livelost.jpg", 200);
							gLastTick = BS_GetTicks();
							gamedata.bphuc = true;
							if (isEditMode) {
								gamedata.player->lives = 999;
							}
						}
						cheatCode[0] = 0;
						enteringCheat = false;
					} else if(enteringCheat) {
						/* Cheatcode may only contain the characters in CHEATCODECHARS */
						if(strchr(CHEATCODECHARS, event.key.keysym.sym) && strlen(cheatCode) < 30) {
							cheatCode[strlen(cheatCode)+1] = '\0';
							cheatCode[strlen(cheatCode)] = event.key.keysym.sym;
						} ;
						if ((event.key.keysym.sym == SDLK_BACKSPACE)  && (strlen(cheatCode)>0)) { /* Backspace - delete last char */
							cheatCode[strlen(cheatCode)-1] = '\0' ;
						} ;
						cheatTick = BS_GetTicks() + CHEATTIMEOUT;
#endif // ALLOW_CHEATCODES
					}


					break;
				case SDL_QUIT:
					exit(0);
			}
		}

		if(turboKeyPressed || (joymove & JOYSTICK_TURBO)) {
			turboMode = true;
		} else {
			turboMode = false;
		}
	}

	if(lhandle.leveltime > 0 && gamedata.player->lives > 0) {
		addTimeToScore(); // Visibly add remaining time to score
	}
}


void addTimeToScore() {
	char tmp1[100];
	SDL_Event event;
	SDL_Color BONUSCOLOR_ACTIVE   = { 0xff, 0xff, 0xff, 0 };
	Uint32 wtick = BS_GetTicks() + 50;
	bool beep = true;

	if(enableColor3D) {
		BS_Set3DMode(COLOR3D_NONE);
	}

	while(lhandle.remaintime > 0) {
		lhandle.remaintime--;
		gamedata.player->score += lhandle.timebonusmultiplicator;
		sprintf(tmp1, "Remaining time: %d\n\nScore: %d\n\nPress space to continue immediately.", (Uint32)lhandle.remaintime, gamedata.player->score);

		drawrect(0, 0, SCR_WIDTH, SCR_HEIGHT, 0x00000000);
		renderFontHandlerText(0, 50, "SCORE", BONUSCOLOR_ACTIVE, true, false, FONT_menufont_50);
		renderFontHandlerText(50, 200, tmp1, BONUSCOLOR_ACTIVE, false, false, FONT_menufont_20);
		BS_Flip(gScreen);
		if(beep) {
			soundPlayFX(FX_COLLECT_PIXEL);
		}
		beep = !beep;
		while(wtick > BS_GetTicks()) {
			SDL_Delay(1);
		}
		wtick = BS_GetTicks() + 50;
               // Handle Keyboard
                SDL_PollEvent(&event) ;
		if(event.type == SDL_KEYUP) {
			CHECK_BOSSKEY ;
			if (event.key.keysym.sym == SDLK_SPACE) {
				/* Add score and continue game immediately */
				gamedata.player->score += lhandle.timebonusmultiplicator * ((Uint32)lhandle.remaintime) ;
				lhandle.remaintime = 0 ;
			}
		}
	}
	SDL_Delay(1000);
}

Uint32 BS_GetTicks() {
	if(!isPauseMode) {
		BS_MyTick += (SDL_GetTicks() - BS_MyLastTick);
	}
	BS_MyLastTick = SDL_GetTicks();
	return BS_MyTick;
}
