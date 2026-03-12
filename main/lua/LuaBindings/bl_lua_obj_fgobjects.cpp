// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//

#include "bl_lua_obj_fgobjects.h"

#define NEED_REGISTER_FUNCTION
#include "bl_lua.h"
#undef NEED_REGISTER_FUNCTION

#include "bl_lua_obj_gfx.h"
#include "bl_lua_obj_anim.h"

// Internal: parse property string and set properties
void luaObj_parseProperty(lua_State *L, Uint32 objid, char *property) {

	// Parse string and call corresponding SET functions
	if(strstr(property, "blocking")) {
		if(strchr(property, '!')) {
			setFGObjSetBlocking(objid, false);
		} else {
			setFGObjSetBlocking(objid, true);
		}
	} else if(strstr(property, "visible")) {
		if(strchr(property, '!')) {
			setFGObjSetVisible(objid, false);
		} else {
			setFGObjSetVisible(objid, true);
		}
	} else if(strstr(property, "ontop")) {
		if(strchr(property, '!')) {
			setFGObjSetOnTop(objid, false);
		} else {
			setFGObjSetOnTop(objid, true);
		}
	} else if(strstr(property, "killing")) {
		if(strchr(property, '!')) {
			setFGObjSetKilling(objid, false);
		} else {
			setFGObjSetKilling(objid, true);
		}
	} else if(strstr(property, "pixel")) {
		if(strchr(property, '!')) {
			setFGObjSetPixel(objid, false);
		} else {
			setFGObjSetPixel(objid, true);
		}
	} else if(strstr(property, "elevator")) {
		if(strchr(property, '!')) {
			setFGObjSetElevator(objid, false);
		} else {
			setFGObjSetElevator(objid, true);
		}
	} else {
		LUADIE(L, ERROR_LUAL2BTEXTVALUE, property);
	}

}

// INTERNAL: get property value
bool luaObj_getProperty(Uint32 objid, char *property) {

	// Parse string and call corresponding GET functions
	if(strstr(property, "blocking")) {
		return getFGObjSetBlocking(objid);
	} else if(strstr(property, "visible")) {
		return getFGObjSetVisible(objid);
	} else if(strstr(property, "ontop")) {
		return getFGObjSetOnTop(objid);
	} else if(strstr(property, "killing")) {
		return getFGObjSetKilling(objid);
	} else if(strstr(property, "pixel")) {
		return getFGObjSetPixel(objid);
	} else if(strstr(property, "elevator")) {
		return getFGObjSetElevator(objid);
	}
	return false;
}

// This dynamic property handler gets its property name from upvalues
int luaObjObjectProperty(lua_State *L) {
	char propname[1000];
	char tmp[1000];
	Uint32 objid = 0;

	Uint32 n = lua_gettop(L);
	if(n > 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjObjectProperty");
	}

	if(!lua_isnumber(L, lua_upvalueindex(1))) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObjObjectProperty (id)");
	} else {
		objid = (Uint32)lua_tonumber(L, lua_upvalueindex(1));
	}

	if(!lua_isstring(L, lua_upvalueindex(2))) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObjObjectProperty (name)");
	} else {
		sprintf(propname, "%s", lua_tostring(L, lua_upvalueindex(2)));
	}

	if(n == 1) {
		if(!lua_isnumber(L, 1)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjObjectProperty");
		} else {
			if(lua_tonumber(L, 1) == 0) {
				sprintf(tmp, "!%s", propname);
			} else if(lua_tonumber(L, 1) == 1){
				sprintf(tmp, "%s", propname);
			} else {
				LUADIE(L, ERROR_LUAL2BRANGE, "luaObjObjectProperty");
			}
		}
		luaObj_parseProperty(L, objid, tmp);
	}

	if(luaObj_getProperty(objid, propname)) {
		lua_pushnumber(L, 1);
	} else {
		lua_pushnumber(L, 0);
	}

	return 1;
}

// Object X position
int luaObjObjectX(lua_State *L) {
	Uint32 objid = 0;
	Uint32 n = lua_gettop(L);
	if(n > 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjObjectX");
	}

	if(!lua_isnumber(L, lua_upvalueindex(1))) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObjObjectX (id)");
	} else {
		objid = (Uint32)lua_tonumber(L, lua_upvalueindex(1));
	}

	Sint32 x = 0;
	if(n == 1) {
		if(!lua_isnumber(L, 1)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjObjectX");
		} else {
			x = (Sint32)lua_tonumber(L, 1);
		}
		setFGObjSetPosX(objid, x);
	}

	x = getFGObjSetPosX(objid);
	lua_pushnumber(L, x);

	return 1;
}

// Object Y position
int luaObjObjectY(lua_State *L) {
	Uint32 objid = 0;
	Uint32 n = lua_gettop(L);
	if(n > 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjObjectY");
	}

	if(!lua_isnumber(L, lua_upvalueindex(1))) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObjObjectY (id)");
	} else {
		objid = (Uint32)lua_tonumber(L, lua_upvalueindex(1));
	}

	Sint32 y = 0;
	if(n == 1) {
		if(!lua_isnumber(L, 1)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjObjectY");
		} else {
			y = (Sint32)lua_tonumber(L, 1);
		}
		setFGObjSetPosY(objid, y);
	}

	y = getFGObjSetPosY(objid);
	lua_pushnumber(L, y);

	return 1;
}

