// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//

#include "bl_lua_obj_player.h"

#define NEED_REGISTER_FUNCTION
#include "bl_lua.h"
#undef NEED_REGISTER_FUNCTION

#include "playersprite.h"
#include "bl_lua_obj_gfx.h"
#include "bl_lua_obj_anim.h"

// Player X coordinates
int luaObjPlayerX(lua_State *L) {
	Uint32 n = lua_gettop(L);
	if(n > 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjPlayerX");
	}
	double x = 0.0;
	if(n == 1) {
		if(!lua_isnumber(L, 1)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjPlayerX");
		} else {
			x = (double)lua_tonumber(L, 1);
		}
		spritex = x;
	}

	x = spritex;
	lua_pushnumber(L, x);

	return 1;
}

// Player Y coordinates
int luaObjPlayerY(lua_State *L) {
	Uint32 n = lua_gettop(L);
	if(n > 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjPlayerY");
	}
	double y = 0.0;
	if(n == 1) {
		if(!lua_isnumber(L, 1)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjPlayerY");
		} else {
			y = (double)lua_tonumber(L, 1);
		}
		spritey = y;
	}

	y = spritey;
	lua_pushnumber(L, y);

	return 1;
}

// Player X speed
int luaObjPlayerVX(lua_State *L) {
	Uint32 n = lua_gettop(L);
	if(n > 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjPlayerVX");
	}
	double x = 0.0;
	if(n == 1) {
		if(!lua_isnumber(L, 1)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjPlayerVX");
		} else {
			x = (double)lua_tonumber(L, 1);
		}
		spritevx = x;
	}

	x = spritevx;
	lua_pushnumber(L, x);

	return 1;
}

// Player Y speed
int luaObjPlayerVY(lua_State *L) {
	Uint32 n = lua_gettop(L);
	if(n > 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjPlayerVY");
	}
	double y = 0.0;
	if(n == 1) {
		if(!lua_isnumber(L, 1)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjPlayerVY");
		} else {
			y = (double)lua_tonumber(L, 1);
		}
		spritevy = y;
	}

	y = spritevy;
	lua_pushnumber(L, y);

	return 1;
}

// Kill a playder (loose a live)
int luaObjKillPlayer(lua_State *L) {
	killPlayer = true;

	// Avoid compiler warning about unused argument (because prototype requires it)
	if(!L) {
        return 0;
    }

	return 0;
}

// Player Score
int luaObjPlayerScore(lua_State *L) {
	Uint32 n = lua_gettop(L);
	if(n > 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjPlayerScore");
	}
	Uint32 score = 0;
	if(n == 1) {
		if(!lua_isnumber(L, 1)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjPlayerScore");
		} else {
			score = (Uint32)lua_tonumber(L, 1);
		}
		gamedata.player->score = score;
	}

	score = gamedata.player->score;
	lua_pushnumber(L, score);

	return 1;
}

// Player Lives
int luaObjPlayerLives(lua_State *L) {
	Uint32 n = lua_gettop(L);
	if(n > 1) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjPlayerLives");
	}
	Uint32 lives = 0;
	if(n == 1) {
		if(!lua_isnumber(L, 1)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjPlayerLives");
		} else {
			lives = (Uint32)lua_tonumber(L, 1);
		}
		gamedata.player->lives = lives;
	}

	lives = gamedata.player->lives;
	lua_pushnumber(L, lives);

	return 1;
}

// Player Graphics
int luaObjPlayerGFX(lua_State *L) {
	Uint32 n = lua_gettop(L);
	if(n < 1 || n > 2) {
		LUADIE(L, ERROR_LUAARGCOUNT, "luaObjPlayerGFX");
	}
	Uint32 movetype = 0;
	Uint32 gfxid = 0;

	if(!lua_isnumber(L, 1)) {
		LUADIE(L, ERROR_LUAL2BTYPE, "luaObjPlayerGFX (1)");
	} else {
		movetype = (Uint32)lua_tonumber(L, 1);
	}
	if(movetype > PLAYERSPRITE_MAX) {
		LUADIE(L, ERROR_LUAL2BRANGE, "luaObjPlayerGFX (1)");
	}

	if(n == 2) {
		if(!lua_istable(L, 2)) {
			LUADIE(L, ERROR_LUAL2BTYPE, "luaObjPlayerGFX (2)");
		} else {
			gfxid = luaObj_getidfromtable (L, 2, "internal");
			setSpriteGFX((PLAYERSPRITETYPE)movetype, gfxid);
		}
	}

	gfxid = getSpriteGFX((PLAYERSPRITETYPE)movetype);
	if(gfxid < MAX_FGOBJECTGFX) {
		luaObjPushGraphic(L, gfxid);
	} else {
		luaObjPushAnim(L, gfxid);
	}

	return 1;
}
