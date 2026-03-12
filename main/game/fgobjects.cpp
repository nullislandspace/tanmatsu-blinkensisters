// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#include <stdio.h>
#include "globals.h"
#include "fgobjects.h"
#include "errorhandler.h"
#include "gameengine.h"
#include "colission.h"
#include "engine.h"
#include "bl_lua.h"
#include "sound.h"
#include "playersprite.h"
#include "convert.h"
#include "levelhandler.h"

#include <string.h>

Uint32 fgobjcnt = 0;
Uint32 fgobjgfxcnt = 0;
Uint32 fgobjanimcnt = 0;

FGOBJS *fgObjs[MAX_FGOBJECTS];
FGGFX *fgGFX[MAX_FGOBJECTGFX];
FGANIMS *fgAnims[MAX_FGOBJECTGFX];
FGOVERLAP fgOverlap;

void initFGObjs() {
	fgobjcnt = 0;
	fgobjgfxcnt = 0;
	fgobjanimcnt = 0;
	
	Uint32 i;
	for(i = 0; i < MAX_FGOBJECTS; i++) {
        fgObjs[i] = 0;
    }

	for(i = 0; i < MAX_FGOBJECTGFX; i++) {
        fgGFX[i] = 0;
    }

	for(i = 0; i < MAX_FGOBJECTGFX; i++) {
        fgAnims[i] = 0;
    }
}

void deInitFGObjs() {
    Uint32 i;
	
	for(i = 0; i < MAX_FGOBJECTS; i++) {
        free(fgObjs[i]);
        fgObjs[i] = 0;
    }
    
    for(i = 0; i < MAX_FGOBJECTGFX; i++) {
        if(fgGFX[i]) {
            if(fgGFX[i]->surface) {
                SDL_FreeSurface(fgGFX[i]->surface);
            }
            free(fgGFX[i]);
            fgGFX[i] = 0;
        }
    }

    for(i = 0; i < MAX_FGOBJECTGFX; i++) {
        if(fgAnims[i]) {
            free(fgAnims[i]);
            fgAnims[i] = 0;
        }
    }
	
	fgobjgfxcnt = 0;
	fgobjcnt = 0;
}

SDL_Surface* getFGSurface(const Uint32 gfxid) {
    if(gfxid < MAX_FGOBJECTGFX) {
        if(!fgGFX[gfxid]) {
    		DIE(ERROR_INDEX_INVALID, "gfxobj");
	   }
	   return fgGFX[gfxid]->surface;
    } else {
        Uint32 tmpnum = gfxid - MAX_FGOBJECTGFX;
        return fgGFX[fgAnims[tmpnum]->frames[fgAnims[tmpnum]->currentFrame].gfxobj]->surface;
    }
    
    return 0;
}
            

Uint32 addFGObjGFX(const char *fname, bool ignoreLoadError) {
	if(fgobjgfxcnt == MAX_FGOBJECTGFX) {
		DIE(ERROR_BOUNDARY, "MAX_FGOBJECTGFX");
	}
	
	char fullfname[MAX_FNAME_LENGTH];
	sprintf(fullfname, "%s", configGetPath(fname));
	SDL_Surface* temp = IMG_Load(fullfname);
	if(!temp) {
		if(!ignoreLoadError) {
			DIE(ERROR_IMAGE_READ, fullfname);
		} else {
			printf("Warning: Can't load FgObjGFX '%s' - ignored!\n", fname);
			return PSEUDO_FGOBJECTGFXNUM;
		}
	}

    Uint32 nextfree = 0;
	for(Uint32 i = 0; i < MAX_FGOBJECTGFX; i++) {
        if(!fgGFX[i]) {
            nextfree = i;
            break;
        }
    }

    FGGFX *tmp = (FGGFX *)malloc(sizeof(FGGFX));
    if(!tmp) {
        DIE(ERROR_MALLOC, "addFGObjGFX()");
    }
    memset(tmp, 0, sizeof(FGGFX));
    fgGFX[nextfree] = tmp;

	fgGFX[nextfree]->surface = convertToBSSurface(temp);
	sprintf(fgGFX[nextfree]->filename, "%s", fname);
	SDL_FreeSurface(temp);
	fgobjgfxcnt++;
	return nextfree;
}