// Object GFX
int luaObjObjectGFX(lua_State *L) {
	Uint32 objid, gfxid;
	objid = gfxid = 0;
	Uint32 n = lua_gettop(L);
	if(n != 1 && n != 0) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjObjectGFX");
	}

	if(!lua_isnumber(L, lua_upvalueindex(1))) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObjObjectY (id)");
	} else {
		objid = (Uint32)lua_tonumber(L, lua_upvalueindex(1));
	}

	if(n == 1) {
		if(!lua_istable(L, 1)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjObjectGFX");
		} else {
			gfxid = luaObj_getidfromtable (L, 1, "internal");
		}
		setFGObjSetGFX(objid, gfxid);
	}
	gfxid = getFGObjSetGFX(objid);

	if(gfxid < MAX_FGOBJECTGFX) {
		luaObjPushGraphic(L, gfxid);
	} else {
		luaObjPushAnim(L, gfxid);
	}

	return 1;
}


// Object callbacks
int luaObjRegisterObjectCallback(lua_State *L) {
	Uint32 cbid = 0;
	Uint32 objid = 0;

	Uint32 n = lua_gettop(L);
	if(n > 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjRegisterObjectCallback");
	}

	if(!lua_isnumber(L, lua_upvalueindex(1))) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObjRegisterObjectCallback (objid)");
	} else {
		objid = (Uint32)lua_tonumber(L, lua_upvalueindex(1));
	}

	if(!lua_isnumber(L, lua_upvalueindex(2))) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObjRegisterObjectCallback (cbid)");
	} else {
		cbid = (Uint32)lua_tonumber(L, lua_upvalueindex(2));
	}

	if(cbid >= OBJECT_CB_LAST) {
		LUADIE(L, ERROR_LUAL2BRANGE, "luaObjRegisterObjectCallback");
	}

	//TODO: This still dooesn't use good accessors... should be changed...

	if(n == 1) {
		if(!lua_isstring(L, 1)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjRegisterObjectCallback");
		} else {
			sprintf(fgObjs[objid]->luaCB[cbid].cbName, "%s", lua_tostring(L, 1));
		}
	}
	lua_pushstring(L, fgObjs[objid]->luaCB[cbid].cbName);
	return 1;
}

// Creates a new FG Object
int luaObjNewFGObject(lua_State *L) {
	Uint32 x, y, gfxid;
	x = y = gfxid = 0;
	char properties[1000];
	Uint32 n = lua_gettop(L);
	if(n != 3 && n != 4) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjNewFGObject");
	}

	if(!lua_isnumber(L, 1)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObjNewFGObject (arg1)");
	} else {
		x = (Uint32)lua_tonumber(L, 1);
	}

	if(!lua_isnumber(L, 2)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObjNewFGObject (arg2)");
	} else {
		y = (Uint32)lua_tonumber(L, 2);
	}

	if(!lua_istable(L, 3)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObjNewFGObject (arg3)");
	} else {
		gfxid = luaObj_getidfromtable (L, 3, "internal");
	}

	// Default: All booleans off (object exists more or less only als ID)
	Uint32 objid = addFGObj(gfxid, x, y, false, false, false, false);

	if(n == 4) {
		if(!lua_isstring(L, 4)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjNewFGObject (arg4)");
		} else {
			sprintf(properties, "%s", lua_tostring(L, 4));
			char *start, *end;
			start = properties;
			while((end = strchr(start, ','))) {
				*end = '\0';
				if(strlen(start) > 0) {
					luaObj_parseProperty(L, objid, start);
				}
				end++;
				start = end;
			}
			if(strlen(start) > 0) {
				luaObj_parseProperty(L, objid, start);
			}
		}

	}

	luaObjPushFGObject(L, objid);

	return 1;
}

void luaObjPushFGObject(lua_State *L, Uint32 objid) {
	bsl_startobject();

	bsl_addobjectstringfunction("blocking", luaObjObjectProperty, objid, "blocking");
	bsl_addobjectstringfunction("visible", luaObjObjectProperty, objid, "visible");
	bsl_addobjectstringfunction("ontop", luaObjObjectProperty, objid, "ontop");
	bsl_addobjectstringfunction("killing", luaObjObjectProperty, objid, "killing");
	bsl_addobjectstringfunction("pixel", luaObjObjectProperty, objid, "pixel");
	bsl_addobjectstringfunction("elevator", luaObjObjectProperty, objid, "elevator");
	bsl_addobjectfunction("x", luaObjObjectX, objid);
	bsl_addobjectfunction("y", luaObjObjectY, objid);
	bsl_addobjectfunction("gfx", luaObjObjectGFX, objid);
	bsl_addobjectnumbervalue("internal", objid);

    bsl_starttable_local("callback");
	bsl_addobjectnumberfunction("playercollision", luaObjRegisterObjectCallback, objid, OBJECT_CB_PLAYERCOLLISION);
	bsl_addobjectnumberfunction("playeraction", luaObjRegisterObjectCallback, objid, OBJECT_CB_PLAYERACTION);
	bsl_endtable_local("callback");

	bsl_endobject();
}

void luaObjPushFGOverlapObject(lua_State *L) {
	bsl_startobject();

	bsl_addobjectnumbervalue("top", fgOverlap.top);
	bsl_addobjectnumbervalue("bottom", fgOverlap.bottom);
	bsl_addobjectnumbervalue("left", fgOverlap.left);
	bsl_addobjectnumbervalue("right", fgOverlap.right);

	bsl_endobject();
}
