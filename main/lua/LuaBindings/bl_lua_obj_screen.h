// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#ifndef BL_LUA_OBJ_SCREEN_H
#define BL_LUA_OBJ_SCREEN_H

#include "bl_lua_objbindings.h"
#include "bl_lua_obj_shared.h"

int luaObjResolutionX(lua_State *L);
int luaObjResolutionY(lua_State *L);
int luaObjResolutionTilesize(lua_State *L);

int luaObjScreenFilterProperty(lua_State *L);
int luaObjScreenFilterFading(lua_State *L);

#endif // BL_LUA_OBJ_SCREEN_H
