// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#ifndef BL_LUA_OBJ_ANIM_H
#define BL_LUA_OBJ_ANIM_H

#include "bl_lua_objbindings.h"
#include "bl_lua_obj_shared.h"

int luaObjLoadAnim(lua_State *L);
int luaObjDuplicateAnim(lua_State *L);
void luaObjPushAnim(lua_State *L, Uint32 idx);
int luaObjAnimFramerate(lua_State *L);
int luaObjAnimLooptype(lua_State *L);
int luaObjAnimCurrentFrame(lua_State *L);
int luaObjAnimCurrentDirection(lua_State *L);

#endif // BL_LUA_OBJ_ANIM_H
