// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See LICENSE for licensing information
//


#include "globals.h"
#include "gameengine.h"
#include "showloading.h"
#include "engine.h"
#include "showpicture.h"
#ifdef ALLOW_ACTIONCAPTURE
#include "actioncapture.h"
#endif // ALLOW_ACTIONCAPTURE
#include "sound.h"
#include "errorhandler.h"
#include "highscore.h"
#include "levelhandler.h"
#include <stdio.h>
#include "bsgui.h"

GAMEDATA gamedata;

void initGame() {
	gamedata.player = &gamedata.players[0];
	gamedata.player->level = 1;
	gamedata.player->score = 0;
	gamedata.player->lives = 2;
	gamedata.bphuc = false;
}

void playGame() {	
#ifdef ALLOW_ACTIONCAPTURE
	initActionCapture();
#endif // ALLOW_ACTIONCAPTURE

	/* Compute MAX_LEVELS by doing a fileglobbing (levels*.conf) */
		char fullfname_conf[MAX_FNAME_LENGTH];

	Uint32 max_levels = 1;
	Uint32 min_level = 1;
	bool searchnext = true;
	char tmpfname[100];
	while(searchnext) {
		sprintf(tmpfname, "level%d.conf", max_levels);
		
		sprintf(fullfname_conf, configGetPath(tmpfname));
		FILE* fh = fopen(fullfname_conf, "r");
		if(!fh) {
			searchnext = false;
		} else {
			fclose(fh);
			max_levels++;
		}
	}
	
	max_levels--;
	//printf("MAX LEVELS: %d\n", max_levels);
	if(gamedata.player->level < min_level) {
		gamedata.player->level = min_level;
	}

	gamedata.bphuc = false;
	showLoading();
	while(gamedata.player->level <= max_levels && gamedata.player->lives > 0) {
		gamedata.player->lives++;
		initEngine();
		soundPlayFX(FX_LEVEL_FINISHED);
		displayEngine();
		showLoading();
		deInitEngine();
		isPauseMode = false;
		gamedata.player->level =lhandle.nextlevels[lhandle.currentexit];
	}
#ifdef ALLOW_ACTIONCAPTURE
	deInitActionCapture();
#endif // ALLOW_ACTIONCAPTURE

	
	printf("FINAL SCORE: %d\n", gamedata.player->score);
	
	if(gamedata.player->lives > 0) {
		showPicture("gamewon.jpg", 5000);
	} else {
		showPicture("gameover.jpg", 5000);
	}
	
	// Highscore Handling
	gamedata.player->level--;
	initHighscore();
	enterHighscore(gamedata.player->score, gamedata.player->level);
	showHighscore();
#ifndef DISABLE_NETWORK
// 	if(guiYesNoDialog("Internet Highscore", "Submit your score to the internet?", false)) {
// 		submitInternetHighscore(gamedata.player->score, gamedata.player->level);
// 		showInternetHighscore();
// 	}
#endif // DISABLE_NETWORK
	deInitHighscore();

}
