// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#ifndef COLISSION_H
#define COLISSION_H

#include "monstersprites.h"

typedef enum _COL_TYPE {
	COL_NONE = 0,
	COL_LEFT = 1,
	COL_RIGHT = 2,
	COL_UP = 4,
	COL_DOWN = 8
} COL_TYPE;

Uint32 getPlayerMonsterColission();
bool handlePixelCollission();
void handlePlayerTilesColission();
void handlePlayerTilesColission_magicTiles();
void handlePlayerTilesColission_down_limited();
void handlePlayerTilesColission_down();
void handlePlayerTilesColission_up();
void handlePlayerTilesColission_up_limited();
void handlePlayerTilesColission_right();
void handlePlayerTilesColission_left();

#endif // COLISSION_H
