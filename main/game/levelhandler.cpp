// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#include "globals.h"
#include "levelhandler.h"
#include "tiles.h"
#include "background.h"
#include "background2.h"
#include "sound.h"
#include "drawprimitives.h"
#include "monstersprites.h"
#include "blending.h"
#include "errorhandler.h"
#include "fgobjects.h"
#include "bl_lua.h"
#include "triggers.h"
#include "outputfilter.h"
#include "engine.h"
#include "playersprite.h"
#include "SDL_gfxPrimitives.h"
#include "fonthandler.h"
LEVELHANDLER lhandle;


#include <stdio.h>
#include <string.h>

void initLevel(Uint32 level) {
	char val[100];
	char * tmp ;

	char fullfname_conf[MAX_FNAME_LENGTH];
	char shortfname_conf[MAX_FNAME_LENGTH];
	sprintf(fullfname_conf, "%slevel%d.conf", configGetPath(""), level);
	sprintf(shortfname_conf, "level%d.conf", level);

    // Init lhandle before parsing the config file
	lhandle.hasScript = false;
	lhandle.hasOOScript = false;
	lhandle.hasBG2 = false;
	lhandle.width = 0;
	lhandle.height = 0;
	lhandle.numBlocks = 0;
	lhandle.numPixels = 0;
	lhandle.dlist = 0;
	lhandle.pixels = 0;
	lhandle.numMonsters = 0;
	lhandle.monsters = 0;
	lhandle.hasMagicTiles = false;
	lhandle.magicTileSource = 0;
	lhandle.magicTileDestination = 0;
	lhandle.blLuaState = 0;
	lhandle.blOOLuaState = 0;
	lhandle.blConfigLuaState = 0;
	lhandle.scriptfile[0] = 0;
	lhandle.ooscriptfile[0] = 0;
	lhandle.bgfile[0] = 0;
	lhandle.bg2file[0] = 0;
	lhandle.sndfile[0] = 0;
	lhandle.tilesfile[0] = 0;
	lhandle.leveldata[0] = 0;
	lhandle.magictile[0] = 0;
	lhandle.leveltime = 0;
	lhandle.remaintime = 0.0;
	lhandle.timebonusmultiplicator = 10; /* default value = 10 */
	lhandle.timebonuspixel = 10; /* default value = 10 seconds */
	lhandle.timemalusifkatedies = 1000; /* default value = 1000 */
	memset(&lhandle.luaCB, 0, sizeof(lhandle.luaCB));
	for (int i= 0 ; i<100; i++) lhandle.nextlevels[i] = (int) level + 1 ;


    // Parse the config file
    blLuaInit(shortfname_conf, LUABINDINGTYPE_CONFIG);

	// Prepare for real LUA scripting
	initFGObjs();
	initPlayerSprite();
	initSoundFXLua();
	initTriggers();
	initOutputFilter();

	if (strlen(lhandle.bg2file) > 0) lhandle.hasBG2 = true ;
	if (strlen(lhandle.scriptfile) > 0) lhandle.hasScript = true ;
	if (strlen(lhandle.ooscriptfile) > 0) lhandle.hasOOScript = true ;
	lhandle.remaintime = (double)lhandle.leveltime;

	/* handle magictiles. */
	if (strlen(lhandle.magictile) > 0) {
		sprintf(val, "%s", lhandle.magictile);
		tmp = strchr(val, ':');
		if(tmp) {
			*tmp = 0;
			tmp++;
			lhandle.magicTileSource = (Uint32)atoi(lhandle.magictile) - 1;
			lhandle.magicTileDestination = (Uint32)atoi(tmp) - 1;
			lhandle.hasMagicTiles = true;
		} ;
	};

	soundStartMusic(lhandle.sndfile);
	lhandle.musicPlaying = true;

	FILE* fh = fopen(configGetPath(lhandle.leveldata), "r");
	if(!fh) {
		DIE(ERROR_FILE_READ, configGetPath(lhandle.leveldata));
	}
	char line[MAX_STRING_LENGTH];
	bool first_tile = true;
	Uint8 type;
	char tchar[2];
	tchar[1] = 0;
	Uint32 x = 0;
	Uint32 y = 0;
	DISPLAYLIST* thandle;
	DISPLAYLIST* lasthandle_tiles = 0;
	while(fgets(line, sizeof(line), fh)) {
		while((tmp = strchr(line, '\n'))) {
			*tmp = 0;
		}
		while((tmp = strchr(line, '\r'))) {
			*tmp = 0;
		}

		x = 0;
		for(Uint32 i = 0; i < strlen(line); i++) {
			tchar[0] = line[i];
			if(tchar[0] == '+') {
				lhandle.respawn_x = lhandle.start_x = x;
				lhandle.respawn_y = lhandle.start_y = y;
				addPixel(x, y, PIXELTYPE_RESPAWN); // startpoint is first respawn poinr
			} else if(tchar[0] == '-') {
				addPixel(x, y, PIXELTYPE_EXIT);
			} else if(tchar[0] == '#') {
				addPixel(x, y, PIXELTYPE_PIXEL);
				lhandle.numPixels ++ ;
			} else if(tchar[0] == 'P') {
				addPixel(x, y, PIXELTYPE_POWER);
				lhandle.numPixels ++ ;
			} else if(tchar[0] == 'T') {
				addPixel(x, y, PIXELTYPE_TIMEBONUS);
			} else if(tchar[0] == '$') {
				addPixel(x, y, PIXELTYPE_POINTS);
			} else if(tchar[0] == '~') {
				addPixel(x, y, PIXELTYPE_RESPAWN);
			} else if(tchar[0] == '*') {
				addPixel(x, y, PIXELTYPE_LIVE);
			} else if(tchar[0] >= 65 && tchar[0] < 65 + MAX_MONSTER_TYPES) {
				addMonster(tchar[0] - 65, x, y);
			} else if((type = atoi(tchar)) > 0 || (tchar[0] >= 'a' && tchar[0] <= 'z')) {
				if(type) {
					type--; // In file, first tile-type = 1 !!
				} else {
					// User scriptable "tiles"
					type = (Uint8)tchar[0];
				}
				thandle = (DISPLAYLIST*)malloc(sizeof(DISPLAYLIST));
				thandle->type = type;
				thandle->x = x;
				thandle->y = y;
				thandle->next = 0;
				if(first_tile) {
					lhandle.dlist = thandle;
					first_tile = false;
				} else {
					lasthandle_tiles->next = thandle;
				}
				lasthandle_tiles = thandle;
				lhandle.numBlocks++;
			}
			x += TILESIZE;
			if(x > lhandle.width) {
				lhandle.width = x;
			}
		}
		y += TILESIZE;
		if(y > lhandle.height) {
			lhandle.height = y;
		}
	}


	initBackground(lhandle.bgfile);
	if(lhandle.hasBG2) {
		initBackground2(lhandle.bg2file);
	}
	initTiles(lhandle.tilesfile);

	if(lhandle.width < (unsigned int)SCR_WIDTH) {
		lhandle.width = SCR_WIDTH;
	}
	if(lhandle.height < (unsigned int)SCR_HEIGHT) {
		lhandle.height = SCR_HEIGHT;
	}

	Uint32 t2dcount = ((lhandle.height / TILESIZE) * (lhandle.width / TILESIZE));
	lhandle.tiles2d = (Uint8 *)malloc(t2dcount * sizeof(Uint8));
	memset(lhandle.tiles2d, 0, t2dcount * sizeof(Uint8));
	initTiles2D();

	if(lhandle.hasScript) {
		//printf("Script-File: %s\n", lhandle.scriptfile);
		blLuaInit(lhandle.scriptfile, LUABINDINGTYPE_OLDSTYLE);
		blLuaCall(lhandle.blLuaState, "initLevel", "");
	}

	if(lhandle.hasOOScript) {
		//printf("Script-File: %s\n", lhandle.scriptfile);
		blLuaInit(lhandle.ooscriptfile, LUABINDINGTYPE_OO);
		// OO-LUA LUA Physics Callback
		if(lhandle.luaCB[CB_ENGINE_INIT].cbName[0] != 0) {
			blLuaCall(lhandle.blOOLuaState, lhandle.luaCB[CB_ENGINE_INIT].cbName, "");
		}
	}


	return;
}

