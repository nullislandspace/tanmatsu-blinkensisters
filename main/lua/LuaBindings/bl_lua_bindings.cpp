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

#include "globals.h"
#include "bl_lua_bindings.h"
#include "fgobjects.h"
#include "errorhandler.h"
#include "lua.h"
#include "sound.h"
#include "levelhandler.h"
#include "engine.h"
#include "triggers.h"
#include "outputfilter.h"

static int l2bDie(lua_State *L) {
	if(!lua_isstring(L, 1)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		LUADIE(L, ERROR_LUAL2BDIE, (char *)lua_tostring(L, 1));
	}
	return 0;
}

static int l2bLoadGFX(lua_State *L) {
	char tmp[1000];
	if(!lua_isstring(L, 1)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		sprintf(tmp, "%s", lua_tostring(L, 1));
	}

	Uint32 idx = addFGObjGFX(tmp);
	lua_pushnumber(L, idx);

	return 1;
}

static int l2bAddObject(lua_State *L) {
	Uint32 gfxobj, x, y;
	gfxobj = x = y = 0;
	bool isBlocking = false;
	bool isVisible = false;
	bool isOnTop = false;
	bool isKilling = false;
	if(!lua_isnumber(L, 1) ||
	   !lua_isnumber(L, 2) ||
	   !lua_isnumber(L, 3) ||
	   !lua_isnumber(L, 4) ||
	   !lua_isnumber(L, 5) ||
	   !lua_isnumber(L, 6) ||
	   !lua_isnumber(L, 7)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		gfxobj = (Uint32)lua_tonumber(L, 1);
		x = (Uint32)lua_tonumber(L, 2) * TILESIZE;
		y = (Uint32)lua_tonumber(L, 3) * TILESIZE;
		if(lua_tonumber(L, 4) > 0) {
			isBlocking = true;
		}
		if(lua_tonumber(L, 5) > 0) {
			isVisible = true;
		}
		if(lua_tonumber(L, 6) > 0) {
			isOnTop = true;
		}
		if(lua_tonumber(L, 7) > 0) {
			isKilling = true;
		}

	}

	Uint32 idx = addFGObj(gfxobj, x, y, isBlocking, isVisible, isOnTop, isKilling);
	lua_pushnumber(L, idx);

	return 1;
}


static int l2bSetVisible(lua_State *L) {
	Uint32 gfxobj = 0;
	bool isVisible = false;
	if(!lua_isnumber(L, 1) ||
	   !lua_isnumber(L, 2)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		gfxobj = (Uint32)lua_tonumber(L, 1);
		if(lua_tonumber(L, 2) > 0) {
			isVisible = true;
		}

	}

	setFGObjSetVisible(gfxobj, isVisible);

	return 0;
}

static int l2bSetBlocking(lua_State *L) {
	Uint32 gfxobj = 0;
	bool isBlocking = false;
	if(!lua_isnumber(L, 1) ||
	   !lua_isnumber(L, 2)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		gfxobj = (Uint32)lua_tonumber(L, 1);
		if(lua_tonumber(L, 2) > 0) {
			isBlocking = true;
		}

	}

	setFGObjSetBlocking(gfxobj, isBlocking);

	return 0;
}

static int l2bSetOnTop(lua_State *L) {
	Uint32 gfxobj = 0;
	bool isOnTop = false;
	if(!lua_isnumber(L, 1) ||
	   !lua_isnumber(L, 2)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		gfxobj = (Uint32)lua_tonumber(L, 1);
		if(lua_tonumber(L, 2) > 0) {
			isOnTop = true;
		}

	}

	setFGObjSetOnTop(gfxobj, isOnTop);

	return 0;
}

static int l2bSetElevator(lua_State *L) {
	Uint32 gfxobj = 0;
	bool isElevator = false;
	if(!lua_isnumber(L, 1) ||
	   !lua_isnumber(L, 2)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		gfxobj = (Uint32)lua_tonumber(L, 1);
		if(lua_tonumber(L, 2) > 0) {
			isElevator = true;
		}

	}

	setFGObjSetElevator(gfxobj, isElevator);

	return 0;
}

static int l2bSetKilling(lua_State *L) {
	Uint32 gfxobj = 0;
	bool isKilling = false;
	if(!lua_isnumber(L, 1) ||
	   !lua_isnumber(L, 2)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		gfxobj = (Uint32)lua_tonumber(L, 1);
		if(lua_tonumber(L, 2) > 0) {
			isKilling = true;
		}
	}

	setFGObjSetKilling(gfxobj, isKilling);

	return 0;
}

