// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//

#include "bl_lua_obj_level.h"

#define NEED_REGISTER_FUNCTION
#include "bl_lua.h"
#undef NEED_REGISTER_FUNCTION

// Gravity on/off
int luaObjLevelGravity(lua_State *L) {
	Uint32 n = lua_gettop(L);
	if(n > 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjLevelGravity");
	}
	Uint32 gravity = 0;
	if(n == 1) {
		if(!lua_isnumber(L, 1)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjLevelGravity");
		} else {
			if(lua_tonumber(L, 1) == 0) {
				hasGravity = false;
			} else if(lua_tonumber(L, 1) == 1){
				hasGravity = true;
			} else {
				LUADIE(L, ERROR_LUAL2BRANGE, "luaObjLevelGravity");
			}
		}
	}

	if(hasGravity) {
		gravity = 1;
	}
	lua_pushnumber(L, gravity);

	return 1;
}

// Free movement mode on/off
int luaObjLevelFreeMovement(lua_State *L) {
	Uint32 n = lua_gettop(L);
	if(n > 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjLevelFreeMovement");
	}
	Uint32 fm = 0;
	if(n == 1) {
		if(!lua_isnumber(L, 1)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjLevelFreeMovement");
		} else {
			if(lua_tonumber(L, 1) == 0) {
				freeMovement = false;
			} else if(lua_tonumber(L, 1) == 1){
				freeMovement = true;
			} else {
				LUADIE(L, ERROR_LUAL2BRANGE, "luaObjLevelFreeMovement");
			}
		}
	}

	if(freeMovement) {
		fm = 1;
	}
	lua_pushnumber(L, fm);

	return 1;
}

// exit allowed on/off
int luaObjLevelCanExit(lua_State *L) {
	Uint32 n = lua_gettop(L);
	if(n > 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjLevelCanExit");
	}
	Uint32 canexit = 0;
	if(n == 1) {
		if(!lua_isnumber(L, 1)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjLevelCanExit");
		} else {
			if(lua_tonumber(L, 1) == 0) {
				allowedToExit = false;
			} else if(lua_tonumber(L, 1) == 1){
				allowedToExit = true;
			} else {
				LUADIE(L, ERROR_LUAL2BRANGE, "luaObjLevelCanExit");
			}
		}
	}

	if(allowedToExit) {
		canexit = 1;
	}
	lua_pushnumber(L, canexit);

	return 1;
}

// Paint special tiles on/off
int luaObjLevelPaintSpecialTiles(lua_State *L) {
	Uint32 n = lua_gettop(L);
	if(n > 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjLevelPaintSpecialTiles");
	}
	Uint32 paint = 0;
	if(n == 1) {
		if(!lua_isnumber(L, 1)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjLevelPaintSpecialTiles");
		} else {
			if(lua_tonumber(L, 1) == 0) {
				canPaintSpecialTiles = false;
			} else if(lua_tonumber(L, 1) == 1){
				canPaintSpecialTiles = true;
			} else {
				LUADIE(L, ERROR_LUAL2BRANGE, "luaObjLevelPaintSpecialTiles");
			}
		}
	}

	if(canPaintSpecialTiles) {
		paint = 1;
	}
	lua_pushnumber(L, paint);

	return 1;
}

// disable parallax scrolling on/off
int luaObjLevelLockedBG(lua_State *L) {
	Uint32 n = lua_gettop(L);
	if(n > 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjLevelLockedBG");
	}
	Uint32 locked = 0;
	if(n == 1) {
		if(!lua_isnumber(L, 1)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjLevelLockedBG");
		} else {
			if(lua_tonumber(L, 1) == 0) {
				lockedBG = false;
			} else if(lua_tonumber(L, 1) == 1){
				lockedBG = true;
			} else {
				LUADIE(L, ERROR_LUAL2BRANGE, "luaObjLevelLockedBG");
			}
		}
	}

	if(lockedBG) {
		locked = 1;
	}
	lua_pushnumber(L, locked);

	return 1;
}

// Number of collected pixels
int luaObjLevelPixelsCollected(lua_State *L) {
	Uint32 n = lua_gettop(L);
	if(n > 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjLevelPixelsCollected");
	}
	Uint32 pixels = 0;
	if(n == 1) {
		if(!lua_isnumber(L, 1)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjLevelPixelsCollected");
		} else {
			foundPixels = (Uint32)lua_tonumber(L, 1);
		}
	}

	pixels = foundPixels;
	lua_pushnumber(L, pixels);

	return 1;
}

// Number of required Pixels
int luaObjLevelPixelsRequired(lua_State *L) {
	Uint32 n = lua_gettop(L);
	if(n > 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjLevelPixelsRequired");
	}
	Uint32 pixels = 0;
	if(n == 1) {
		if(!lua_isnumber(L, 1)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjLevelPixelsRequired");
		} else {
			lhandle.numPixels = (Uint32)lua_tonumber(L, 1);
		}
	}

	pixels = lhandle.numPixels;
	lua_pushnumber(L, pixels);

	return 1;
}

// level total time
int luaObjLevelTimeTotal(lua_State *L) {

	Uint32 n = lua_gettop(L);
	if(n > 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjLevelTimeTotal");
	}

	if(n == 1) {
		if(!lua_isnumber(L, 1)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjLevelTimeTotal");
		} else {
			lhandle.leveltime = (Uint32)lua_tonumber(L, 1);
		}
	}

	lua_pushnumber(L, lhandle.leveltime);

	return 1;
}

// Level Remaining time
int luaObjLevelTimeRemaining(lua_State *L) {

	Uint32 n = lua_gettop(L);
	if(n > 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjLevelTimeTotal");
	}

	if(n == 1) {
		if(!lua_isnumber(L, 1)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjLevelTimeTotal");
		} else {
			lhandle.remaintime = (double)lua_tonumber(L, 1);
		}
	}

	lua_pushnumber(L, lhandle.remaintime);

	return 1;
}

// Level time used
int luaObjLevelTimeUsed(lua_State *L) {

	Uint32 n = lua_gettop(L);
	if(n > 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjLevelTimeTotal");
	}

	if(n == 1) {
		if(!lua_isnumber(L, 1)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjLevelTimeTotal");
		} else {
			lhandle.remaintime = (double)lhandle.leveltime - (double)lua_tonumber(L, 1);
		}
	}

	lua_pushnumber(L, (double)lhandle.leveltime - lhandle.remaintime);

	return 1;
}