Uint32 addFGObjAnim(const char *templfname, const Uint32 startnum, const Uint32 endnum, const double fps, const Uint32 looptype) {
	if(fgobjanimcnt == MAX_FGOBJECTGFX) {
		DIE(ERROR_BOUNDARY, "MAX_FGOBJECTGFX");
	}
	
    Uint32 nextfree = 0;
	for(Uint32 i = 0; i < MAX_FGOBJECTGFX; i++) {
        if(!fgAnims[i]) {
            nextfree = i;
            break;
        }
    }

    FGANIMS *tmp = (FGANIMS *)malloc(sizeof(FGANIMS));
    if(!tmp) {
        DIE(ERROR_MALLOC, "addFGObjAnim()");
    }
    memset(tmp, 0, sizeof(FGANIMS));
    fgAnims[nextfree] = tmp;	
	fgobjanimcnt++;
	
	// Fill the anim with basic data
	sprintf(fgAnims[nextfree]->filetemplate, "%s", templfname);
	fgAnims[nextfree]->filestartnum = startnum;
	fgAnims[nextfree]->fileendnum = endnum;
	fgAnims[nextfree]->fps = fps;
	fgAnims[nextfree]->looptype = looptype;
	if(fps != 0) {
	   fgAnims[nextfree]->tickIncrement = 1000.0 / fps;
    } else {
        fgAnims[nextfree]->tickIncrement = 0.0;
    }
	
	// Now, prepare to load the graphics
	char tmpname[MAX_FNAME_LENGTH];
	sprintf(tmpname, "%s", templfname);
	char *tmppos = strchr(tmpname, '%');
	if(!tmppos) {
        DIE(ERROR_FILETEMPLATE, "addFGObjAnim()");
    }
    char *tmpcnt = tmppos;
    Uint32 tmplen = 1;
	tmpcnt++;
	while(*tmpcnt == '%') {
        tmplen++;
        tmpcnt++;
    }

    // Now we know the first and last char of the "%" placeholder and the minimum length...
    // break the string into 2 parts
    *tmppos = 0;
    
    // ok, we got the minimum length of the number in tmplen, the first part of the name in tmpname and the
    // second in tmpcnt
    // Generate all filenames
    fgAnims[nextfree]->frameCount = 0;
    char tmpNumber[MAX_FNAME_LENGTH]; 
    char shortName[MAX_FNAME_LENGTH];   
    for(Uint32 i = startnum; i <= endnum; i++) {
        sprintf(tmpNumber, "%d", i);
        // Padding if required
        while(strlen(tmpNumber) < tmplen) {
            sprintf(shortName, "0%s", tmpNumber);
            sprintf(tmpNumber, "%s", shortName);
        }
        sprintf(shortName, "%s%s%s", tmpname, tmpNumber, tmpcnt);
        printf("Shortname: %s\n", shortName);
        fgAnims[nextfree]->frames[fgAnims[nextfree]->frameCount].gfxobj = addFGObjGFX(shortName);
        fgAnims[nextfree]->frameCount++;
    }
     
    // Add the final settings
    if(fgAnims[nextfree]->looptype == ANIMLOOPTYPE_REVERSELOOP) {
        if(fgAnims[nextfree]->frameCount > 1) {
            fgAnims[nextfree]->currentFrame = fgAnims[nextfree]->frameCount - 1;
            fgAnims[nextfree]->loopDirection = -1;
        } else {
            fgAnims[nextfree]->currentFrame = 0;
            fgAnims[nextfree]->loopDirection = 0; // Single frame - no animation
        }
    } else {
        fgAnims[nextfree]->currentFrame = 0;
        if(fgAnims[nextfree]->frameCount > 1) {
            fgAnims[nextfree]->loopDirection = 1;
        } else {
            fgAnims[nextfree]->loopDirection = 0; // Single frame - no animation
        }
    }
    fgAnims[nextfree]->lastTick = (double)BS_GetTicks() + fgAnims[nextfree]->tickIncrement;

	return nextfree + MAX_FGOBJECTGFX; // Anims have a offset of MAX_FGOBJECTGFX as pseudo-GFX :-)
}

