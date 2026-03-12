// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//

#include "bl_lua_obj_engine.h"

#define NEED_REGISTER_FUNCTION
#include "bl_lua.h"
#undef NEED_REGISTER_FUNCTION

// Kills the engine
int luaObjDie(lua_State *L) {
	if(!lua_isstring(L, 1)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		LUADIE(L, ERROR_LUAL2BDIE, (char *)lua_tostring(L, 1));
	}
	return 0;
}

// Loads an additional file
int luaObjInclude(lua_State *L) {
	Uint32 n = lua_gettop(L);
	if(n != 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjInclude");
	}

	if(!lua_isstring(L, 1)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "");
	} else {
		char fullfname[MAX_FNAME_LENGTH];
		sprintf(fullfname, "%s", configGetPath(lua_tostring(L, 1)));
		if(luaL_loadfile(L, fullfname)) {
			LUADIE(L, ERROR_LUALOAD, (char *)lua_tostring(L, -1));
		}
		if(lua_pcall(L,0,0,0)) {
			LUADIE(L, ERROR_LUAPARSE, (char *)lua_tostring(L, -1));
		}
	}
	return 0;
}

// Plays a video (removed - kept as no-op for Lua compatibility)
int luaObjPlayVideo(lua_State *L) {
	(void)L;
	return 0;
}

