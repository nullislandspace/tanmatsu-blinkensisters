// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//

#define NEED_REGISTER_FUNCTION
#include "bl_lua.h"
#undef NEED_REGISTER_FUNCTION

#include "bl_lua_objbindings.h"


// Include all OOLUA modules
#include "bl_lua_obj_anim.h"
#include "bl_lua_obj_callbacks.h"
#include "bl_lua_obj_engine.h"
#include "bl_lua_obj_fgobjects.h"
#include "bl_lua_obj_gfx.h"
#include "bl_lua_obj_level.h"
#include "bl_lua_obj_paint.h"
#include "bl_lua_obj_player.h"
#include "bl_lua_obj_screen.h"
#include "bl_lua_obj_shared.h"
#include "bl_lua_obj_sound.h"
#include "bl_lua_obj_timer.h"

// Other headers
#include "fgobjects.h"
#include "playersprite.h"

// Required for Paint-Callbacks
bool allowLUAPaint;


// +++++++++++++++++++++++   REGISTERING GLOBALS IN LUA 	+++++++++++++++++++++++++++++++

void blRegisterLuaObjBindings(lua_State *L) {
	allowLUAPaint = false;
	
	bsl_starttable_global("bs");
	bsl_addfunction("die", luaObjDie);
	bsl_addfunction("include", luaObjInclude);
	bsl_addfunction("playvideo", luaObjPlayVideo);
	
	// various screen functions
	bsl_starttable_local("screen");
	// Screen and tile resolution
	bsl_starttable_local("resolution");
	bsl_addfunction("x", luaObjResolutionX);
	bsl_addfunction("y", luaObjResolutionY);
	bsl_addfunction("tilesize", luaObjResolutionTilesize);
	bsl_endtable_local("resolution");
    
    bsl_starttable_local("filter");
    bsl_addfunction("fading", luaObjScreenFilterFading);
    bsl_starttable_local("color");
    bsl_addobjectfunction("greyscale", luaObjScreenFilterProperty, OUTPUTFILTER_GREYSCALE);
    bsl_addobjectfunction("invert", luaObjScreenFilterProperty, OUTPUTFILTER_COLORINVERT);
    bsl_endtable_local("color"); 
    bsl_starttable_local("noise");
    bsl_addobjectfunction("rectangle", luaObjScreenFilterProperty, OUTPUTFILTER_NOISERECT);
    bsl_addobjectfunction("circle", luaObjScreenFilterProperty, OUTPUTFILTER_NOISECIRCLE);
    bsl_addobjectfunction("stripes", luaObjScreenFilterProperty, OUTPUTFILTER_NOISESTRIPE);
    bsl_endtable_local("noise"); 
    bsl_endtable_local("filter"); 
    
    bsl_endtable_local("screen");
	
	// callbacks
	bsl_starttable_local("callback");
		// Paint callbacks
	bsl_starttable_local("paint");
	bsl_addobjectfunction("stage1", luaObjRegisterCallback, CB_RENDER_STAGE_1);
	bsl_addobjectfunction("stage2", luaObjRegisterCallback, CB_RENDER_STAGE_2);
	bsl_addobjectfunction("stage3", luaObjRegisterCallback, CB_RENDER_STAGE_3);
	bsl_addobjectfunction("stage4", luaObjRegisterCallback, CB_RENDER_STAGE_4);
	bsl_addobjectfunction("stage5", luaObjRegisterCallback, CB_RENDER_STAGE_5);
	bsl_addobjectfunction("hud", luaObjRegisterCallback, CB_RENDER_STAGE_HUD);
	bsl_endtable_local("paint");

	// Engine callbacks
	bsl_starttable_local("engine");
	bsl_addobjectfunction("init",   luaObjRegisterCallback, CB_ENGINE_INIT);
	bsl_addobjectfunction("physics",luaObjRegisterCallback, CB_ENGINE_PHYSICS);
	bsl_endtable_local("engine");
	
	bsl_endtable_local("callback");
	
		
	// Sound
	bsl_starttable_local("snd");
	bsl_addfunction("load", luaObjLoadSoundFX);
	bsl_endtable_local("snd");

	// Timer
	bsl_starttable_local("timer");
	bsl_addfunction("new", luaObjTimerNew);
	bsl_endtable_local("timer");
	
	// Graphic
	bsl_starttable_local("gfx");
	bsl_addfunction("load", luaObjLoadGraphic);
	bsl_endtable_local("gfx");
	
	// Anims
	bsl_starttable_local("anim");
	bsl_addfunction("load", luaObjLoadAnim);
	bsl_addfunction("duplicate", luaObjDuplicateAnim);
	
	// anim-looptypes
	bsl_starttable_local("type");
	bsl_addnumbervalue("forwardloop", ANIMLOOPTYPE_FORWARDLOOP);
	bsl_addnumbervalue("reverseloop", ANIMLOOPTYPE_REVERSELOOP);
	bsl_addnumbervalue("pingpong", ANIMLOOPTYPE_PINGPONG);
	bsl_endtable_local("type");

	// anim-directions for pingpong type
	bsl_starttable_local("direction");
	bsl_addnumbervalue("forward", 1);
	bsl_addnumbervalue("backward", -1);
	bsl_endtable_local("direction");
	
	bsl_endtable_local("anim");
	
	// Objects
	bsl_starttable_local("obj");
	bsl_addfunction("new", luaObjNewFGObject);
	bsl_endtable_local("obj");
	

	// Player
	bsl_starttable_local("player");
	bsl_addfunction("x", luaObjPlayerX);
	bsl_addfunction("y", luaObjPlayerY);
	bsl_addfunction("speed_x", luaObjPlayerVX);
	bsl_addfunction("speed_y", luaObjPlayerVY);
	bsl_addfunction("kill", luaObjKillPlayer);
	bsl_addfunction("score", luaObjPlayerScore);
	bsl_addfunction("lives", luaObjPlayerLives);
	
	bsl_starttable_local("callback");
	bsl_addobjectfunction("livelost", luaObjRegisterCallback, CB_PLAYER_LIVELOST);
	bsl_endtable_local("callback");
	
	/*
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
	 */
	bsl_starttable_local("movetype");
	bsl_addnumbervalue("nomove", PLAYERSPRITE_NOMOVE);
	bsl_addnumbervalue("left", PLAYERSPRITE_LEFT);
	bsl_addnumbervalue("leftup", PLAYERSPRITE_LEFTUP);
	bsl_addnumbervalue("leftdown", PLAYERSPRITE_LEFTDOWN);
	bsl_addnumbervalue("right", PLAYERSPRITE_RIGHT);
	bsl_addnumbervalue("rightup", PLAYERSPRITE_RIGHTUP);
	bsl_addnumbervalue("rightdown", PLAYERSPRITE_RIGHTDOWN);
	bsl_addnumbervalue("up", PLAYERSPRITE_UP);
	bsl_addnumbervalue("down", PLAYERSPRITE_DOWN);
	bsl_endtable_local("movetype");
	bsl_addfunction("gfx", luaObjPlayerGFX);
	
	
	bsl_endtable_local("player");

	// Level
	bsl_starttable_local("level");
	bsl_addfunction("gravity", luaObjLevelGravity);
	bsl_addfunction("freemovement", luaObjLevelFreeMovement);
	bsl_addfunction("exitopen", luaObjLevelCanExit);
	bsl_addfunction("paintspecialtiles", luaObjLevelPaintSpecialTiles);
	bsl_addfunction("lockedbg", luaObjLevelLockedBG);
	
	bsl_starttable_local("time");
	bsl_addfunction("total", luaObjLevelTimeTotal);
	bsl_addfunction("remaining", luaObjLevelTimeRemaining);
	bsl_addfunction("used", luaObjLevelTimeUsed);
	bsl_endtable_local("time");
	
	// Tiles (arrays, max coords, types)
	bsl_starttable_local("tiles");
	// "Types" Char to Number mapping
	bsl_starttable_local("types");
	char name[100];
	for(Uint32 tn = 1; tn < 9; tn++) {
		sprintf(name, "char_%d", tn); 
		bsl_addnumbervalue(name, tn);
	}
	for(char tt = 'a'; tt <= 'z'; tt++) {
		sprintf(name, "char_%c", tt); 
		bsl_addnumbervalue(name, tt);
	}
	bsl_endtable_local("types");

	 
	// min/max size of tiles
	bsl_starttable_local("size");
	bsl_addnumbervalue("minx", 0);
	bsl_addnumbervalue("miny", 0);
	bsl_addnumbervalue("maxx", lhandle.width);
	bsl_addnumbervalue("maxy", lhandle.height);
	bsl_endtable_local("size");

	Uint32 starttick = SDL_GetTicks();
	Uint32 elemcount = 0;
	bsl_starttable_local("data");
	
	char nElemCount[20];
	/*
	for(Uint32 px = 0; px <= lhandle.height; px++) {
		for(Uint32 py = 0; py <= lhandle.width; py++) {
			Uint8 tileType = getTileType(px, py);
			if(tileType) {
				//printf("%d/%d:%d\n", px, py, tileType);
				sprintf(nElemCount, "%d", elemcount);
				bsl_starttable_local(nElemCount);
				bsl_addnumbervalue("type", tileType);
				bsl_addnumbervalue("x", px);
				bsl_addnumbervalue("y", py);
				bsl_endtable_local(nElemCount);
				elemcount++;
			}
		}
	}
	*/
	
	DISPLAYLIST *tthis = lhandle.dlist;
	Uint8 tileType = 0;
	while(tthis) {
		if(tthis->type < 10) {
			tileType = (Uint8)tthis->type + 1; // Undo offset correction for normal tiles
		} else {
			tileType = (Uint8)tthis->type;
		}
		sprintf(nElemCount, "%d", elemcount);
		bsl_starttable_local(nElemCount);
		bsl_addnumbervalue("type", tileType);
		bsl_addnumbervalue("x", tthis->x);
		bsl_addnumbervalue("y", tthis->y);
		bsl_endtable_local(nElemCount);
		elemcount++;
		tthis = tthis->next;
	}
	
	bsl_endtable_local("data");
	Uint32 endtick = SDL_GetTicks();
	printf("Tiles2Array took %d ticks for %d elements\n", endtick - starttick, elemcount);
	
	bsl_endtable_local("tiles");
	
	
	//   --sub level.pixels
	bsl_starttable_local("pixels");
	bsl_addfunction("collected", luaObjLevelPixelsCollected);
	bsl_addfunction("required", luaObjLevelPixelsRequired);
	bsl_endtable_local("pixels");
	
	bsl_endtable_local("level");
	
	// "enums"
	bsl_addnumbervalue("yes", 1);
	bsl_addnumbervalue("no", 0);
	
	bsl_endtable_global("bs");
}