static int l2bSetPixel(lua_State *L) {
	Uint32 gfxobj = 0;
	bool isPixel = false;
	if(!lua_isnumber(L, 1) ||
	   !lua_isnumber(L, 2)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		gfxobj = (Uint32)lua_tonumber(L, 1);
		if(lua_tonumber(L, 2) > 0) {
			isPixel = true;
		}
	}

	setFGObjSetPixel(gfxobj, isPixel);

	return 0;
}

static int l2bSetGFX(lua_State *L) {
	Uint32 gfxobj, gfx;
	gfxobj = gfx = 0;
	if(!lua_isnumber(L, 1) ||
	   !lua_isnumber(L, 2)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		gfxobj = (Uint32)lua_tonumber(L, 1);
		gfx = (Uint32)lua_tonumber(L, 2);
	}

	setFGObjSetGFX(gfxobj, gfx);

	return 0;
}

static int l2bSetObjPos(lua_State *L) {
	Uint32 gfxobj, x, y;
	gfxobj = x = y = 0;
	if(!lua_isnumber(L, 1) ||
	   !lua_isnumber(L, 2) ||
	   !lua_isnumber(L, 3)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		gfxobj = (Uint32)lua_tonumber(L, 1);
		x = (Sint32)lua_tonumber(L, 2);
		y = (Sint32)lua_tonumber(L, 3);
	}

	setFGObjSetPos(gfxobj, x, y);

	return 0;
}

static int l2bSetPlayerX(lua_State *L) {
	Uint32 x;
	x = 0;
	if(!lua_isnumber(L, 1)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		x = (Uint32)lua_tonumber(L, 1);
	}

	spritex = (double)x;

	return 0;
}

static int l2bGetPlayerX(lua_State *L) {
	Uint32 x = (Uint32)spritex;
	lua_pushnumber(L, x);

	return 1;
}

static int l2bGetPlayerY(lua_State *L) {
	Uint32 y = (Uint32)spritey;
	lua_pushnumber(L, y);

	return 1;
}


static int l2bSetPlayerY(lua_State *L) {
	Uint32 y;
	y = 0;
	if(!lua_isnumber(L, 1)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		y = (Uint32)lua_tonumber(L, 1);
	}

	spritey = (double)y;

	return 0;
}

static int l2bSetLockedBG(lua_State *L) {
	if(!lua_isnumber(L, 1)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		if(lua_tonumber(L, 1) > 0) {
			lockedBG = true;
		} else {
			lockedBG = false;
		}
	}

	return 0;
}

static int l2bSetGravity(lua_State *L) {
	if(!lua_isnumber(L, 1)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		if(lua_tonumber(L, 1) > 0) {
			hasGravity = true;
		} else {
			hasGravity = false;
		}
	}

	return 0;
}

static int l2bSetSpecialTiles(lua_State *L) {
	if(!lua_isnumber(L, 1)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		if(lua_tonumber(L, 1) > 0) {
			canPaintSpecialTiles = true;
		} else {
			canPaintSpecialTiles = false;
		}
	}

	return 0;
}

static int l2bSetFreeMovement(lua_State *L) {
	if(!lua_isnumber(L, 1)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		if(lua_tonumber(L, 1) > 0) {
			freeMovement = true;
		} else {
			freeMovement = false;
		}
	}

	return 0;
}


static int l2bLoadSoundFX(lua_State *L) {
	char tmp[1000];
	if(!lua_isstring(L, 1)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		sprintf(tmp, "%s", lua_tostring(L, 1));
	}

	Uint32 idx = addSoundFXLua(tmp);
	lua_pushnumber(L, idx);

	return 1;
}


static int l2bPlaySoundFX(lua_State *L) {
	Uint32 fx = 0;
	if(!lua_isnumber(L, 1)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		fx = (Uint32)lua_tonumber(L, 1);
	}

	soundPlayFXLua(fx);

	return 0;
}

static int l2bIncrementMaxPixels(lua_State *L) {
	Uint32 pixels = 0;
	if(!lua_isnumber(L, 1)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		pixels = (Uint32)lua_tonumber(L, 1);
	}

	lhandle.numPixels += pixels ;

	return 0;
}

static int l2bAddPixel(lua_State *L) {
	Uint32 x = 0;
	Uint32 y = 0;
	if(!lua_isnumber(L, 1) ||
	   !lua_isnumber(L, 2)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		x = (Uint32)lua_tonumber(L, 1) * TILESIZE;
		y = (Uint32)lua_tonumber(L, 2) * TILESIZE;
	}

	addPixel(x, y, PIXELTYPE_PIXEL);

	return 0;
}

