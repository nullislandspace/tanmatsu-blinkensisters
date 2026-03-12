// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#ifndef BL_LUA_OBJ_FGOBJECTS_H
#define BL_LUA_OBJ_FGOBJECTS_H

#include "bl_lua_objbindings.h"
#include "bl_lua_obj_shared.h"

int luaObjObjectProperty(lua_State *L);
int luaObjObjectX(lua_State *L);
int luaObjObjectY(lua_State *L);
int luaObjObjectGFX(lua_State *L);
int luaObjNewFGObject(lua_State *L);
int luaObjRegisterObjectCallback(lua_State *L);
void luaObjPushFGObject(lua_State *L, Uint32 objid);
void luaObjPushFGOverlapObject(lua_State *L);

#endif // BL_LUA_OBJ_FGOBJECTS_H
