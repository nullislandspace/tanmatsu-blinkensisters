// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#include "globals.h"
#include "highscore.h"
#include "errorhandler.h"
#include "joystick.h"
#include "fonthandler.h"
#include "gameengine.h"
#include "drawprimitives.h"
#include "bsscreen.h"
#include <string.h>


SDL_Surface *HIGHSCORE_Surface;
HIGHSCORELIST hslist;
SDL_Color HSCOLOR   = { 0xff, 0xff, 0xff, 0 };

void initHighscore() {
	char fullfname[MAX_FNAME_LENGTH];
	// load Background image
#ifndef DISABLE_BACKGROUND_ART
	HIGHSCORE_Surface = BS_IMG_Load_DisplayFormat(configGetPath("highscorebg.jpg"),DIE_ON_FILE_ERROR) ;
#endif

	// Pre-Init highscores in case we can't load them
	for(Uint32 i=0; i < 10; i++) {
		sprintf(hslist.scores[i].name, "MoveZig");
		hslist.scores[i].score = (10 - i) * 10;
		hslist.scores[i].level = 10 - i;
	}

	sprintf(fullfname, "%s", configGetPath("highscore.dat"));

	FILE* ifh = fopen(fullfname, "rb");
	if(!ifh) {
		// Create default file
		FILE* ofh = fopen(fullfname, "wb");
		if(!ofh) {
			DIE(ERROR_FILE_WRITE, fullfname);
		}
		fwrite(&hslist, sizeof(hslist), 1, ofh);
		fclose(ofh);
	} else {
		fread(&hslist, sizeof(hslist), 1, ifh);
		fclose(ifh);
	}
}

void enterHighscore(Uint32 score, Uint32 level) {
	// First, check if the player HAS made it into the highscore list
	if(score <= hslist.scores[9].score || gamedata.bphuc) {
		displayNonHighscore(score, level);
		return;
	}

	bool enteringName = true;

	flushJoystick();
	char name[100];
	name[0] = '\0';
	Uint32 i;
	Uint32 highest = 10;
	Uint32 waitUntil =SDL_GetTicks() + 20000;


	while(enteringName) {
		if(SDL_GetTicks() > waitUntil) {
			enteringName = false;
		}

		// Copy the background into the bigger surface
		#ifdef DISABLE_BACKGROUND_ART
		drawrect(0, 0, SCR_WIDTH, SCR_HEIGHT, 0x000000);
#else
		SDL_BlitSurface(HIGHSCORE_Surface, 0, gScreen, 0);
#endif

		char tmp[100];
		renderFontHandlerText(0, 70, "Your Score", HSCOLOR, true, false, FONT_menufont_50);
		sprintf(tmp, "Score: %d\nLevel: %d", score, level);
		renderFontHandlerText(220, 140, tmp, HSCOLOR, false, false, FONT_menufont_20);
		if (SDL_GetTicks() % 200 < 100) { // blinking cursor
			sprintf(tmp, "You made it to the local highscore!\n\nEnter your name: %s_", name);
		} else {
			sprintf(tmp, "You made it to the local highscore!\n\nEnter your name: %s ", name);
		} ;
		renderFontHandlerText(100, 310, tmp, HSCOLOR, false, false, FONT_textfont_20);


		// Tell SDL to update the whole screen
		BS_Flip(gScreen);

		// Handle input
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type)
			{
				case SDL_KEYDOWN:
					break;
				case SDL_KEYUP:
					CHECK_BOSSKEY ;
					if (event.key.keysym.sym == SDLK_RETURN) {
						enteringName = false;
					} else {
						sprintf(tmp, "%c", event.key.keysym.sym);
						if(strstr(NAMECODECHARS, tmp) && strlen(name) < 11) {
							strcat(name, tmp);
						};
						if ((event.key.keysym.sym == SDLK_BACKSPACE)  && (strlen(name)>0)) { /* Backspace - delete last char */
							name[strlen(name)-1] = '\0' ;
						};
					}
					break;
				case SDL_QUIT:
					quitToLauncher();
			}
		}
		if(enteringName) {
			// Require less CPU
			SDL_Delay(10);
		}
	}

	if(strlen(name) == 0) {
		// do not enter empty name into highscore
		return;
	}

	// Find where to insert new score
	for(i = 0; i < 9; i++) {
		if(highest == 10 && score > hslist.scores[i].score) {
			highest = i;
		}
	}

	// Move scores down
	for(i = 8; i >= highest; i--) {
		hslist.scores[i+1].score = hslist.scores[i].score;
		hslist.scores[i+1].level = hslist.scores[i].level;
		sprintf(hslist.scores[i+1].name, "%s", hslist.scores[i].name);
		// Fix a small Uint problem
		if(i == 0) {
			break;
		}
	}
	hslist.scores[highest].score = score;
	hslist.scores[highest].level = level;
	sprintf(hslist.scores[highest].name, "%s", name);

	// Write new hiscore file
	char fullfname[MAX_FNAME_LENGTH];
	sprintf(fullfname, "%s", configGetPath("highscore.dat"));
	FILE* ofh = fopen(fullfname, "wb");
	if(!ofh) {
		DIE(ERROR_FILE_WRITE, fullfname);
	}
	fwrite(&hslist, sizeof(hslist), 1, ofh);
	fclose(ofh);

}