static int l2bAddGFXPixel(lua_State *L) {
	Uint32 gfxobj = 0;
	Uint32 x = 0;
	Uint32 y = 0;
	Uint32 incMaxPixel = 0;

	if(!lua_isnumber(L, 1) ||
	   !lua_isnumber(L, 2) ||
	   !lua_isnumber(L, 3) ||
	   !lua_isnumber(L, 4)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		gfxobj = (Uint32)lua_tonumber(L, 1);
		x = (Uint32)lua_tonumber(L, 2) * TILESIZE;
		y = (Uint32)lua_tonumber(L, 3) * TILESIZE;
		incMaxPixel = (Uint32)lua_tonumber(L, 4);
	}

	Uint32 idx = addFGObj(gfxobj, x, y, false, true, false, false);
	setFGObjSetPixel(idx, true);
	if(incMaxPixel) {
		lhandle.numPixels ++ ;
	}
	lua_pushnumber(L, idx);

	return 1;
}

static int l2bAddBonusLive(lua_State *L) {
	Uint32 x = 0;
	Uint32 y = 0;
	if(!lua_isnumber(L, 1) ||
	   !lua_isnumber(L, 2)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		x = (Uint32)lua_tonumber(L, 1) * TILESIZE;
		y = (Uint32)lua_tonumber(L, 2) * TILESIZE;
	}

	addPixel(x, y, PIXELTYPE_LIVE);

	return 0;
}

static int l2bAddBonusPoints(lua_State *L) {
	Uint32 x = 0;
	Uint32 y = 0;
	if(!lua_isnumber(L, 1) ||
	   !lua_isnumber(L, 2)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		x = (Uint32)lua_tonumber(L, 1) * TILESIZE;
		y = (Uint32)lua_tonumber(L, 2) * TILESIZE;
	}

	addPixel(x, y, PIXELTYPE_POINTS);

	return 0;
}

static int l2bPlayVideo(lua_State *L) {
	// Video playback removed - function kept as no-op for Lua compatibility
	(void)L;
	return 0;
}

static int l2bSetCanExit(lua_State *L) {
	if(!lua_isnumber(L, 1)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		if(lua_tonumber(L, 1) > 0) {
			allowedToExit = true;
		} else {
			allowedToExit = false;
		}
	}

	return 0;
}

static int l2bGetCanExit(lua_State *L) {
	Uint32 yesno = 0;
	if(allowedToExit) {
		yesno = 1;
	}
	lua_pushnumber(L, yesno);

	return 1;
}

static int l2bAddFGTrigger(lua_State *L) {
	Uint32 gfxobj = 0;
	Uint32 x = 0;
	Uint32 y = 0;
	char funcname[100];
	bool isActive = false;
	bool isVisible = false;

	if(!lua_isnumber(L, 1) ||
	   !lua_isnumber(L, 2) ||
	   !lua_isnumber(L, 3) ||
	   !lua_isstring(L, 4) ||
	   !lua_isnumber(L, 5) ||
	   !lua_isnumber(L, 6)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		gfxobj = (Uint32)lua_tonumber(L, 1);
		x = (Uint32)lua_tonumber(L, 2) * TILESIZE;
		y = (Uint32)lua_tonumber(L, 3) * TILESIZE;
		sprintf(funcname, "%s", lua_tostring(L, 4));
		if(lua_tonumber(L, 5) > 0) {
			isActive = true;
		}
		if(lua_tonumber(L, 6) > 0) {
			isVisible = true;
		}
	}

	Uint32 idx = addFGObj(gfxobj, x, y, false, isVisible, false, false);
	registerFGTrigger(idx, isActive, funcname);
	lua_pushnumber(L, idx);

	return 1;
}

static int l2bSetTriggerActive(lua_State *L) {
	Uint32 objid = 0;
	bool isActive = false;
	if(!lua_isnumber(L, 1) ||
	   !lua_isnumber(L, 2)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		if(lua_tonumber(L, 2) > 0) {
			isActive = true;
		}
		objid = (Uint32)lua_tonumber(L, 1);
	}
	setTriggerActive(objid, isActive);

	return 0;
}

static int l2bGetCollectedPixelCount(lua_State *L) {
	lua_pushnumber(L, foundPixels);
	return 1;
}

static int l2bSetCollectedPixelCount(lua_State *L) {
	if(!lua_isnumber(L, 1)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		if(lua_tonumber(L, 1) >= 0) {
			foundPixels = (Uint32)lua_tonumber(L, 1);
		}
	}

	return 0;
}

static int l2bGetRequiredPixelCount(lua_State *L) {
	lua_pushnumber(L, lhandle.numPixels);
	return 1;
}