Uint32 duplicateFGObjAnim(const Uint32 obj) {
	Uint32 objid = obj - MAX_FGOBJECTGFX; // reverse pseudo-GFX offset
	
	if(fgobjanimcnt == MAX_FGOBJECTGFX) {
		DIE(ERROR_BOUNDARY, "MAX_FGOBJECTGFX");
	}
	
    Uint32 nextfree = 0;
	for(Uint32 i = 0; i < MAX_FGOBJECTGFX; i++) {
        if(!fgAnims[i]) {
            nextfree = i;
            break;
        }
    }
	
    FGANIMS *tmp = (FGANIMS *)malloc(sizeof(FGANIMS));
    if(!tmp) {
        DIE(ERROR_MALLOC, "addFGObjAnim()");
    }
    memset(tmp, 0, sizeof(FGANIMS));
    fgAnims[nextfree] = tmp;	
	fgobjanimcnt++;
	
	// Fill the anim with basic data
	sprintf(fgAnims[nextfree]->filetemplate, "%s", fgAnims[objid]->filetemplate);
	fgAnims[nextfree]->filestartnum = fgAnims[objid]->filestartnum;
	fgAnims[nextfree]->fileendnum = fgAnims[objid]->fileendnum;
	fgAnims[nextfree]->fps = fgAnims[objid]->fps;
	fgAnims[nextfree]->looptype = fgAnims[objid]->looptype;
	fgAnims[nextfree]->tickIncrement = fgAnims[objid]->tickIncrement;
	fgAnims[nextfree]->currentFrame = fgAnims[objid]->currentFrame;
	fgAnims[nextfree]->frameCount = fgAnims[objid]->frameCount;
	fgAnims[nextfree]->loopDirection = fgAnims[objid]->loopDirection;
	fgAnims[nextfree]->lastTick = fgAnims[objid]->lastTick;
	
    for(Uint32 i = 0; i < fgAnims[nextfree]->frameCount; i++) {
        fgAnims[nextfree]->frames[i].gfxobj = fgAnims[objid]->frames[i].gfxobj;
    }
	
	return nextfree + MAX_FGOBJECTGFX; // Anims have a offset of MAX_FGOBJECTGFX as pseudo-GFX :-)
}


void advanceFGObjAnim() {
    double curTicks = (double)BS_GetTicks();
    
 	for(Uint32 i = 0; i < MAX_FGOBJECTGFX; i++) {
        if(fgAnims[i]) {
            if(fgAnims[i]->frameCount > 1 && fgAnims[i]->fps > 0) {
                while(fgAnims[i]->lastTick < curTicks) {
                    if(fgAnims[i]->looptype == ANIMLOOPTYPE_FORWARDLOOP) {
                        fgAnims[i]->currentFrame++;
                        if((Uint32)fgAnims[i]->currentFrame == fgAnims[i]->frameCount) {
                            fgAnims[i]->currentFrame = 0;
                        }
                    } else if(fgAnims[i]->looptype == ANIMLOOPTYPE_REVERSELOOP) {
                        fgAnims[i]->currentFrame--;
                        if(fgAnims[i]->currentFrame < 0) {
                            fgAnims[i]->currentFrame = fgAnims[i]->frameCount - 1;
                        }
                    } else if(fgAnims[i]->looptype == ANIMLOOPTYPE_PINGPONG) {
                        fgAnims[i]->currentFrame += fgAnims[i]->loopDirection;
                        if(fgAnims[i]->currentFrame < 0 || (Uint32)fgAnims[i]->currentFrame == fgAnims[i]->frameCount) {
                            // Reverse direction...
                            fgAnims[i]->loopDirection *= -1;
                            // and add a "double" correction" for smooth animation
                            fgAnims[i]->currentFrame += fgAnims[i]->loopDirection * 2;
                        }
                    }
                    // Advance time
                    if(!turboMode) {
                        fgAnims[i]->lastTick += fgAnims[i]->tickIncrement;
                    } else {
                        fgAnims[i]->lastTick += (fgAnims[i]->tickIncrement / 4);
                    }
                }
            }    
        }
    }
}

bool setFGObjAnimLoopType(const Uint32 obj, const Uint32 looptype) {
    Uint32 objid = obj - MAX_FGOBJECTGFX; // reverse pseudo-GFX offset
    
    if(!fgAnims[objid]) {
		DIE(ERROR_INDEX_INVALID, "setFGObjAnimLoopType");
	}
	
	fgAnims[objid]->looptype = looptype;
    if(fgAnims[objid]->looptype == ANIMLOOPTYPE_REVERSELOOP) {
        if(fgAnims[objid]->frameCount > 1) {
            fgAnims[objid]->currentFrame = fgAnims[objid]->frameCount - 1;
            fgAnims[objid]->loopDirection = -1;
        } else {
            fgAnims[objid]->currentFrame = 0;
            fgAnims[objid]->loopDirection = 0; // Single frame - no animation
        }
    } else {
        fgAnims[objid]->currentFrame = 0;
        if(fgAnims[objid]->frameCount > 1) {
            fgAnims[objid]->loopDirection = 1;
        } else {
            fgAnims[objid]->loopDirection = 0; // Single frame - no animation
        }
    }    
    return true;
}

bool setFGObjAnimCurrentFrame(const Uint32 obj, const Uint32 frameNum) {
    Uint32 objid = obj - MAX_FGOBJECTGFX; // reverse pseudo-GFX offset
    
    if(!fgAnims[objid]) {
		DIE(ERROR_INDEX_INVALID, "setFGObjAnimLoopType");
	}

    if(frameNum >= fgAnims[objid]->frameCount) {
        return false;
    } else {
        fgAnims[objid]->currentFrame = frameNum; 
        // Advance time
        fgAnims[objid]->lastTick = (double)BS_GetTicks();
    }
    return true;
}
    
