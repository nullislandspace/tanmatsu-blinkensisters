// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#ifndef FGOBJECTS_H
#define FGOBJECTS_H

#include "globals.h"

typedef enum _OBJECT_CALLBACK_TYPES {
	OBJECT_CB_PLAYERCOLLISION = 0,
	OBJECT_CB_PLAYERACTION,
	OBJECT_CB_LAST
} OBJECT_CALLBACK_TYPES;

struct OBJECT_CALLBACKS {
	char cbName[MAX_STRING_LENGTH];
};	

typedef struct _FGOBJS {
	bool isBlocking;
	bool isVisible;
	bool isOnTop;
	bool isKilling;
	bool isPixel;
	bool isElevator;
    bool hasPlayerElevatorCollission;
	Uint32 gfxobj;
	Sint32 x;
	Sint32 y;
	OBJECT_CALLBACKS luaCB[OBJECT_CB_LAST];
} FGOBJS;

typedef struct _FGOVERLAP {
	Sint32 left;
	Sint32 right;
	Sint32 bottom;
	Sint32 top;
} FGOVERLAP;
extern FGOVERLAP fgOverlap;

typedef enum _ANIMLOOPTYPE {
	ANIMLOOPTYPE_FORWARDLOOP = 0,
	ANIMLOOPTYPE_REVERSELOOP,
	ANIMLOOPTYPE_PINGPONG
} ANIMLOOPTYPE;

typedef struct _FGANIMFRAME {
    Uint32 gfxobj;
} FGANIMFRAME;

typedef struct _FGANIMS {
    Uint32 frameCount;
    Sint32 currentFrame;
    Sint32 loopDirection;
    Uint32 looptype;
    double lastTick;
    double fps;
    double tickIncrement;
    char filetemplate[MAX_STRING_LENGTH];
    Uint32 filestartnum;
    Uint32 fileendnum;
    FGANIMFRAME frames[MAX_FGOBJECTGFX];
} FGANIMS;

typedef struct _FGGFX {
    SDL_Surface* surface;
    char filename[MAX_STRING_LENGTH];
} FGGFX;

void initFGObjs();
void deInitFGObjs();
Uint32 addFGObjGFX(const char *fname, bool ignoreLoadError = false);
Uint32 addFGObjAnim(const char *templfname, const Uint32 startnum, const Uint32 endnum, const double fps, const Uint32 looptype);
Uint32 duplicateFGObjAnim(const Uint32 obj);
void advanceFGObjAnim();
Uint32 addFGObj(const Uint32 gfxobj, const Sint32 x, const Sint32 y, const bool isBlocking, const bool isVisible, const bool isOnTop, const bool isKilling);
bool deleteFGObj(const Uint32 objid);
void paintSingleFGObj(SDL_Surface *gfx, const Sint32 x, const Sint32 y);
void paintFGObjs(const Uint32 xoffs, const Uint32 yoffs, const bool topObjects);
void doFGPlayerAction(const Uint32 playerx, const Uint32 playery);

void setFGObjSetVisible(const Uint32 obj, const bool visible);
void setFGObjSetBlocking(const Uint32 obj, const bool blocking);
void setFGObjSetOnTop(const Uint32 obj, const bool ontop);
void setFGObjSetElevator(const Uint32 obj, const bool elevator);
void setFGObjSetGFX(const Uint32 obj, const Uint32 gfx);
Uint32 getFGObjSetGFX(const Uint32 obj);
void setFGObjSetKilling(const Uint32 obj, const bool killing);
void setFGObjSetPixel(const Uint32 obj, const bool pixel);
void setFGObjSetPos(const Uint32 obj, const Sint32 x, const Sint32 y);
void setFGObjSetPosX(const Uint32 obj, const Sint32 x);
void setFGObjSetPosY(const Uint32 obj, const Sint32 y);
Sint32 getFGObjSetPosX(const Uint32 obj);
Sint32 getFGObjSetPosY(const Uint32 obj);
bool getFGObjSetVisible(const Uint32 obj);
bool getFGObjSetBlocking(const Uint32 obj);
bool getFGObjSetOnTop(const Uint32 obj);
bool getFGObjSetElevator(const Uint32 obj);
bool getFGObjSetKilling(const Uint32 obj);
bool getFGObjSetPixel(const Uint32 obj);

bool setFGObjAnimLoopType(const Uint32 obj, const Uint32 looptype);
bool setFGObjAnimCurrentFrame(const Uint32 obj, const Uint32 frameNum);
bool setFGObjAnimCurrentDirection(const Uint32 obj, const Sint32 loopDirection);
bool setFGObjAnimFrameRate(const Uint32 obj, const double fps);

Uint32 getFGObjAnimLoopType(const Uint32 obj);
Uint32 getFGObjAnimCurrentFrame(const Uint32 obj);
Sint32 getFGObjAnimCurrentDirection(const Uint32 obj);
double getFGObjAnimFrameRate(const Uint32 obj);


void handlePlayerFGColission();
bool getPlayerFGKillColission();
void handlePlayerFGPixelCollission();
bool checkPlayerFGTrueCollission(const Uint32 objid);
void handlePlayerFGTrueCollission();
SDL_Surface* getFGSurface(const Uint32 gfxid);
void handlePlayerFGObjCallbacks();

extern Uint32 fgobjcnt;
extern Uint32 fgobjgfxcnt;
extern Uint32 fgobjanimcnt;
//extern SDL_Surface *FGOBJ_Surface[];
extern FGOBJS *fgObjs[];
extern FGGFX *fgGFX[];
extern FGANIMS *fgAnims[];
extern FGOVERLAP fgOverlap;

#endif // FGOBJECTS_H
