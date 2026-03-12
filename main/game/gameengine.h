// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#ifndef GAMEENGINE_H
#define GAMEENGINE_H

struct PLAYER {
	Uint32 level;
	Sint32 score;
	Uint32 lives;
};

struct GAMEDATA {
	PLAYER players[MAX_PLAYERS];
	PLAYER* player;
	Uint32 highscore;
	bool bphuc;
};

extern GAMEDATA gamedata;

void initGame();
void playGame();

#endif // GAMEENGINE_H
