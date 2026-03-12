// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//

#include "bl_lua_obj_sound.h"

#define NEED_REGISTER_FUNCTION
#include "bl_lua.h"
#undef NEED_REGISTER_FUNCTION

// Generates a new FX Sound object
int luaObjLoadSoundFX(lua_State *L) {
	char tmp[1000];
	Uint32 n = lua_gettop(L);
	if(n != 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjLoadSoundFX");
	}
	if(!lua_isstring(L, 1)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObjLoadSoundFX");
	} else {
		sprintf(tmp, "%s", lua_tostring(L, 1));
	}

	Uint32 idx = addSoundFXLua(tmp);
	idx = idx;
	bsl_startobject();
	bsl_addobjectfunction("play", luaObjPlaySoundFX, idx);
	bsl_addobjectstringvalue("filename", tmp);
	bsl_addobjectnumbervalue("internal", idx);
	bsl_endobject();

	return 1;
}

// Plays a FX Sound object
int luaObjPlaySoundFX(lua_State *L) {
	Uint32 fx = 0;
	if(!lua_isnumber(L, lua_upvalueindex(1))) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObjPlaySoundFX");
	} else {
		fx = (Uint32)lua_tonumber(L, lua_upvalueindex(1));
	}

	soundPlayFXLua(fx);

	return 0;
}
