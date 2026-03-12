// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See LICENSE for licensing information
//


#include "globals.h"
#include "intro.h"
#include "sound.h"
#include "drawprimitives.h"
#include "debug.h"
#include "background.h"
#include "tiles.h"
#include "errorhandler.h"
#include "engine.h"
#include "joystick.h"
#include "convert.h"
#include "fonthandler.h"
#include "bsscreen.h"

SDL_Surface *introFG;
SDL_Color INTROTEXT_COLOR  = { 0xe0, 0xe0, 0xe0, 0 };
bool blinkOn = true;
int bgMusicSelect = 0;
Uint32 bgoffs = 0;


void initIntro()
{
#ifndef DISABLE_BACKGROUND_ART
	SDL_Surface *temp = IMG_Load(configGetPath("intro.png"));
	if(!temp) {
		DIE(ERROR_IMAGE_READ, "intro.png");
	}
	introFG = convertToBSSurface(temp);
	SDL_FreeSurface(temp);
#endif

	initBackground("introbg.jpg");

	switch(bgMusicSelect) {
		case 0:
			soundStartMusic("introMusic.mp3", true);
		break;
		case 1:
			soundStartMusic("bs_ingame2.mp3", true);
		break;
		case 2:
			soundStartMusic("bs_ingame0.mp3", true);
		break;
		case 3:
			soundStartMusic("bs_bonuslevel0.mp3", true);
		break;

	}
	bgMusicSelect++;
	if(bgMusicSelect == 4) {
		bgMusicSelect = 0;
	}

	gLastTick = BS_GetTicks();

}

void deInitIntro ()
{
	soundStopMusic();
#ifndef DISABLE_BACKGROUND_ART
	SDL_FreeSurface(introFG);
#endif
	deInitBackground();
}


void renderIntro()
{

	// Ask SDL for the time in milliseconds
	Uint32 tick = BS_GetTicks();

	// Delay a bit if we're too fast
	while (tick <= gLastTick) {
		SDL_Delay(1);
		tick = BS_GetTicks();
	}

	// Physics Engine
	while (gLastTick < tick)
	{
		gLastTick += 1000 / PHYSICSFPS;
		bgoffs++;
		if(bgoffs >= INTRO_BGWIDTH) {
			bgoffs -= INTRO_BGWIDTH;
		}
	}


	// This intro is quite simply made, so the foreground bitmap must be EXACTLY the same
	// size as our screen!

	// Background
	drawBackground(bgoffs, 0);

#ifndef DISABLE_BACKGROUND_ART
	SDL_BlitSurface(introFG, NULL, gScreen, NULL);
#endif

	// Lock screen if needed
	if (SDL_MUSTLOCK(gScreen))
		if (SDL_LockSurface(gScreen) < 0)
			return;

	// Draw Version String and something blinking

#ifndef CAVAC_RELEASEMODE
	drawrect(40,455,300,20,0x00000000);
#endif //  CAVAC_RELEASEMODE

#ifndef BLINKOMATEDITION
	renderFontHandlerText(45, 450, "Version: " VERSION, INTROTEXT_COLOR, false, false, FONT_textfont_20);
#else
	renderFontHandlerText(45, 450, "Blink-O-Mat Version: " VERSION, INTROTEXT_COLOR, false, false, FONT_textfont_20);
#endif // BLINKOMATEDITION

	renderFontHandlerText(320, 370, "Press Space to start", INTROTEXT_COLOR, true, false, FONT_textfont_20);
#ifndef CAVAC_RELEASEMODE
	Uint32 tmpcol;
	if(blinkOn) {
		tmpcol = 0x00FFFFFF;
	} else {
		tmpcol = 0x00000000;
	}
	blinkOn = !blinkOn;

	circleColor(gScreen,20,460,15,tmpcol);


#ifdef SHOWFRAMERATE
	displayFPSCounter();
#endif //
#endif // CAVAC_RELEASEMODE

	// Unlock screen if needed
	if (SDL_MUSTLOCK(gScreen))
		SDL_UnlockSurface(gScreen);

	// Tell SDL to update the whole screen
	BS_Flip(gScreen);

}



void displayIntro()
{

	initIntro();

	bool displayIntro = true;
	while (displayIntro)
	{
		// Render stuff
		renderIntro();

		// Stop intro if sound has finished playing
		if(soundPlayOnceFinished()) {
			displayIntro = false;
		}

		// Poll for events, and handle the ones we care about.
		Uint32 joymove = getJoystickMoves();
		if(joymove & JOYSTICK_JUMP || joymove & JOYSTICK_ACTION) {
			displayIntro = false;
			attracktModeRunning = false;
		}
		SDL_Event event;
		while (SDL_PollEvent(&event) && displayIntro)
		{
			switch (event.type)
			{
				case SDL_KEYDOWN:
					break;
				case SDL_KEYUP:
					CHECK_BOSSKEY ;
					if (event.key.keysym.sym != SDLK_ESCAPE) {
						displayIntro = false;
						attracktModeRunning = false;
					}
					break;
				case SDL_QUIT:
					exit(0);
			}
		}
	}

	deInitIntro();
}

