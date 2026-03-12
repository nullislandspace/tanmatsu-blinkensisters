// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//

#include "bl_lua_obj_screen.h"

#define NEED_REGISTER_FUNCTION
#include "bl_lua.h"
#undef NEED_REGISTER_FUNCTION

// Screen WIDTH
int luaObjResolutionX(lua_State *L) {
	Uint32 n = lua_gettop(L);
	if(n != 0) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjResolutionX");
	}

	lua_pushnumber(L, SCR_WIDTH);
	return 1;
}

// Screen HEIGHT
int luaObjResolutionY(lua_State *L) {
	Uint32 n = lua_gettop(L);
	if(n != 0) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjResolutionY");
	}

	lua_pushnumber(L, SCR_HEIGHT);
	return 1;
}

// TILESIZE
int luaObjResolutionTilesize(lua_State *L) {
	Uint32 n = lua_gettop(L);
	if(n != 0) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjResolutionTilesize");
	}

	lua_pushnumber(L, TILESIZE);
	return 1;
}

// Filter types ("properties")
int luaObjScreenFilterProperty(lua_State *L) {
	Uint32 filterType = 0;
	Uint32 n = lua_gettop(L);
	if(n > 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjScreenFilterProperty");
	}

	if(!lua_isnumber(L, lua_upvalueindex(1))) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObjScreenFilterProperty (val)");
	} else {
		filterType = (Uint32)lua_tonumber(L, lua_upvalueindex(1));
	}

	bool isSet = false;
	if(n == 1) {
		if(!lua_isnumber(L, 1)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjScreenFilterProperty");
		} else {
			if(lua_tonumber(L, 1) == 0) {
				isSet = false;
			} else if(lua_tonumber(L, 1) == 1){
				isSet = true;
			} else {
				LUADIE(L, ERROR_LUAL2BRANGE, "luaObjScreenFilterProperty");
			}
		}
		setSpecificOutputFilter(filterType, isSet);
	}

	isSet = getSpecificOutputFilter(filterType);
	if(isSet) {
	   lua_pushnumber(L, 1);
    } else {
        lua_pushnumber(L, 0);
    }

	return 1;
}

// Filter / screen fading
int luaObjScreenFilterFading(lua_State *L) {
	Uint32 n = lua_gettop(L);
	if(n > 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjScreenFilterFading");
	}

	Uint32 fading = 255;
	if(n == 1) {
		if(!lua_isnumber(L, 1)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjScreenFilterFading");
		} else {
            fading = (Uint32)lua_tonumber(L, 1);
		}
		setOutputFilterFading(fading);
	}

	fading = getOutputFilterFading();
    lua_pushnumber(L, fading);

	return 1;
}