void deInitLevel() {

	// deInit LUA add-ons
	deInitPlayerSprite();
	deInitFGObjs();
	deInitSoundFXLua();

	free(lhandle.tiles2d);


    blLuaDeInit();

	deInitTiles();
	deInitBackground();
	if(lhandle.hasBG2) {
		deInitBackground2();
	}

	DISPLAYLIST* tthis;
	DISPLAYLIST* tnext = lhandle.dlist;
	tthis = lhandle.dlist;
	while(tnext) {
		tnext = tthis->next;
		free(tthis);
		tthis = tnext;
	}
	lhandle.dlist = 0;

	freeMonsters();
	soundStopMusic();
	deInitTriggers();
	deInitOutputFilter();

	return;
}

void addPixel(Uint32 x, Uint32 y, Uint32 pixeltype) {

	DISPLAYLIST* thandle = (DISPLAYLIST*)malloc(sizeof(DISPLAYLIST));
	thandle->type = pixeltype;
	thandle->x = x;
	thandle->y = y;
	thandle->next = 0;
	if(!lhandle.pixels) {
		lhandle.pixels = thandle;
	} else {
		// Find next pixel
		DISPLAYLIST *mhandle = lhandle.pixels;
		while(mhandle->next) {
			mhandle = mhandle->next;
		}
		mhandle->next = thandle;
	}
}


