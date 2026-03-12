// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//

#include "bl_lua_obj_gfx.h"

#define NEED_REGISTER_FUNCTION
#include "bl_lua.h"
#undef NEED_REGISTER_FUNCTION


// Makes a new GFX object
int luaObjLoadGraphic(lua_State *L) {
	char tmp[1000];
	Uint32 n = lua_gettop(L);
	if(n != 1) {
        sprintf(tmp, "luaObjLoadGraphic: n = %d", n);
		LUADIE(L, ERROR_LUAARGCOUNT, tmp);
	}
	if(!lua_isstring(L, 1)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObjLoadGraphic");
	} else {
		sprintf(tmp, "%s", lua_tostring(L, 1));
	}

	Uint32 idx = addFGObjGFX(tmp);
	luaObjPushGraphic(L, idx);

	return 1;
}

void luaObjPushGraphic(lua_State *L, Uint32 idx) {
	bsl_startobject();
	bsl_addobjectstringvalue("filename", fgGFX[idx]->filename);
	bsl_addobjectnumbervalue("internal", idx);
	bsl_addobjectstringvalue("type", "gfx");
	bsl_endobject();
}

