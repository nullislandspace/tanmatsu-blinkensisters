#include "globals.h"
#include <string.h>
#include "triggers.h"
#include "fgobjects.h"
#include "errorhandler.h"
#include "engine.h"
#include "bl_lua.h"
#include "levelhandler.h"

TRIGGER gTriggers[1000];

void initTriggers() {
	memset(&gTriggers, 0, sizeof(gTriggers));
}

void deInitTriggers() {
	memset(&gTriggers, 0, sizeof(gTriggers));
}

void registerFGTrigger(Uint32 objid, bool isActive, char* funcname) {
	Uint32 i;
	bool found = false;

	if(!fgObjs[objid]) {
		DIE(ERROR_INDEX_INVALID, "ObjectID does not exist");
	}

	for(i = 0; i < MAX_TRIGGERS; i++) {
		if(gTriggers[i].type == TRIGGER_TYPE_NONE) {
			found = true;
			break;
		}
	}
	if(!found) {
		DIE(ERROR_INDEX_INVALID, "Too many triggers");
	}

	gTriggers[i].type = TRIGGER_TYPE_FG;
	gTriggers[i].isActive = isActive;
	gTriggers[i].objid = objid;
	sprintf(gTriggers[i].funcname, "%s", funcname);
}

Uint32 registerTimerTrigger(char* funcname, double tickIncrement) {
	Uint32 i;
	bool found = false;
		
	for(i = 0; i < MAX_TRIGGERS; i++) {
		if(gTriggers[i].type == TRIGGER_TYPE_NONE) {
			found = true;
			break;
		}
	}
	if(!found) {
		DIE(ERROR_INDEX_INVALID, "Too many triggers");
	}
	
	gTriggers[i].type = TRIGGER_TYPE_TIMER;
	gTriggers[i].isActive = true;
	sprintf(gTriggers[i].funcname, "%s", funcname);
	gTriggers[i].tickIncrement = tickIncrement * 1000.0;
	gTriggers[i].lastTick = (double)BS_GetTicks();
	return i; // i == TimerTrigger ID
}

void setTimerTriggerInterval(Uint32 triggerid, double tickIncrement) {
	gTriggers[triggerid].tickIncrement = tickIncrement * 1000.0;
	gTriggers[triggerid].lastTick = (double)BS_GetTicks();
}

double getTimerTriggerInterval(Uint32 triggerid) {
	return gTriggers[triggerid].tickIncrement / 1000.0;
}

void setTimerTriggerActive(Uint32 triggerid, bool isActive) {
	gTriggers[triggerid].isActive = isActive;
	gTriggers[triggerid].lastTick = (double)BS_GetTicks();
}

bool getTimerTriggerActive(Uint32 triggerid) {
	return gTriggers[triggerid].isActive;
}

	
void setTriggerActive(Uint32 objid, bool isActive) {
	Uint32 i;
	bool found = false;

	for(i = 0; i < MAX_TRIGGERS; i++) {
		if(gTriggers[i].objid == objid) {
			gTriggers[i].isActive = isActive;
			found = true;
		}
	}
	if(!found) {
		DIE(ERROR_INDEX_INVALID, "ObjectID not found in Triggers");
	}
}


void handleTriggers() {
	Uint32 i;
	Sint32 playerx = (Sint32)spritex;
	Sint32 playery = (Sint32)spritey;
	double currentTick = (double)BS_GetTicks();

	for(i = 0; i < MAX_TRIGGERS; i++) {

		// Handle FG-Object triggers
		if(gTriggers[i].type == TRIGGER_TYPE_FG && gTriggers[i].isActive) {
			// Check if player CAN touch the object
			Uint32 objid = gTriggers[i].objid;
			SDL_Surface *tempsurface = getFGSurface(fgObjs[objid]->gfxobj);
			if((playerx + TILESIZE) <= fgObjs[objid]->x || (playery + TILESIZE) <= fgObjs[objid]->y ||
			   (fgObjs[objid]->x + tempsurface->w) <= playerx ||
			   (fgObjs[objid]->y + tempsurface->h) <= playery) {
				// not overlapping
				continue;
			}
			if(checkPlayerFGTrueCollission(objid)) {
				blLuaCall(lhandle.blLuaState, gTriggers[i].funcname, "i", objid);
			}
		} else if(gTriggers[i].type == TRIGGER_TYPE_TIMER && gTriggers[i].isActive && gTriggers[i].tickIncrement > 0.0) {
			while(gTriggers[i].lastTick < currentTick) {
				blLuaCall(lhandle.blOOLuaState, gTriggers[i].funcname, "T", i); // Push timer object
				gTriggers[i].lastTick += gTriggers[i].tickIncrement;
			}
		}
	}

}