bool setFGObjAnimCurrentDirection(const Uint32 obj, const Sint32 loopDirection) {
    Uint32 objid = obj - MAX_FGOBJECTGFX; // reverse pseudo-GFX offset
    
    if(!fgAnims[objid]) {
		DIE(ERROR_INDEX_INVALID, "setFGObjAnimLoopType");
	}
	
	if(fgAnims[objid]->looptype != ANIMLOOPTYPE_REVERSELOOP) {
        return false;
    } else if(fgAnims[objid]->frameCount == 1) {
        return false;
    } else {
        fgAnims[objid]->loopDirection = loopDirection;
    }
    return true;
}
    
bool setFGObjAnimFrameRate(const Uint32 obj, const double fps) {
    Uint32 objid = obj - MAX_FGOBJECTGFX; // reverse pseudo-GFX offset
    
    if(!fgAnims[objid]) {
		DIE(ERROR_INDEX_INVALID, "setFGObjAnimLoopType");
	}
	
    fgAnims[objid]->fps = fps;
    if(fps != 0) {
	   fgAnims[objid]->tickIncrement = 1000.0 / fps;    
    } else {
        fgAnims[objid]->tickIncrement = 0;
    }
    // Advance time
    fgAnims[objid]->lastTick = (double)BS_GetTicks();
    return true;
}

Uint32 getFGObjAnimLoopType(const Uint32 obj) {
    Uint32 objid = obj - MAX_FGOBJECTGFX; // reverse pseudo-GFX offset
    
    if(!fgAnims[objid]) {
		DIE(ERROR_INDEX_INVALID, "setFGObjAnimLoopType");
	}
	return fgAnims[objid]->looptype;
}

Uint32 getFGObjAnimCurrentFrame(const Uint32 obj) {
    Uint32 objid = obj - MAX_FGOBJECTGFX; // reverse pseudo-GFX offset
    
    if(!fgAnims[objid]) {
		DIE(ERROR_INDEX_INVALID, "setFGObjAnimLoopType");
	}
	
	return fgAnims[objid]->currentFrame;
}

Sint32 getFGObjAnimCurrentDirection(const Uint32 obj) {
    Uint32 objid = obj - MAX_FGOBJECTGFX; // reverse pseudo-GFX offset
    
    if(!fgAnims[objid]) {
		DIE(ERROR_INDEX_INVALID, "setFGObjAnimLoopType");
	}	
	return fgAnims[objid]->loopDirection;
}

double getFGObjAnimFrameRate(const Uint32 obj) {
    Uint32 objid = obj - MAX_FGOBJECTGFX; // reverse pseudo-GFX offset
    
    if(!fgAnims[objid]) {
		DIE(ERROR_INDEX_INVALID, "setFGObjAnimLoopType");
	}
    return fgAnims[objid]->fps;	
}


Uint32 addFGObj(const Uint32 gfxobj, const Sint32 x, const Sint32 y, const bool isBlocking, const bool isVisible, const bool isOnTop, const bool isKilling) {
	if(fgobjcnt == MAX_FGOBJECTS) {
		DIE(ERROR_BOUNDARY, "MAX_FGOBJECT");
	}
	
	Uint32 nextfree = 0;
	for(Uint32 i = 0; i < MAX_FGOBJECTS; i++) {
        if(!fgObjs[i]) {
            nextfree = i;
            break;
        }
    }
	
    FGOBJS *tmp = (FGOBJS *)malloc(sizeof(FGOBJS));
    if(!tmp) {
        DIE(ERROR_MALLOC, "addFGObj()");
    }
    memset(tmp, 0, sizeof(FGOBJS));
    fgObjs[nextfree] = tmp;
	
	fgObjs[nextfree]->gfxobj = gfxobj;
	fgObjs[nextfree]->x = x;
	fgObjs[nextfree]->y = y;
	fgObjs[nextfree]->isBlocking = isBlocking;
	fgObjs[nextfree]->isVisible = isVisible;
	fgObjs[nextfree]->isOnTop = isOnTop;
	fgObjs[nextfree]->isKilling = isKilling;
	fgObjs[nextfree]->isPixel = false;
	fgObjs[nextfree]->isElevator = false;
	
	fgobjcnt++;
	return nextfree;
}

bool deleteFGObj(const Uint32 objid) {
	if(objid >= MAX_FGOBJECTS) {
		DIE(ERROR_BOUNDARY, "MAX_FGOBJECT");
	}
	if(fgObjs[objid]) {
        free(fgObjs[objid]);
        fgObjs[objid] = 0;
        fgobjcnt--;
        return true;
    }
    return false;
}

