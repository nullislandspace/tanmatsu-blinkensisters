// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#ifndef TILES_H
#define TILES_H

void initTiles(const char *fname);
void deInitTiles();
void paintSingleTile(const Uint32 num, const Sint32 x, const Sint32 y);
Uint32 getNumOfTiles();
void paintLevelTiles(const Uint32 xoffs, const Uint32 yoffs);

typedef enum _SPECIALTILES {
	SPECIALTILE_START,
	SPECIALTILE_GOAL
} SPECIALTILES;

#endif // TILES_H
