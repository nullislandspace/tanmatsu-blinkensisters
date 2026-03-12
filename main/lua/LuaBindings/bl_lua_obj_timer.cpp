// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//

#include "bl_lua_obj_timer.h"

#define NEED_REGISTER_FUNCTION
#include "bl_lua.h"
#undef NEED_REGISTER_FUNCTION

#include "triggers.h"

// Generates a new FX Sound object
int luaObjTimerNew(lua_State *L) {
	char funcname[1000];
	double interval = 0.0;
	Uint32 n = lua_gettop(L);
	if(n != 2) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjTimerNew");
	}

	if(!lua_isstring(L, 1)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObjTimerNew (1)");
	} else {
		sprintf(funcname, "%s", lua_tostring(L, 1));
	}

	if(!lua_isnumber(L, 2)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObjTimerNew (2)");
	} else {
		interval = (double)lua_tonumber(L, 2);
	}

	Uint32 idx = registerTimerTrigger(funcname, interval);
	luaObjPushTimer(L, idx);
	return 1;
}

void luaObjPushTimer(lua_State *L, Uint32 idx) {
	bsl_startobject();
	bsl_addobjectfunction("active", luaObjTimerActive, idx);
	bsl_addobjectfunction("interval", luaObjTimerActive, idx);
	bsl_addobjectnumbervalue("internal", idx);
	bsl_endobject();
}

// Sets the active property
int luaObjTimerActive(lua_State *L) {
	Uint32 objid = 0;
	Uint32 n = lua_gettop(L);
	if(n > 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjTimerActive");
	}

	if(!lua_isnumber(L, lua_upvalueindex(1))) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObjTimerActive (id)");
	} else {
		objid = (Uint32)lua_tonumber(L, lua_upvalueindex(1));
	}

	bool isActive = false;
	if(n == 1) {
		if(!lua_isnumber(L, 1)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjTimerActive");
		} else {
			if(lua_tonumber(L, 1) == 0) {
				isActive = false;
			} else if(lua_tonumber(L, 1) == 1){
				isActive = true;
			} else {
				LUADIE(L, ERROR_LUAL2BRANGE, "luaObjTimerActive");
			}
		}
		setTimerTriggerActive(objid, isActive);
	}

	if(getTimerTriggerActive(objid)) {
		lua_pushnumber(L, 1);
	} else {
		lua_pushnumber(L, 0);
	}

	return 1;
}

// sets the timer interval
int luaObjTimerInterval(lua_State *L) {
	Uint32 objid = 0;
	Uint32 n = lua_gettop(L);
	if(n > 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjTimerActive");
	}

	if(!lua_isnumber(L, lua_upvalueindex(1))) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObjTimerActive (id)");
	} else {
		objid = (Uint32)lua_tonumber(L, lua_upvalueindex(1));
	}

	double interval = 0.0;
	if(n == 1) {
		if(!lua_isnumber(L, 1)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjTimerActive");
		} else {
			interval = (double)lua_tonumber(L, 1);
		}
		setTimerTriggerInterval(objid, interval);
	}

	interval = getTimerTriggerInterval(objid);
	lua_pushnumber(L, interval);

	return 1;
}

