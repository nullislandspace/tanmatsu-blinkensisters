// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#include "globals.h"
#define NEED_REGISTER_FUNCTION
#include "bl_lua.h"
#undef NEED_REGISTER_FUNCTION
#include "errorhandler.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "bl_lua_bindings.h"
#include "bl_lua_objbindings.h"
#include "bl_lua_configbindings.h"
#include "bl_lua_obj_fgobjects.h"
#include "bl_lua_obj_timer.h"
#include "bl_lua_obj_gfx.h"
#include "bl_lua_obj_anim.h"

#include <stdarg.h>
#include <string.h>

lua_State* scriptIsRunning;

#define debugLineStackSize 10
lua_Debug debugLineStack[LUABINDINGTYPE_MAX][debugLineStackSize];
Uint32 debugLineStackPtr[LUABINDINGTYPE_MAX];
Uint32 debugLineStackCount[LUABINDINGTYPE_MAX];
lua_State *debugableScript[LUABINDINGTYPE_MAX];
lua_State *doLuaStackTrace = 0;


void blLuaInit(char *fname, LUABINDINGTYPE luatype)
{
	lua_State* blLUA;
	char fullfname[MAX_FNAME_LENGTH];
	sprintf(fullfname, "%s", configGetPath(fname));

	// Initialize LUA
	blLUA = lua_open();
	if(!blLUA) {
		DIE(ERROR_LUAINIT, "");
	}

	// Load libraries
	luaopen_base(blLUA);
	luaopen_io(blLUA);
	luaopen_string(blLUA);
	luaopen_math(blLUA);

	// Init debugging
    debugLineStackPtr[luatype] = 0;
	debugLineStackCount[luatype] = 0;
	debugableScript[luatype] = blLUA;
	lua_sethook(blLUA, blLuaDebugHook, LUA_MASKLINE, 0);

	if(luatype == LUABINDINGTYPE_OLDSTYLE) {
		blRegisterLuaBindings(blLUA);
	} else if(luatype == LUABINDINGTYPE_OO) {
		blRegisterLuaObjBindings(blLUA);
	} else if(luatype == LUABINDINGTYPE_CONFIG) {
		blRegisterLuaConfigBindings(blLUA);
	}

	scriptIsRunning = blLUA;
	// Load script file
	if(luaL_loadfile(blLUA, fullfname)) {
		DIE(ERROR_LUALOAD, (char *)lua_tostring(blLUA, -1));
	}

	// Parse script file (for globals and such)
	if(lua_pcall(blLUA,0,0,0)) {
		DIE(ERROR_LUAPARSE, (char *)lua_tostring(blLUA, -1));
	}

	if(luatype == LUABINDINGTYPE_OLDSTYLE) {
		lhandle.blLuaState = blLUA;
	} else if(luatype == LUABINDINGTYPE_OO) {
		lhandle.blOOLuaState = blLUA;
	} else if(luatype == LUABINDINGTYPE_CONFIG) {
	    lhandle.blConfigLuaState = blLUA;
	    // Get the setting back in one go
	    blGetLuaConfigValues(blLUA);
	}

	scriptIsRunning = 0;
}

void blLuaDeInit()
{
	if(lhandle.blLuaState) {
		lua_close(lhandle.blLuaState);
		lhandle.blLuaState = NULL;
	}
	if(lhandle.blOOLuaState) {
		lua_close(lhandle.blOOLuaState);
		lhandle.blOOLuaState = NULL;
	}
	if(lhandle.blConfigLuaState) {
		lua_close(lhandle.blConfigLuaState);
		lhandle.blConfigLuaState = NULL;
	}
}