void paintLevelPixels(Uint32 xoffs, Uint32 yoffs, Uint32 blinker) {
	DISPLAYLIST *tthis = lhandle.pixels;
	Uint32 color = (Uint32)blinkbright + (Uint32)blinkbright * 256 + (Uint32)blinkbright * 65536;
	Uint32 livecolor = (255 - (Uint32)blinkbright) + (Uint32)blinkbright * 65536;
	Sint32 xabs = 0;
	Sint32 yabs = 0;
	Uint32 pitch = gScreen->w;
	static Sint32 respawncoloffs = 0;
	static Sint32 goalcoloffs = 0;
	static Sint32 lasttick = 0;
	Uint32 tilecolor;
	Uint32 bgcolor;

	Sint32 tick = BS_GetTicks();
	// Ignore delays over 1 second
	if((lasttick + 1000) < tick) {
		lasttick = tick;
	}

	while(lasttick < tick) {
		respawncoloffs = (respawncoloffs + 1) % TILESIZE;
		//startcoloffs = (startcoloffs + 1) % TILESIZE;
		goalcoloffs--;
		if(goalcoloffs < 0) {
			goalcoloffs = TILESIZE - 1;
		}
		if(!turboMode) {
			lasttick += SPECIALTILEDELAY;
		} else {
			lasttick += SPECIALTILEDELAY / 4;
		}
	}


	while(tthis) {
		xabs = tthis->x - xoffs;
		yabs = tthis->y - yoffs;
		if(tthis->type != PIXELTYPE_NONE && xabs > -TILESIZE && yabs > -TILESIZE && xabs < SCR_WIDTH && yabs < SCR_HEIGHT) {
			if(tthis->type == PIXELTYPE_PIXEL || tthis->type == PIXELTYPE_POWER) {
				drawrect(xabs, yabs, TILESIZE, TILESIZE, 0x000000a0);
				drawrect(xabs + 2, yabs + 2, TILESIZE - 4, TILESIZE - 4, color);
				if(tthis->type == PIXELTYPE_POWER) {
					renderFontHandlerText(xabs + 10, yabs + 5, "P", BS_Color_WHITE, false, false, FONT_textfont_20);
				}
			} else if(tthis->type == PIXELTYPE_TIMEBONUS) {
				drawrect(xabs, yabs, TILESIZE, TILESIZE, 0x000000a0);
				drawrect(xabs + 2, yabs + 2, TILESIZE - 4, TILESIZE - 4, color);
				renderFontHandlerText(xabs + 10, yabs + 5, "T", BS_Color_WHITE, false, false, FONT_textfont_20);
			} else if(tthis->type == PIXELTYPE_POINTS) {
				drawrect(xabs + TILESIZE/3, yabs, TILESIZE/3, TILESIZE, 0x0000a000);
				drawrect(xabs, yabs + TILESIZE/3, TILESIZE, TILESIZE/3, 0x0000a000);
				drawrect(xabs + TILESIZE/3 + 1, yabs + 1, TILESIZE/3 - 2, TILESIZE - 2, livecolor);
				drawrect(xabs + 1, yabs + TILESIZE/3 + 1, TILESIZE - 2, TILESIZE/3 - 2, livecolor);
			} else if(tthis->type == PIXELTYPE_LIVE) {
				circleColor(gScreen,xabs + TILESIZE/2, yabs + TILESIZE/2, TILESIZE/2, 0x0000a0);
				circleColor(gScreen,xabs + TILESIZE/2, yabs + TILESIZE/2, TILESIZE/2 - 1, color);
				circleColor(gScreen,xabs + TILESIZE/2, yabs + TILESIZE/2, TILESIZE/4, 0xa000a0);
				circleColor(gScreen,xabs + TILESIZE/2, yabs + TILESIZE/2, TILESIZE/6, livecolor);
			} else if(canPaintSpecialTiles && tthis->type == PIXELTYPE_RESPAWN) {
        		for(Sint32 x = xabs; x < (xabs + TILESIZE); x++) {
        			if(x > 0 && x < SCR_WIDTH) {
        				for(Sint32 y = yabs; y < (yabs + TILESIZE); y++) {
        					if(y > 0 && y < SCR_HEIGHT) {
                                if(tthis->x == lhandle.respawn_x && tthis->y == lhandle.respawn_y) {
                                    // Start Tile
                                	bgcolor =  blend_mul(BS_Unpack(gScreen->pixels[x + y * pitch]), 0x00707040);
                                    tilecolor = (((TILESIZE - (abs(x - xabs - TILESIZE/2) + abs(y - yabs - TILESIZE/2))) + respawncoloffs) % TILESIZE) * 16;
                                } else {
                                    // Respawn point
        						    bgcolor =  blend_mul(BS_Unpack(gScreen->pixels[x + y * pitch]), 0x00707070);
        						    tilecolor = (((TILESIZE - (abs(x - xabs - TILESIZE/2) + abs(y - yabs - TILESIZE/2))) + respawncoloffs) % TILESIZE) * 8;
        						    tilecolor = tilecolor + (tilecolor << 8) + (tilecolor << 16)/*+ tilecolor << 8 + tilecolor << 16*/;
                                }
        						gScreen->pixels[x + y * pitch] =  BS_PackOpaque(blend_add(bgcolor, tilecolor));
        					}
        				}
        			}
        		}
        	} else if(canPaintSpecialTiles && tthis->type == PIXELTYPE_EXIT) {
				for(Sint32 x = xabs; x < (xabs + TILESIZE); x++) {
					if(x > 0 && x < SCR_WIDTH) {
						for(Sint32 y = yabs; y < (yabs + TILESIZE); y++) {
							if(y > 0 && y < SCR_HEIGHT) {
								if(allowedToExit) {
									tilecolor = ((((TILESIZE - (abs(x - xabs - TILESIZE/2) + abs(y - yabs - TILESIZE/2))) + goalcoloffs) % TILESIZE) * 16) << 8;
									color =  blend_mul(BS_Unpack(gScreen->pixels[x + y * pitch]), 0x00704070);
								} else {
									tilecolor = ((((TILESIZE - (abs(x - xabs - TILESIZE/2) + abs(y - yabs - TILESIZE/2))) + goalcoloffs) % TILESIZE) * 16) << 16;
									color =  blend_mul(BS_Unpack(gScreen->pixels[x + y * pitch]), 0x00407070);
								}
								gScreen->pixels[x + y * pitch] =  BS_PackOpaque(blend_add(color, tilecolor));
							}
						}
					}
				}
			}
		}
		tthis = tthis->next;
	}
}