void setFGObjSetVisible(const Uint32 obj, const bool visible) {
	if(!fgObjs[obj]) {
		DIE(ERROR_INDEX_INVALID, "fgobj");
	}
	fgObjs[obj]->isVisible = visible;
}

void setFGObjSetBlocking(const Uint32 obj, const bool blocking) {
	if(!fgObjs[obj]) {
		DIE(ERROR_INDEX_INVALID, "fgobj");
	}
	fgObjs[obj]->isBlocking = blocking;
}

void setFGObjSetKilling(const Uint32 obj, const bool killing) {
	if(!fgObjs[obj]) {
		DIE(ERROR_INDEX_INVALID, "fgobj");
	}
	fgObjs[obj]->isKilling = killing;
}

void setFGObjSetPixel(const Uint32 obj, const bool pixel) {
	if(!fgObjs[obj]) {
		DIE(ERROR_INDEX_INVALID, "fgobj");
	}
	fgObjs[obj]->isPixel = pixel;
}

void setFGObjSetOnTop(const Uint32 obj, const bool ontop) {
	if(!fgObjs[obj]) {
		DIE(ERROR_INDEX_INVALID, "fgobj");
	}
	fgObjs[obj]->isOnTop = ontop;
}

void setFGObjSetElevator(const Uint32 obj, const bool elevator) {
	if(!fgObjs[obj]) {
		DIE(ERROR_INDEX_INVALID, "fgobj");
	}
	fgObjs[obj]->isElevator = elevator;
}

bool getFGObjSetVisible(const Uint32 obj) {
    if(!fgObjs[obj]) {
		DIE(ERROR_INDEX_INVALID, "fgobj");
	}
	return fgObjs[obj]->isVisible;
}

bool getFGObjSetBlocking(const Uint32 obj) {
    if(!fgObjs[obj]) {
		DIE(ERROR_INDEX_INVALID, "fgobj");
	}

	return fgObjs[obj]->isBlocking;
}

bool getFGObjSetOnTop(const Uint32 obj) {
    if(!fgObjs[obj]) {
		DIE(ERROR_INDEX_INVALID, "fgobj");
	}
	return fgObjs[obj]->isOnTop;
}

bool getFGObjSetElevator(const Uint32 obj) {
    if(!fgObjs[obj]) {
		DIE(ERROR_INDEX_INVALID, "fgobj");
	}
	return fgObjs[obj]->isElevator;
}

bool getFGObjSetKilling(const Uint32 obj) {
    if(!fgObjs[obj]) {
		DIE(ERROR_INDEX_INVALID, "fgobj");
	}
	return fgObjs[obj]->isKilling;
}

bool getFGObjSetPixel(const Uint32 obj) {
    if(!fgObjs[obj]) {
		DIE(ERROR_INDEX_INVALID, "fgobj");
	}
	return fgObjs[obj]->isPixel;
}


void setFGObjSetPos(const Uint32 obj, const Sint32 x, const Sint32 y) {
	if(!fgObjs[obj]) {
		DIE(ERROR_INDEX_INVALID, "fgobj");
	}

	if(fgObjs[obj]->hasPlayerElevatorCollission && spritevy >= 0) {
	    spritey += (double)abs((int)(fgObjs[obj]->y - y)) + 1.0;
	}

	fgObjs[obj]->x = x;
	fgObjs[obj]->y = y;	
}

void setFGObjSetPosX(const Uint32 obj, const Sint32 x) {
	if(!fgObjs[obj]) {
		DIE(ERROR_INDEX_INVALID, "fgobj");
	}

	fgObjs[obj]->x = x;
}

void setFGObjSetPosY(const Uint32 obj, const Sint32 y) {
	if(!fgObjs[obj]) {
		DIE(ERROR_INDEX_INVALID, "fgobj");
	}

	if(fgObjs[obj]->hasPlayerElevatorCollission && spritevy >= 0) {
	    spritey += (double)abs((int)(fgObjs[obj]->y - y)) + 1.0;
	}

	fgObjs[obj]->y = y;	
}

Sint32 getFGObjSetPosX(const Uint32 obj) {
	if(!fgObjs[obj]) {
		DIE(ERROR_INDEX_INVALID, "fgobj");
	}

	return fgObjs[obj]->x;
}

Sint32 getFGObjSetPosY(const Uint32 obj) {
	if(!fgObjs[obj]) {
		DIE(ERROR_INDEX_INVALID, "fgobj");
	}

	return fgObjs[obj]->y;
}

