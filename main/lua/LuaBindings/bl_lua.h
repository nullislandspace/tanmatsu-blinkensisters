// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#ifndef BL_LUA_H
#define BL_LUA_H

#include "globals.h"
#include "lua.h"

typedef enum _LUABINDINGTYPE {
    LUABINDINGTYPE_OLDSTYLE = 0,
    LUABINDINGTYPE_OO,
    LUABINDINGTYPE_CONFIG,
    LUABINDINGTYPE_MAX
} LUABINDINGTYPE;

void blLuaInit(char *fname, LUABINDINGTYPE luatype);
void blLuaDeInit();
void blLuaCall(lua_State* blState, const char *func, const char *sig, ...);

// Access global values
bool blLuaGetDouble(lua_State* blState, const char *globalname, double* locDouble);
bool blLuaGetSint32(lua_State* blState, const char *globalname, Sint32* locInt);
bool blLuaGetUint32(lua_State* blState, const char *globalname, Uint32* locInt);
bool blLuaGetString(lua_State* blState, const char *globalname, char* locString);

extern lua_State* scriptIsRunning;
void blLuaDebugHook (lua_State *blState, lua_Debug *ar);
void blLuaPrintBackTrace(lua_State *blState);
extern lua_State *doLuaStackTrace;
void blLuaDieWithError(lua_State* blState, Uint32 errorcode, const char* extrainfo, Uint32 linenum, const char* filename);
#define LUADIE(state, err, txt) blLuaDieWithError(state, err, txt, __LINE__, __FILE__);

#endif // BL_LUA_H


// To simplify registration of the binding function, use macros
#ifdef NEED_REGISTER_FUNCTION
#define REGISTER(func,name) lua_pushcfunction(L, func); \
lua_setglobal(L, name);

#define REGISTERGLOBAL(val, name) lua_pushnumber(L, val); \
lua_setglobal(L, name);

// New Obj-defines

	// ---- TABLES ----
#define bsl_starttable_global(name)		lua_newtable(L);

#define bsl_endtable_global(name)		lua_setglobal(L, name);

#define bsl_starttable_local(name)		lua_pushstring(L, name); \
										lua_newtable(L);

#define bsl_endtable_local(name)		lua_rawset(L, -3);

#define bsl_addfunction(name, funcptr)  lua_pushstring(L, name); \
										lua_pushcfunction(L, funcptr); \
										lua_rawset(L, -3);

#define bsl_addnumbervalue(name, value)	lua_pushstring(L, name); \
										lua_pushnumber(L, value); \
										lua_rawset(L, -3);

#define bsl_addstringvalue(name, value)	lua_pushstring(L, name); \
										lua_pushstring(L, value); \
										lua_rawset(L, -3);


	// ---- OBJECTS ----
#define bsl_startobject()				lua_newtable(L);

#define bsl_endobject()

#define bsl_addobjectfunction(name, funcptr, id)	lua_pushstring(L, name); \
													lua_pushnumber(L, id); \
													lua_pushcclosure(L, funcptr, 1); \
													lua_rawset(L, -3);


#define bsl_addobjectnumberfunction(name, funcptr, id, value)	lua_pushstring(L, name); \
																lua_pushnumber(L, id); \
																lua_pushnumber(L, value); \
																lua_pushcclosure(L, funcptr, 2); \
																lua_rawset(L, -3);


#define bsl_addobjectstringfunction(name, funcptr, id, txt)	lua_pushstring(L, name); \
															lua_pushnumber(L, id); \
															lua_pushstring(L, txt); \
															lua_pushcclosure(L, funcptr, 2); \
															lua_rawset(L, -3);

#define bsl_addobjectnumbervalue(name, value)		lua_pushstring(L, name); \
													lua_pushnumber(L, value); \
													lua_rawset(L, -3);

#define bsl_addobjectstringvalue(name, value)		lua_pushstring(L, name); \
													lua_pushstring(L, value); \
													lua_rawset(L, -3);

#endif // NEED_REGISTER_FUNCTION
