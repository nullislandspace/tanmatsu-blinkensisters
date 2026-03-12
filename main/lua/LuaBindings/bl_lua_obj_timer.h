// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#ifndef BL_LUA_OBJ_TIMER_H
#define BL_LUA_OBJ_TIMER_H

#include "bl_lua_objbindings.h"
#include "bl_lua_obj_shared.h"

int luaObjTimerNew(lua_State *L);
void luaObjPushTimer(lua_State *L, Uint32 idx);
int luaObjTimerActive(lua_State *L);
int luaObjTimerInterval(lua_State *L);

#endif // BL_LUA_OBJ_TIMER_H