void setFGObjSetGFX(const Uint32 obj, const Uint32 gfx) {
	if(!fgObjs[obj]) {
		DIE(ERROR_INDEX_INVALID, "fgobj");
	}
	
	fgObjs[obj]->gfxobj = gfx;
}

Uint32 getFGObjSetGFX(const Uint32 obj) {
	if(!fgObjs[obj]) {
		DIE(ERROR_INDEX_INVALID, "fgobj");
	}
	
	return fgObjs[obj]->gfxobj;
}

void paintSingleFGObj(SDL_Surface *gfx, const Sint32 x, const Sint32 y) {
	
	SDL_Rect src;
	SDL_Rect dest;
	Sint32 tmpoffs;
	if(x >= 0) {
		src.x = 0;
		src.w = gfx->w;
		dest.x = x;
		dest.w = gfx->w;
	} else if((Sint32)gfx->w + x > 0) {
		tmpoffs = -x;
		src.x = tmpoffs;
		src.w = gfx->w - tmpoffs;
		dest.x = 0;
		dest.w = gfx->w - tmpoffs;
	} else {
		// Nothing to paint
		return;
	}
	
	if(y >= 0) {
		src.y = 0;
		src.h = gfx->h;
		dest.y = y;
		dest.h = gfx->h;
	} else if((Sint32)gfx->h + y > 0) {
		tmpoffs = -y;
		src.y = tmpoffs;
		src.h = gfx->h - tmpoffs;
		dest.y = 0;
		dest.h = gfx->h - tmpoffs;
	} else {
		// Nothing to paint
		return;
	}	

	SDL_BlitSurface(gfx, &src, gScreen, &dest);

	
	return;
}

void paintFGObjs(const Uint32 xoffs, const Uint32 yoffs, const bool topObjects) {
	Sint32 xpos;
	Sint32 ypos;
	
	for(Uint32 i = 0; i < MAX_FGOBJECTS; i++) {
        if(!fgObjs[i]) {
            continue;
        }
		xpos =  fgObjs[i]->x - (Sint32)xoffs;
		ypos =  fgObjs[i]->y - (Sint32)yoffs;
		SDL_Surface *tempsurface = getFGSurface(fgObjs[i]->gfxobj);
		if(fgObjs[i]->isVisible && fgObjs[i]->isOnTop == topObjects && xpos + tempsurface->w >= 0 && ypos + tempsurface->h >= 0 && xpos < SCR_WIDTH && ypos < SCR_HEIGHT) {
			paintSingleFGObj(tempsurface, xpos, ypos);
		}
	}
}

void doFGPlayerAction(const Uint32 playerx, const Uint32 playery) {
	Uint32 playerxmax = playerx + TILESIZE;
	Uint32 playerymax = playery + TILESIZE;
	for(Uint32 i = 0; i < MAX_FGOBJECTS; i++) {
		if(!fgObjs[i] || !fgObjs[i]->isVisible) {
			continue;
		}
		
		SDL_Surface *tempsurface = getFGSurface(fgObjs[i]->gfxobj);
		Uint32 xpos =  fgObjs[i]->x;
		Uint32 ypos =  fgObjs[i]->y;
		Uint32 xmax = tempsurface->w + xpos;
		Uint32 ymax = tempsurface->h + ypos;
		
		if(playerxmax > xpos && playerx < xmax && playerymax > ypos && playery < ymax) {
			if(lhandle.hasScript) {
				blLuaCall(lhandle.blLuaState, "handleAction", "i", i);
			}
			if(lhandle.hasOOScript && strlen(fgObjs[i]->luaCB[OBJECT_CB_PLAYERACTION].cbName) > 0) {
				blLuaCall(lhandle.blOOLuaState, fgObjs[i]->luaCB[OBJECT_CB_PLAYERACTION].cbName, "F", i);
			}
		}
	}
}

void handlePlayerFGColission() {
	handlePlayerFGPixelCollission();
	handlePlayerFGTrueCollission();
}