void blLuaCall(lua_State* blState, const char *func, const char *sig, ...)
{
	if(!blState) {
		DIE(ERROR_LUANOINIT, "");
	}

	va_list vl;
	int narg, nres, npopres;

	va_start(vl, sig);
	lua_getglobal(blState, func);

	// push arguments
	narg = 0;
	bool deli = false;
	while(*sig && !deli) {
		switch(*sig) {
			case 'd': // Double
				lua_pushnumber(blState, va_arg(vl, double));
				narg++;
				break;
			case 'i': // Int
				lua_pushnumber(blState, va_arg(vl, int));
				narg++;
				break;
			case 's': // String
				lua_pushstring(blState, va_arg(vl, char *));
				narg++;
				break;
			case 'F': // OOLUA FG Object id (push the whole object
				luaObjPushFGObject(blState, (Uint32)va_arg(vl, Uint32));
				narg++;
				break;
			case 'C': // OOLUA Colission / fgOverlap table (must be pre-filled from last col. check)
				luaObjPushFGOverlapObject(blState);
				narg++;
				break;
			case 'T': // OOLUA TIMER Object id (push the whole object
				luaObjPushTimer(blState, (Uint32)va_arg(vl, Uint32));
				narg++;
				break;
			case 'G': // OOLUA GFX Object id (push the whole object
				luaObjPushGraphic(blState, (Uint32)va_arg(vl, Uint32));
				narg++;
				break;
			case 'A': // OOLUA GFX Object id (push the whole object
				luaObjPushAnim(blState, (Uint32)va_arg(vl, Uint32));
				narg++;
				break;
			case '*': // Pre-pushed object for OO Lua-Bindings, must at the start of the string!
				narg++;
				break;
			case '>': // Delimeter for results
				deli = true;
				break;
			default:
				char tmp[20];
				sprintf(tmp, "FORMAT: (%c)", *sig);
				DIE(ERROR_LUAPARAMFORMAT, tmp);
		}
		luaL_checkstack(blState, 1, "too many arguments");
		sig++;
	}

	// call the function
	scriptIsRunning = blState;
	npopres = nres = strlen(sig); // number of expected results
	if(lua_pcall(blState, narg, nres, 0)) {
		char tmp[200];
		doLuaStackTrace = blState;
		sprintf(tmp, "Function: '%s'\n%s", func, lua_tostring(blState, -1));
		DIE(ERROR_LUACALL, tmp);
	}
	scriptIsRunning = 0;

	// Get results
	nres = -nres; // Stack index of first result
	while(*sig) {
		switch(*sig) {
			case 'd': // double
				if(!lua_isnumber(blState, nres)) {
					DIE(ERROR_LUARESULTTYPE, func);
				}
				*va_arg(vl, double *) = lua_tonumber(blState, nres);
				break;
			case 'i': // int
				if(!lua_isnumber(blState, nres)) {
					DIE(ERROR_LUARESULTTYPE, func);
				}
				*va_arg(vl, int *) = (int)lua_tonumber(blState, nres);
				break;
			case 's': // string
				if(!lua_isstring(blState, nres)) {
					DIE(ERROR_LUARESULTTYPE, func);
				}
				sprintf(*va_arg(vl, char **), "%s", lua_tostring(blState, nres));
				break;
			default:
				char tmp[20];
				sprintf(tmp, "FORMAT: (%c)", *sig);
				DIE(ERROR_LUAPARAMFORMAT, tmp);
		}
		sig++;
	}

	va_end(vl);

	while(npopres) {
		lua_pop(blState, 1);
		npopres--;
	}
}

bool blLuaGetDouble(lua_State* blState, const char *globalname, double* locDouble) {
    lua_getglobal(blState, globalname);
    if(lua_isnil(blState, -1)) {
        // Global not set or something
        lua_pop(blState, 1);
        return false;
    }

    if(lua_isnumber(blState, -1)) {
        *locDouble = (double)lua_tonumber(blState, -1);
        lua_pop(blState, 1);
    } else {
        LUADIE(blState, ERROR_LUAL2BGLOBALTYPE, globalname);
    }

    return true;
}

bool blLuaGetSint32(lua_State* blState, const char *globalname, Sint32* locInt) {
    lua_getglobal(blState, globalname);
    if(lua_isnil(blState, -1)) {
        // Global not set or something
        lua_pop(blState, 1);
        return false;
    }

    if(lua_isnumber(blState, -1)) {
        *locInt = (Sint32)lua_tonumber(blState, -1);
        lua_pop(blState, 1);
    } else {
        LUADIE(blState, ERROR_LUAL2BGLOBALTYPE, globalname);
    }

    return true;
}

