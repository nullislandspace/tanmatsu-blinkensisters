// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//

#include "bl_lua_obj_shared.h"

#define NEED_REGISTER_FUNCTION
#include "bl_lua.h"
#undef NEED_REGISTER_FUNCTION

// Read an OBJID from object/table
Uint32 luaObj_getidfromtable (lua_State *L, Uint32 tableidx, const char *key) {
	Uint32 id;
	lua_pushstring(L, key);
	lua_gettable(L, tableidx);  /* get table[key] */
	if (!lua_isnumber(L, -1)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObj_getidfromtable: Someone messed with the 'internal' field of the object!");
	}
	id = (Uint32)lua_tonumber(L, -1);
	lua_pop(L, 1);  /* remove number */
	return id;
}