void handlePlayerFGTrueCollission() {
    Sint32 playerx = (Sint32)spritex;
	Sint32 playery = (Sint32)spritey;
	
	for(Uint32 i = 0; i < MAX_FGOBJECTS; i++) {
        if(!fgObjs[i]) {
            continue;
        }
        fgObjs[i]->hasPlayerElevatorCollission = false;
		if(fgObjs[i]->isKilling || !fgObjs[i]->isVisible || !fgObjs[i]->isBlocking) {
			continue;
		}
		// Check if player CAN touch the object
		SDL_Surface *tempsurface = getFGSurface(fgObjs[i]->gfxobj);
		if((playerx + TILESIZE) <= fgObjs[i]->x || (playery + TILESIZE) <= fgObjs[i]->y ||
		      (fgObjs[i]->x + tempsurface->w) <= playerx ||
		      (fgObjs[i]->y + tempsurface->h) <= playery) {
            // not overlapping
            continue;
        }
        if(i == 5) {
            printf("TEST\n");
        }
        if(checkPlayerFGTrueCollission(i)) {
            // Check bottom
            if(spritevy > 0.0 && fgOverlap.bottom) {
                spritevy = 0;
                spritey -= fgOverlap.bottom;
                isJumping = false;
                checkPlayerFGTrueCollission(i);
                if(fgObjs[i]->isElevator) {
                    fgObjs[i]->hasPlayerElevatorCollission = true;
                }
            }
            // Check left
            if(spritevx < 0.0 && fgOverlap.left) {
                spritevx = 0;
                spritex += fgOverlap.left;
                checkPlayerFGTrueCollission(i);
            }
            // Check right
            if(spritevx > 0.0 && fgOverlap.right > 0) {
                spritevx = 0;
                spritex -= fgOverlap.right;
                checkPlayerFGTrueCollission(i);
            }
            // Check top
            if(spritevy < 0.0 && fgOverlap.top) {
                spritevy = 0;
                spritey += fgOverlap.top;
                //checkPlayerFGTrueCollission(i);
            }

        }
    }
    
}


bool getPlayerFGKillColission() {
	Sint32 playerx = (Sint32)spritex;
	Sint32 playery = (Sint32)spritey;
	
	for(Uint32 i = 0; i < MAX_FGOBJECTS; i++) {
		if(!fgObjs[i] || !fgObjs[i]->isKilling || !fgObjs[i]->isVisible) {
			continue;
		}
		
		SDL_Surface *tempsurface = getFGSurface(fgObjs[i]->gfxobj);
		if((playerx + TILESIZE) > fgObjs[i]->x && playerx < (fgObjs[i]->x + tempsurface->w) &&
		   (playery + TILESIZE) > fgObjs[i]->y && playery < (fgObjs[i]->y + tempsurface->h) &&
           checkPlayerFGTrueCollission(i)) {
			return true;
		}
	}
	return false;
}

void handlePlayerFGPixelCollission() {
	Sint32 playerx = (Sint32)spritex;
	Sint32 playery = (Sint32)spritey;
	
	for(Uint32 i = 0; i < MAX_FGOBJECTS; i++) {
		if(!fgObjs[i] || !fgObjs[i]->isPixel || !fgObjs[i]->isVisible) {
			continue;
		}
		
		SDL_Surface *tempsurface = getFGSurface(fgObjs[i]->gfxobj);
		if((playerx + TILESIZE) > fgObjs[i]->x && playerx < (fgObjs[i]->x + tempsurface->w) &&
		   (playery + TILESIZE) > fgObjs[i]->y && playery < (fgObjs[i]->y + tempsurface->h) &&
           checkPlayerFGTrueCollission(i)) {
			foundPixels++;
			gamedata.player->score += PIXEL_SCORE;
			soundPlayFX(FX_COLLECT_PIXEL);
			fgObjs[i]->isVisible = false;
		}
	}
}

void handlePlayerFGObjCallbacks() {
	Sint32 playerx = (Sint32)spritex;
	Sint32 playery = (Sint32)spritey;
	
	for(Uint32 i = 0; i < MAX_FGOBJECTS; i++) {
		if(!fgObjs[i] || fgObjs[i]->luaCB[OBJECT_CB_PLAYERCOLLISION].cbName[0] == 0) {
			continue;
		}
		
		SDL_Surface *tempsurface = getFGSurface(fgObjs[i]->gfxobj);
		if((playerx + TILESIZE) > fgObjs[i]->x && playerx < (fgObjs[i]->x + tempsurface->w) &&
		   (playery + TILESIZE) > fgObjs[i]->y && playery < (fgObjs[i]->y + tempsurface->h) &&
           checkPlayerFGTrueCollission(i)) {
			blLuaCall(lhandle.blOOLuaState, fgObjs[i]->luaCB[OBJECT_CB_PLAYERCOLLISION].cbName, "FC", i);
		}
	}
}