bool blLuaGetUint32(lua_State* blState, const char *globalname, Uint32* locInt) {
    lua_getglobal(blState, globalname);
    if(lua_isnil(blState, -1)) {
        // Global not set or something
        lua_pop(blState, 1);
        return false;
    }

    if(lua_isnumber(blState, -1)) {
        *locInt = (Uint32)lua_tonumber(blState, -1);
        lua_pop(blState, 1);
    } else {
        LUADIE(blState, ERROR_LUAL2BGLOBALTYPE, globalname);
    }

    return true;
}

bool blLuaGetString(lua_State* blState, const char *globalname, char* locString) {
    lua_getglobal(blState, globalname);
    if(lua_isnil(blState, -1)) {
        // Global not set or something
        lua_pop(blState, 1);
        return false;
    }

    if(lua_isstring(blState, -1)) {
        sprintf(locString, "%s", lua_tostring(blState, -1));
        lua_pop(blState, 1);
    } else {
        LUADIE(blState, ERROR_LUAL2BGLOBALTYPE, globalname);
    }

    return true;
}

void blLuaDieWithError(lua_State *blState, Uint32 errorcode, const char* extrainfo, Uint32 linenum, const char* filename) {
	//	fprintf(stderr, "Error %d: %s\nError-Info: %s\nError at %s:%d\n",
	//errorcode, getErrorText(errorcode), extrainfo, filename, linenum);

    //blLuaPrintBackTrace(blState);

	char errorforlua[2000];
	sprintf(errorforlua, "\nCall-Error %d: %s\nCall-Error additional Info: %s\n   at %s:%d",
			errorcode, getErrorText(errorcode), extrainfo, filename, linenum);
	luaL_error(scriptIsRunning, errorforlua);

}

void blLuaDebugHook (lua_State *blState, lua_Debug *ar) {
    Uint32 type = 0;
    for(Uint32 i=0; i < LUABINDINGTYPE_MAX; i++) {
        if(debugableScript[i] == blState) {
            type = i;
        }
    }

    if(ar->event == LUA_HOOKLINE) {
        if(debugLineStackCount[type] < debugLineStackSize) {
            debugLineStackCount[type]++;
        }

        lua_getinfo(blState, "nSlufL", ar);
        debugLineStack[type][debugLineStackPtr[type]] = *ar;
        //memcpy((void *)&debugLineStack[type][debugLineStackPtr[type]], ar, sizeof(lua_Debug));
        debugLineStackPtr[type]++;
        if(debugLineStackPtr[type] == debugLineStackSize) {
            debugLineStackPtr[type] = 0;
        }

    }
}

void blLuaPrintBackTrace(lua_State *blState) {
    Uint32 type = 0;
    for(Uint32 i=0; i < LUABINDINGTYPE_MAX; i++) {
        if(debugableScript[i] == blState) {
            type = i;
        }
    }

    fprintf(stderr, "\n\n *** START OF LUA BACKTRACE ***\n\n");
    fprintf(stderr, "Line Trace:\n");
    Uint32 cnt = 0;
    Uint32 ptr = debugLineStackPtr[type];
    if(ptr == 0) {
        ptr = debugLineStackSize - 1;
    } else {
        ptr--;
    }
    while(cnt < debugLineStackCount[type]) {
        fprintf(stderr, "%s::%i %s::%s\n", debugLineStack[type][ptr].source,
                                            debugLineStack[type][ptr].currentline,
                                            debugLineStack[type][ptr].namewhat,
                                            debugLineStack[type][ptr].name);
        cnt++;
        ptr++;
        if(ptr == debugLineStackCount[type]) {
            ptr=0;
        }
    }

    fprintf(stderr, "\n\n *** END OF LUA BACKTRACE ***\n\n");

}

