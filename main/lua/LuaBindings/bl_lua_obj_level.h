// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#ifndef BL_LUA_OBJ_LEVEL_H
#define BL_LUA_OBJ_LEVEL_H

#include "bl_lua_objbindings.h"
#include "bl_lua_obj_shared.h"

int luaObjLevelGravity(lua_State *L);
int luaObjLevelFreeMovement(lua_State *L);
int luaObjLevelCanExit(lua_State *L);
int luaObjLevelPaintSpecialTiles(lua_State *L);
int luaObjLevelLockedBG(lua_State *L);
int luaObjLevelPixelsCollected(lua_State *L);
int luaObjLevelPixelsRequired(lua_State *L);
int luaObjLevelTimeTotal(lua_State *L);
int luaObjLevelTimeRemaining(lua_State *L);
int luaObjLevelTimeUsed(lua_State *L);

#endif // BL_LUA_OBJ_LEVEL_H