static int l2bSetRequiredPixelCount(lua_State *L) {
	if(!lua_isnumber(L, 1)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		if(lua_tonumber(L, 1) >= 0) {
			lhandle.numPixels = (Uint32)lua_tonumber(L, 1);
		}
	}

	return 0;
}

static int l2bSetOutputFilter(lua_State *L) {
	if(!lua_isnumber(L, 1)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		if(lua_tonumber(L, 1) >= 0) {
			setOutputFilter((Uint32)lua_tonumber(L, 1));
		}
	}

	return 0;
}

static int l2bSetOutputFilterFading(lua_State *L) {
	if(!lua_isnumber(L, 1)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		if(lua_tonumber(L, 1) >= 0 && lua_tonumber(L, 1) <= 255) {
			setOutputFilterFading((Uint32)lua_tonumber(L, 1));
		}
	}

	return 0;
}


static int l2bKillPlayer(lua_State *L) {
	killPlayer = true;

	// Avoid compiler warning about unused argument (because prototype requires it)
	if(!L) {
        return 0;
    }

	return 0;
}

void blRegisterLuaBindings(lua_State *L) {
	REGISTER(l2bDie, "Die");
	REGISTER(l2bLoadGFX, "LoadGFX");
	REGISTER(l2bAddObject, "AddObject");
	REGISTER(l2bSetVisible, "SetVisible");
	REGISTER(l2bSetBlocking, "SetBlocking");
	REGISTER(l2bSetOnTop, "SetOnTop");
	REGISTER(l2bSetElevator, "SetElevator");
	REGISTER(l2bSetKilling, "SetKilling");
	REGISTER(l2bSetPixel, "SetPixel");
	REGISTER(l2bSetGFX, "SetGFX");
	REGISTER(l2bSetObjPos, "SetObjPos");
	REGISTER(l2bSetPlayerX, "SetPlayerX");
	REGISTER(l2bGetPlayerX, "GetPlayerX");
	REGISTER(l2bSetPlayerY, "SetPlayerY");
	REGISTER(l2bGetPlayerY, "GetPlayerY");
	REGISTER(l2bLoadSoundFX, "LoadSoundFX");
	REGISTER(l2bPlaySoundFX, "PlaySoundFX");
	REGISTER(l2bIncrementMaxPixels, "IncMaxPixels");
	REGISTER(l2bAddPixel, "AddPixel");
	REGISTER(l2bAddGFXPixel, "AddGFXPixel");
	REGISTER(l2bAddBonusLive, "AddBonusLive");
	REGISTER(l2bAddBonusPoints, "AddBonusPoints");
	REGISTER(l2bSetLockedBG, "SetLockedBG");
	REGISTER(l2bSetGravity, "SetGravity");
	REGISTER(l2bSetSpecialTiles, "SetSpecialTiles");
	REGISTER(l2bSetFreeMovement, "SetFreeMovement");
	REGISTER(l2bPlayVideo, "PlayVideo");
	REGISTER(l2bSetCanExit, "SetCanExit");
	REGISTER(l2bGetCanExit, "GetCanExit");
	REGISTER(l2bAddFGTrigger, "AddFGTrigger");
	REGISTER(l2bSetTriggerActive, "SetTriggerActive");
	REGISTER(l2bGetCollectedPixelCount, "GetCollectedPixelCount");
	REGISTER(l2bGetRequiredPixelCount, "GetRequiredPixelCount");
	REGISTER(l2bSetCollectedPixelCount, "SetCollectedPixelCount");
	REGISTER(l2bSetRequiredPixelCount, "SetRequiredPixelCount");
	REGISTER(l2bSetOutputFilter, "SetOutputFilter");
	REGISTER(l2bSetOutputFilterFading, "SetOutputFilterFading");
	REGISTER(l2bKillPlayer, "KillPlayer");


    // Register required GLOBALS
    REGISTERGLOBAL(OUTPUTFILTER_NONE, "OUTPUTFILTER_NONE");
    REGISTERGLOBAL(OUTPUTFILTER_GREYSCALE, "OUTPUTFILTER_GREYSCALE");
    REGISTERGLOBAL(OUTPUTFILTER_COLORINVERT, "OUTPUTFILTER_COLORINVERT");
    REGISTERGLOBAL(OUTPUTFILTER_NOISERECT, "OUTPUTFILTER_NOISERECT");
    REGISTERGLOBAL(OUTPUTFILTER_NOISECIRCLE, "OUTPUTFILTER_NOISECIRCLE");
    REGISTERGLOBAL(OUTPUTFILTER_NOISESTRIPE, "OUTPUTFILTER_NOISESTRIPE");


}

