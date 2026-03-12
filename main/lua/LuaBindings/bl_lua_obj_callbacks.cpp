// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//

#include "bl_lua_obj_callbacks.h"

#define NEED_REGISTER_FUNCTION
#include "bl_lua.h"
#undef NEED_REGISTER_FUNCTION

// Registers a LUA Callback

int luaObjRegisterCallback(lua_State *L) {
	Uint32 cbid = 0;

	Uint32 n = lua_gettop(L);
	if(n > 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjRegisterCallback");
	}

	if(!lua_isnumber(L, lua_upvalueindex(1))) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObjRegisterCallback (id)");
	} else {
		cbid = (Uint32)lua_tonumber(L, lua_upvalueindex(1));
	}

	if(cbid >= CB_LAST) {
		LUADIE(L, ERROR_LUAL2BRANGE, "luaObjRegisterCallback");
	}

	if(n == 1) {
		if(!lua_isstring(L, 1)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjRegisterCallback");
		} else {
			sprintf(lhandle.luaCB[cbid].cbName, "%s", lua_tostring(L, 1));
		}
	}
	lua_pushstring(L, lhandle.luaCB[cbid].cbName);
	return 1;
}
