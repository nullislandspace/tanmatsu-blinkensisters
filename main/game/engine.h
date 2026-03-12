// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#ifndef ENGINE_H
#define ENGINE_H

#include "bsscreen.h"

void initEngine(bool initForAttractMode = false);
void deInitEngine();
void displayEngine(const char* attrackModeFile = 0);
void addTimeToScore();
Uint32 BS_GetTicks();
void engineDoRender(COLOR3D mode3D);
void renderEngine();
void engineFullPhysics();

extern double spritex, spritey, spritevx, spritevy, blinkbright;
extern Uint32 spriterelx;
extern Uint32 spriterely;
extern bool isJumping;
extern bool hasGravity;
extern bool freeMovement;
extern bool lockedBG;
extern bool touchFGObject;
extern Uint32 foundPixels;
extern bool leftpressed;
extern bool rightpressed;
extern bool canPaintSpecialTiles;
extern bool allowedToExit;
extern bool attracktModeRunning;
extern bool killPlayer;
extern bool turboMode;
extern bool isPauseMode;
extern Uint32 BS_MyTick;
extern Uint32 BS_LastTick;
extern bool enableQuickDraw;

#endif // ENGINE_H
