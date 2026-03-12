// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//

#include "bl_lua_obj_anim.h"

#define NEED_REGISTER_FUNCTION
#include "bl_lua.h"
#undef NEED_REGISTER_FUNCTION

#include "fgobjects.h"

// Makes a new GFX object
int luaObjLoadAnim(lua_State *L) {
    // addFGObjAnim(const char *templfname, const Uint32 startnum, const Uint32 endnum, const Uint32 fps, const Uint32 looptype);

	char templfname[1000];
	Uint32 startnum, endnum, looptype;
	startnum = endnum = looptype = 0;
	double fps = 0;
	Uint32 n = lua_gettop(L);
	if(n != 5) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjLoadAnim");
	}
	if(!lua_isstring(L, 1)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObjLoadAnim (1)");
	} else {
		sprintf(templfname, "%s", lua_tostring(L, 1));
	}

	if(!lua_isnumber(L, 2)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObjLoadAnim (2)");
	} else {
		startnum = (Uint32)lua_tonumber(L, 2);
	}

	if(!lua_isnumber(L, 3)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObjLoadAnim (3)");
	} else {
		endnum = (Uint32)lua_tonumber(L, 3);
	}

	if(!lua_isnumber(L, 4)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObjLoadAnim (4)");
	} else {
		fps = (double)lua_tonumber(L, 4);
	}

	if(!lua_isnumber(L, 5)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObjLoadAnim (5)");
	} else {
		looptype = (Uint32)lua_tonumber(L, 5);
	}


	Uint32 idx = addFGObjAnim(templfname, startnum, endnum, fps, looptype);
	luaObjPushAnim(L, idx);
	return 1;
}

// Makes a new GFX object
int luaObjDuplicateAnim(lua_State *L) {

	Uint32 sourceIDX = 0;
	Uint32 idx = 0;

	Uint32 n = lua_gettop(L);
	if(n != 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjDuplicateAnim");
	}

	if(!lua_istable(L, 1)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObjDuplicateAnim (arg1)");
	} else {
		sourceIDX = luaObj_getidfromtable (L, 1, "internal");
	}

	idx = duplicateFGObjAnim(sourceIDX);
	luaObjPushAnim(L, idx);
	return 1;
}


// Push anim object on the stack
void luaObjPushAnim(lua_State *L, Uint32 idx) {
	bsl_startobject();
	bsl_addobjectnumbervalue("internal", idx);
	bsl_addobjectstringvalue("type", "anim");
    bsl_addobjectfunction("framerate", luaObjAnimFramerate, idx);
    bsl_addobjectfunction("looptype", luaObjAnimLooptype, idx);
    bsl_addobjectfunction("currentframe", luaObjAnimCurrentFrame, idx);
    bsl_addobjectfunction("currentdirection", luaObjAnimCurrentDirection, idx);

	//Filenames
	// FIXME: This accesses the tables directly instead of using proper functions
	bsl_starttable_local("gfx");
	Uint32 realidx = idx - MAX_FGOBJECTGFX;
	for(Uint32 i = 0; i < fgAnims[realidx]->frameCount; i++) {
        char tmpName[10];
        Uint32 gfxid = fgAnims[realidx]->frames[i].gfxobj;
        sprintf(tmpName, "%d", i);
        bsl_starttable_local(tmpName);
        bsl_addobjectstringvalue("filename", fgGFX[gfxid]->filename);
        bsl_addobjectnumbervalue("internal", gfxid);
        bsl_endtable_local(tmpName);
    }
	bsl_endtable_local("gfx");

	bsl_endobject();
}

// FPS
int luaObjAnimFramerate(lua_State *L) {
	Uint32 objid = 0;
	Uint32 n = lua_gettop(L);
	if(n > 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjAnimFramerate");
	}

	if(!lua_isnumber(L, lua_upvalueindex(1))) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObjAnimFramerate (id)");
	} else {
		objid = (Uint32)lua_tonumber(L, lua_upvalueindex(1));
	}

	double fps = 0.0;
	if(n == 1) {
		if(!lua_isnumber(L, 1)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjAnimFramerate");
		} else {
			fps = (double)lua_tonumber(L, 1);
		}
		setFGObjAnimFrameRate(objid, fps);
	}

	fps = getFGObjAnimFrameRate(objid);
	lua_pushnumber(L, fps);

	return 1;
}

// Looptype
int luaObjAnimLooptype(lua_State *L) {
	Uint32 objid = 0;
	Uint32 n = lua_gettop(L);
	if(n > 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjAnimLooptype");
	}

	if(!lua_isnumber(L, lua_upvalueindex(1))) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObjAnimLooptype (id)");
	} else {
		objid = (Uint32)lua_tonumber(L, lua_upvalueindex(1));
	}

	Uint32 looptype = 0;
	if(n == 1) {
		if(!lua_isnumber(L, 1)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjAnimLooptype");
		} else {
			looptype = (Uint32)lua_tonumber(L, 1);
		}
		setFGObjAnimLoopType(objid, looptype);
	}

	looptype = getFGObjAnimLoopType(objid);
	lua_pushnumber(L, looptype);

	return 1;
}

// CurrentFrame
int luaObjAnimCurrentFrame(lua_State *L) {
	Uint32 objid = 0;
	Uint32 n = lua_gettop(L);
	if(n > 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjAnimCurrentFrame");
	}

	if(!lua_isnumber(L, lua_upvalueindex(1))) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObjAnimCurrentFrame (id)");
	} else {
		objid = (Uint32)lua_tonumber(L, lua_upvalueindex(1));
	}

	Uint32 currentFrame = 0;
	if(n == 1) {
		if(!lua_isnumber(L, 1)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjAnimCurrentFrame");
		} else {
			currentFrame = (Uint32)lua_tonumber(L, 1);
		}
		setFGObjAnimCurrentFrame(objid, currentFrame);
	}

	currentFrame = getFGObjAnimCurrentFrame(objid);
	lua_pushnumber(L, currentFrame);

	return 1;
}

// loop direction
int luaObjAnimCurrentDirection(lua_State *L) {
	Uint32 objid = 0;
	Uint32 n = lua_gettop(L);
	if(n > 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjAnimCurrentDirection");
	}

	if(!lua_isnumber(L, lua_upvalueindex(1))) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObjAnimCurrentDirection (id)");
	} else {
		objid = (Uint32)lua_tonumber(L, lua_upvalueindex(1));
	}

	Sint32 loopDirection = 0;
	if(n == 1) {
		if(!lua_isnumber(L, 1)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjAnimCurrentDirection");
		} else {
			loopDirection = (Sint32)lua_tonumber(L, 1);
		}
		setFGObjAnimCurrentDirection(objid, loopDirection);
	}

	loopDirection = getFGObjAnimCurrentDirection(objid);
	lua_pushnumber(L, loopDirection);

	return 1;
}
