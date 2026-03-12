// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#ifndef HIGHSCORE_H
#define HIGHSCORE_H

struct HIGHSCORE {
	Uint32 score;
	Uint32 level;
	char name[MAX_STRING_LENGTH];
};

struct HIGHSCORELIST {
	HIGHSCORE scores[10];
};

void initHighscore();
void showHighscore();
void enterHighscore(Uint32 score, Uint32 level);
void displayNonHighscore(Uint32 score, Uint32 level);
void deInitHighscore();
void waitHighscore();

#ifndef DISABLE_NETWORK
void submitInternetHighscore(Uint32 score, Uint32 level);
void showInternetHighscore();
#endif // DISABLE_NETWORK

#endif // HIGHSCORE_H



