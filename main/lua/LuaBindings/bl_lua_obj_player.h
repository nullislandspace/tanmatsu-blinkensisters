// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#ifndef BL_LUA_OBJ_PLAYER_H
#define BL_LUA_OBJ_PLAYER_H

#include "bl_lua_objbindings.h"
#include "bl_lua_obj_shared.h"

int luaObjPlayerX(lua_State *L);
int luaObjPlayerY(lua_State *L);
int luaObjPlayerVX(lua_State *L);
int luaObjPlayerVY(lua_State *L);
int luaObjKillPlayer(lua_State *L);
int luaObjPlayerScore(lua_State *L);
int luaObjPlayerLives(lua_State *L);
int luaObjPlayerGFX(lua_State *L);

#endif // BL_LUA_OBJ_PLAYER_H
