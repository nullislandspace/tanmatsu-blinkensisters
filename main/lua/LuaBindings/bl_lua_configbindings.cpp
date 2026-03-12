// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-10 The Blinkensisters Team
//
// See License.txt for licensing information
//

#define NEED_REGISTER_FUNCTION
#include "bl_lua.h"
#undef NEED_REGISTER_FUNCTION

#include "bl_lua_configbindings.h"


// ++++++++++++++++   REGISTERING GLOBALS IN LUA 	+++++++++++++++++

void blRegisterLuaConfigBindings(lua_State *L) {
	// Register globals with default values

	bsl_addstringvalue("bg", "");
	bsl_addstringvalue("bg2", "");
	bsl_addstringvalue("leveldata", "");
	bsl_addstringvalue("magictile", "");
	bsl_addstringvalue("ooscript", "");
	bsl_addstringvalue("script", "");
	bsl_addstringvalue("snd", "");
	bsl_addstringvalue("tiles", "");

	bsl_addnumbervalue("leveltime", 0);
	bsl_addnumbervalue("timebonuspixel", 0);
	bsl_addnumbervalue("timebonusmultiplicator", 0);
	bsl_addnumbervalue("timemalusifkatedies", 0);
}

void blGetLuaConfigValues(lua_State *L) {
       /*
       bool blLuaGetDouble(lua_State* blState, const char *globalname, double* locDouble);
bool blLuaGetSint32(lua_State* blState, const char *globalname, Sint32* locInt);
bool blLuaGetUint32(lua_State* blState, const char *globalname, Uint32* locInt);
bool blLuaGetString(lua_State* blState, const char *globalname, char* locString);
*/
	blLuaGetString(L, "bg", &lhandle.bgfile[0]);
	blLuaGetString(L, "bg2", &lhandle.bg2file[0]);
	blLuaGetString(L, "leveldata", &lhandle.leveldata[0]);
	blLuaGetString(L, "magictile", &lhandle.magictile[0]);
	blLuaGetString(L, "ooscript", &lhandle.ooscriptfile[0]);
	blLuaGetString(L, "script", &lhandle.scriptfile[0]);
	blLuaGetString(L, "snd", &lhandle.sndfile[0]);
	blLuaGetString(L, "tiles", &lhandle.tilesfile[0]);

	blLuaGetUint32(L, "leveltime", &lhandle.leveltime);
	blLuaGetUint32(L, "timebonuspixel", &lhandle.timebonuspixel);
	blLuaGetUint32(L, "timebonusmultiplicator", &lhandle.timebonusmultiplicator);
	blLuaGetUint32(L, "timemalusifkatedies", &lhandle.timemalusifkatedies);
}
