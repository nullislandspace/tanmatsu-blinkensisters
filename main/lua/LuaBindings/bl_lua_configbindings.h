// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#ifndef BL_LUA_CONFIGBINDINGS_H
#define BL_LUA_CONFIGBINDINGS_H

#include "globals.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "errorhandler.h"
#include "levelhandler.h"

void blRegisterLuaConfigBindings(lua_State *L);
void blGetLuaConfigValues(lua_State *L);

#endif // BL_LUA_CONFIGBINDINGS_H
