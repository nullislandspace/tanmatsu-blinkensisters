// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#ifndef LEVELHANDLER_H
#define LEVELHANDLER_H

// include the monster-handler here because we need the monster structure
#include "monstersprites.h"
#include "lua.h"

struct DISPLAYLIST {
	Uint8 type;
	Sint32 x;
	Sint32 y;
	DISPLAYLIST* next;
};

typedef enum _PIXELSTYPE {
	PIXELTYPE_NONE = 0,
	PIXELTYPE_PIXEL,
	PIXELTYPE_LIVE,
	PIXELTYPE_POINTS,
	PIXELTYPE_POWER,
	PIXELTYPE_TIMEBONUS,
	/* Special "Pixels" e.g. Respawn and Exit */
	PIXELTYPE_RESPAWN,
	PIXELTYPE_EXIT
} PIXELTYPE;


typedef enum _CALLBACK_TYPES {
	CB_RENDER_STAGE_1 = 0,
	CB_RENDER_STAGE_2,
	CB_RENDER_STAGE_3,
	CB_RENDER_STAGE_4,
	CB_RENDER_STAGE_5,
	CB_RENDER_STAGE_HUD,
	CB_ENGINE_INIT,
	CB_ENGINE_PHYSICS,
	CB_PLAYER_LIVELOST,
	CB_LAST
} CALLBACK_TYPES;

struct LUA_CALLBACKS {
	char cbName[MAX_STRING_LENGTH];
};

struct LEVELHANDLER {
	Uint32 leveltime;
	double remaintime;
	Uint32 timebonusmultiplicator;
	Uint32 timemalusifkatedies;
	Uint32 timebonuspixel;
	char bgfile[MAX_STRING_LENGTH];
	char bg2file[MAX_STRING_LENGTH];
	char sndfile[MAX_STRING_LENGTH];
	char tilesfile[MAX_STRING_LENGTH];
	char leveldata[MAX_STRING_LENGTH];
	char scriptfile[MAX_STRING_LENGTH];
	char ooscriptfile[MAX_STRING_LENGTH];
	char magictile[MAX_STRING_LENGTH];
	Uint32 height;
	Uint32 width;
	Uint32 numBlocks;
	Uint32 numPixels;
	Sint32 start_x;
	Sint32 start_y;
	Sint32 respawn_x;
	Sint32 respawn_y;
	Uint32 numberofgoals;
	bool hasScript;
	bool hasOOScript;
	bool hasBG2;
	bool hasMagicTiles;
	Uint32 magicTileSource;
	Uint32 magicTileDestination;
	Uint32 numMonsters;
	Uint8* tiles2d;
	bool musicPlaying;
	DISPLAYLIST* dlist;
	DISPLAYLIST* pixels;
	MONSTERS* monsters;
	LUA_CALLBACKS luaCB[CB_LAST];
	lua_State* blLuaState;
	lua_State* blOOLuaState;
	lua_State* blConfigLuaState;
	Uint32 nextlevels[100] ;       /*nextlevel depending on number of exits. Currently no more than 100 exits are supportes (FIXME) */
	Uint32 currentexit ;           /* store the current used exit Pixel */
};

extern LEVELHANDLER lhandle;

void initLevel(Uint32 level);
void deInitLevel();
void addPixel(Uint32 x, Uint32 y, Uint32 pixeltype);
void paintLevelPixels(Uint32 xoffs, Uint32 yoffs, Uint32 blinker);
bool checkForTile(Sint32 x, Sint32 y);
void initTiles2D();
Uint8 getTileType(Sint32 x, Sint32 y);

#endif // LEVELHANDLER_H
