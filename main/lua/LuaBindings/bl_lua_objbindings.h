// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#ifndef BL_LUA_OBJBINDINGS_H
#define BL_LUA_OBJBINDINGS_H

#include "globals.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "errorhandler.h"
#include "sound.h"
#include "levelhandler.h"
#include "engine.h"
#include "triggers.h"
#include "outputfilter.h"
#include "fgobjects.h"
#include "gameengine.h"

#include <string.h>

void blRegisterLuaObjBindings(lua_State *L);

extern bool allowLUAPaint;

#endif // BL_LUA_OBJBINDINGS_H