bool checkPlayerFGTrueCollission(const Uint32 objid) {

	bool overlap = false;
	fgOverlap.top = 0;
	fgOverlap.bottom = 0;
	fgOverlap.left = 0;
	fgOverlap.right = 0;



	SDL_Surface* obj = getFGSurface(fgObjs[objid]->gfxobj);
	
	if (SDL_MUSTLOCK(playersprite))
		if (SDL_LockSurface(playersprite) < 0) 
			return false;
	if (SDL_MUSTLOCK(obj))
		if (SDL_LockSurface(obj) < 0) 
			return false;
	
	Uint32 pitch_sprite = playersprite->pitch / 4;
	Uint32 pitch_obj = obj->pitch / 4;
	Sint32 i, j, k, l;
	Uint32 pixel;
	
	Sint32 playerscreenx, playerscreeny;
	Sint32 objx, objy;
	Sint32 minx = -1;
	Sint32 miny = -1;
	Sint32 maxx = TILESIZE;
	Sint32 maxy = TILESIZE;
	
	// Check for colission
	for(i=0; i < TILESIZE; i++) {
		for(j=0; j < TILESIZE; j++) {
			pixel = ((Uint32 *)playersprite->pixels)[(j + playersprite_srcoffset) * pitch_sprite + i];
			
			if(pixel != 0x00000000) {
				playerscreenx = (Uint32)spritex + i;
				playerscreeny = (Uint32)spritey + j;
				// Check, if this point CAN touch the playersprite...
				if(playerscreenx >= fgObjs[objid]->x && playerscreeny >= fgObjs[objid]->y &&
				   playerscreenx < (fgObjs[objid]->x + obj->w) &&
				   playerscreeny < (fgObjs[objid]->y + obj->h)) {
					
					// OK, pixel overlaps, calculate in-object position and check for transparency
					objx = playerscreenx - fgObjs[objid]->x;
					objy = playerscreeny - fgObjs[objid]->y;
					
					pixel = ((Uint32 *)obj->pixels)[objy * pitch_obj + objx];
					if((pixel & 0xff000000) > 0x80000000) {
						overlap = true;
						// Search to left
						if(i < TILESIZE/2) {
						     minx = TILESIZE;
						     for(k = i; k >= 0; k--) {
                                 if(((Uint32 *)playersprite->pixels)[(j + playersprite_srcoffset) * pitch_sprite + k] != 0x00000000) {
                                    minx = k;
                                }
                            }
                            minx = i - minx + 1;
                            if(minx > fgOverlap.left) {
                                fgOverlap.left = minx;
                            }
                        }

                        // Search to right
						if(i >= TILESIZE/2) {
						     maxx = 0;
						     for(k = i; k < TILESIZE; k++) {
                                 if(((Uint32 *)playersprite->pixels)[(j + playersprite_srcoffset) * pitch_sprite + k] != 0x00000000) {
                                    maxx = k;
                                }
                            }
                            maxx = maxx - i + 1;
                            if(maxx > fgOverlap.right) {
                                fgOverlap.right = maxx;
                            }
                        }

                        // Search to top
						if(j < TILESIZE/2) {
						     miny = TILESIZE;
						     for(l = j; l >= 0; l--) {
                                 if(((Uint32 *)playersprite->pixels)[(l + playersprite_srcoffset) * pitch_sprite + i] != 0x00000000) {
                                    miny = l;
                                }
                            }
                            miny = j - miny + 1;
                            if(miny > fgOverlap.top) {
                                fgOverlap.top = miny;
                            }
                        }

                        // Search to bottom
						if(j >= TILESIZE/2) {
						     maxy = 0;
						     for(l = j; l < TILESIZE; l++) {
                                 if(((Uint32 *)playersprite->pixels)[(l + playersprite_srcoffset) * pitch_sprite + i] != 0x00000000) {
                                    maxy = l;
                                }
                            }
                            maxy = maxy - j + 1;
                            if(maxy > fgOverlap.bottom) {
                                fgOverlap.bottom = maxy;
                            }
                        }
					}
				}
				//((Uint32 *)gScreen->pixels)[(j + y) * pitch_bg + i + x] = pixel;
			}
			
		}
	}
	
	// More than a half-overlap shouldn't be possible; but check anyway...
	if(fgOverlap.left > TILESIZE/2) {
		fgOverlap.left = 0;
	}
	if(fgOverlap.top > TILESIZE/2) {
		fgOverlap.top = 0;
	}
	if(fgOverlap.right > TILESIZE/2) {
		fgOverlap.right = 0;
	}
	if(fgOverlap.bottom > TILESIZE/2) {
		fgOverlap.bottom = 0;
	}
	if(!fgOverlap.left && !fgOverlap.right && !fgOverlap.top && !fgOverlap.bottom) {
        // Someone set up us the bomb????
        overlap = false;
    }
	
	if(overlap) {
		//printf("L: %d R: %d T: %d B: %d\n", fgOverlap.left, fgOverlap.right, fgOverlap.top, fgOverlap.bottom);
		
	}
		// Unlock Surfaces if needed
		if (SDL_MUSTLOCK(playersprite)) 
			SDL_UnlockSurface(playersprite);
		if (SDL_MUSTLOCK(obj)) 
			SDL_UnlockSurface(obj);
		
	return overlap;
}