void initTiles2D() {
	printf("Init Tiles2D\n");
	Uint32 elemCount = 0;
	DISPLAYLIST *tthis = lhandle.dlist;
	while(tthis) {
		Uint32 pos = (tthis->x / TILESIZE) + (tthis->y / TILESIZE) * (lhandle.width / TILESIZE);
		elemCount++;
		if(tthis->type < 10) {
			lhandle.tiles2d[pos] = (Uint8)tthis->type + 1; // Undo offset correction for normal tiles
		} else {
			lhandle.tiles2d[pos] = (Uint8)tthis->type;
		}
		tthis = tthis->next;
	}
	printf("Number of tile objects: %d\n", elemCount);

	return;
}

bool checkForTile(Sint32 x, Sint32 y) {
	if(x < 0 || x > (Sint32)lhandle.width || y < 0 || y > (Sint32)lhandle.height) {
		return false;
	}
	Uint32 pos = (x / TILESIZE) + (y / TILESIZE) * (lhandle.width / TILESIZE);
	if(lhandle.tiles2d[pos] > 0 && lhandle.tiles2d[pos] < 10) {
		return true;
	}
	return false;
}

Uint8 getTileType(Sint32 x, Sint32 y) {
	if(x < 0 || x > (Sint32)lhandle.width || y < 0 || y > (Sint32)lhandle.height) {
		return 0;
	}
	Uint32 pos = (x / TILESIZE) + (y / TILESIZE) * (lhandle.width / TILESIZE);
	return lhandle.tiles2d[pos];
}