void showHighscore() {
	// Lock Surfaces if needed

	// Copy the background into the bigger surface
	#ifdef DISABLE_BACKGROUND_ART
		drawrect(0, 0, SCR_WIDTH, SCR_HEIGHT, 0x000000);
#else
		SDL_BlitSurface(HIGHSCORE_Surface, 0, gScreen, 0);
#endif


	char tmp[100];
	renderFontHandlerText(0, 70, "Local highscore", HSCOLOR, true, false, FONT_menufont_50);
	for(Uint32 i = 0; i < 10; i++) {
		sprintf(tmp, "%d", hslist.scores[i].score);
		renderFontHandlerText(100, (i * 25) + 140, tmp, HSCOLOR, false, false, FONT_menufont_20);
		renderFontHandlerText(200, (i * 25) + 140, hslist.scores[i].name, HSCOLOR, false, false, FONT_menufont_20);
		sprintf(tmp, "Level %d", hslist.scores[i].level);
		renderFontHandlerText(450, (i * 25) + 140, tmp, HSCOLOR, false, false, FONT_menufont_20);
	}
	// Tell SDL to update the whole screen
	BS_Flip(gScreen);

	waitHighscore();

}

void displayNonHighscore(Uint32 score, Uint32 level) {
	// Copy the background into the bigger surface
	#ifdef DISABLE_BACKGROUND_ART
		drawrect(0, 0, SCR_WIDTH, SCR_HEIGHT, 0x000000);
#else
		SDL_BlitSurface(HIGHSCORE_Surface, 0, gScreen, 0);
#endif


	char tmp[100];
	renderFontHandlerText(0, 70, "Your score", HSCOLOR, true, false, FONT_menufont_50);
	sprintf(tmp, "Score: %d\nLevel: %d", score,level);
	renderFontHandlerText(220, 140, tmp, HSCOLOR, false, false, FONT_menufont_20);

	renderFontHandlerText(100, 240, "SORRY! You didn't made it\n\nto the highscore list.\n\nBetter luck next time!", HSCOLOR, true, false, FONT_menufont_20);

	// Tell SDL to update the whole screen
	BS_Flip(gScreen);

	waitHighscore();

	return;
}

void deInitHighscore() {
#ifndef DISABLE_BACKGROUND_ART
	SDL_FreeSurface(HIGHSCORE_Surface);
#endif
}

void waitHighscore() {
	// Poll for events, and handle the ones we care about.
	bool running = true;
	Uint32 waitUntil = SDL_GetTicks() + 10000;
	flushJoystick();
	while(running) {
		Uint32 joymove = getJoystickReleases();
		if((joymove & JOYSTICK_JUMP) || (joymove & JOYSTICK_ACTION) || (SDL_GetTicks() > waitUntil)) {
			running = false;
		}
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_KEYDOWN:
					break;
				case SDL_KEYUP:
					// If escape is pressed, return (and thus, quit)
					if (event.key.keysym.sym != SDLK_ESCAPE) {
						running = false;
					}
					break;
				case SDL_QUIT:
					quitToLauncher();
			}
		}
		// Require less CPU
		SDL_Delay(10);
	}
}



/* replace c1 with c2 in string s. Needed for replacing some characters (currently only " ") in URLs */
void char_replace(char *s, char c1, char c2)
{
	size_t i;
	for (i = 0; i < strlen(s); i++) {
		if (s[i] == c1) s[i] = c2;
	}
}


#ifndef DISABLE_NETWORK
void submitInternetHighscore(Uint32 score, Uint32 level) {
	char tmp[200]; 
	bool enteringName = true;
	char name[100];
	name[0] = '\0';
	Uint32 waitUntil =SDL_GetTicks() + 20000;
	while(enteringName) {
		if(SDL_GetTicks() > waitUntil) {
			enteringName = false;
		}

		// Copy the background into the bigger surface
		#ifdef DISABLE_BACKGROUND_ART
		drawrect(0, 0, SCR_WIDTH, SCR_HEIGHT, 0x000000);
#else
		SDL_BlitSurface(HIGHSCORE_Surface, 0, gScreen, 0);
#endif

		char tmp[100];
		renderFontHandlerText(0, 70, "Your Score", HSCOLOR, true, false, FONT_menufont_50);
		sprintf(tmp, "Score: %d\nLevel: %d", score, level);
		renderFontHandlerText(220, 140, tmp, HSCOLOR, false, false, FONT_menufont_20);
		if (SDL_GetTicks() % 200 < 100) { // blinking cursor
			sprintf(tmp, "Enter your (Internet visible) name: %s_", name);
		} else {
			sprintf(tmp, "Enter your (Internet visible) name: %s ", name);
		} ;
		renderFontHandlerText(100, 310, tmp, HSCOLOR, false, false, FONT_textfont_20);


		// Tell SDL to update the whole screen
		BS_Flip(gScreen);

		// Handle input
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type)
			{
				case SDL_KEYDOWN:
					break;
				case SDL_KEYUP:
					CHECK_BOSSKEY ;
					if (event.key.keysym.sym == SDLK_RETURN) {
						enteringName = false;
					} else {
						sprintf(tmp, "%c", event.key.keysym.sym);
						if(strstr(NAMECODECHARS, tmp) && strlen(name) < 11) {
							strcat(name, tmp);
						};
						if ((event.key.keysym.sym == SDLK_BACKSPACE)  && (strlen(name)>0)) { /* Backspace - delete last char */
							name[strlen(name)-1] = '\0' ;
						};
					}
					break;
				case SDL_QUIT:
					quitToLauncher();
			}
		}
		if(enteringName) {
			// Require less CPU
			SDL_Delay(10);
		}
	}

	if(strlen(name) == 0) {
		// do not enter empty name into highscore
		return;
	}
	/* substitute " " with "_" in name. Easier than substiting it with the correct HTML entity - and enough for us now. Must be modiifed if other characters are allowed in names (NAMECODECHARS in globals.h). Must be replaced back, when displaying the highscore */
	char_replace((char*) name, ' ', '_');
	// Submit highscore to Internet
	sprintf(tmp, "%ssubmit/?name=%s&score=%u&level=%u&addon=%s",HIGHSCOREURL,name,score,level,currentAddonName);
	HTTP_Get(tmp,configGetPath("internethighscore.dat"),DO_NOT_SHOW_PROGRESSBAR );
}
void showInternetHighscore() {
	char tmp[500]; 

	// Copy the background into the bigger surface
	#ifdef DISABLE_BACKGROUND_ART
		drawrect(0, 0, SCR_WIDTH, SCR_HEIGHT, 0x000000);
#else
		SDL_BlitSurface(HIGHSCORE_Surface, 0, gScreen, 0);
#endif
	sprintf(tmp, "%sget/?addon=%s",HIGHSCOREURL,currentAddonName);

	HTTP_Get(tmp, configGetPath("internethighscore.dat"), DO_NOT_SHOW_PROGRESSBAR);

	renderFontHandlerText(0, 70, "Global highscore\n(for this addon)", HSCOLOR, true, false, FONT_menufont_50);
	// TODO: not ready now...
        FILE *highscorefile = fopen(configGetPath("internethighscore.dat"), "r");
        if(!highscorefile) {
                DIE(ERROR_FILE_READ, configGetPath("internethighscore.dat"));
        }
        char line[1001];
	int linenumber = 1 ;
	while(fgets(line, 1000, highscorefile)!=NULL) {
		/* TODO: Better formating (but before we must be sure,
		which data is returned by the server. 
		Now: name|score
		should we add more data (e.g. date)?
		*/
		renderFontHandlerText(0, 170+linenumber*20, line, HSCOLOR, true, false, FONT_menufont_20);
		linenumber ++ ;
	}
	fclose(highscorefile);
	// Tell SDL to update the whole screen
	BS_Flip(gScreen);

	waitHighscore();

}
#endif // DISABLE_NETWORK
