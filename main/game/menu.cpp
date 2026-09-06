// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#include "globals.h"
#include "menu.h"
#include "fonthandler.h"
#include "errorhandler.h"
#include "sound.h"
#include "joystick.h"
#include <string.h>
#include <strings.h>
#include "drawprimitives.h"
#include "blending.h"
#include "extractmetabmf.h"
#include "gameengine.h"
#include "intro.h"
#include "engine.h"
#include "bsgui.h"
#include "showloading.h"
#include "bsscreen.h"
#include <unistd.h>

SDL_Surface* menubg = 0;
SDL_Surface* menuonlinebg = 0;

bool menuIsVideoTest = false;
Uint32 attrackModeType = 0;

// MENUCOLOR_ACTIVE and MENUCOLOR_INACTIVE are defined in pal/pal_font.cpp

void initMenu() {
#ifndef DISABLE_BACKGROUND_ART
	menubg = BS_IMG_Load_DisplayFormat(configGetPath("menubg.jpg"),DIE_ON_FILE_ERROR) ;
	menuonlinebg = BS_IMG_Load_DisplayFormat(configGetPath("menuonlinebg.jpg"),DIE_ON_FILE_ERROR);
#endif
}

void deInitMenu() {
#ifndef DISABLE_BACKGROUND_ART
	SDL_FreeSurface(menubg);
	SDL_FreeSurface(menuonlinebg);
	menubg = 0;
#endif
}

bool menuDisplay() {
	const char* menutexte[] = {"Play", "Quit", NULL};

	Uint32 sel = 1;
	Uint32 max = sizeof(menutexte)/sizeof(menutexte[0])-1;
	if(menuIsVideoTest) {
		max = 6;
	}
	bool menuRunning = true;
	flushJoystick();

	soundStartMusic("ADDON/LostPixels/menuMusic.mp3", true);
	while(menuRunning) {

		#ifdef DISABLE_BACKGROUND_ART
		drawrect(0, 0, SCR_WIDTH, SCR_HEIGHT, 0x000000);
#else
		SDL_BlitSurface(menubg, NULL, gScreen, NULL);
#endif

		Uint32 cnt=0;
		while(menutexte[cnt] != NULL) {
			if ((cnt+1)==sel) {
				char s[255];
				sprintf(s,"* %s *",menutexte[cnt]);
				renderFontHandlerText(10, 100+cnt*70, s, MENUCOLOR_ACTIVE, true, false, FONT_menufont_50); /* selected */
			} else {
				renderFontHandlerText(10, 100+cnt*70, menutexte[cnt], MENUCOLOR_INACTIVE, true, false, FONT_menufont_50); /* not selected */
			}
			cnt++ ;
		}
		renderFontHandlerText(30, 440, "Version: " VERSION, MENUCOLOR_INACTIVE, false, false, FONT_menufont_20);

		BS_Flip(gScreen); /* Update whole screen */

		// Handle Joystick. Act on RELEASE, not press: firing on the press left
		// this key's release queued for whatever screen the action opened, and
		// that screen consumed it as its own input.
		Uint32 joymove = getJoystickReleases();
		if(joymove != JOYSTICK_NONE) {
			if((joymove & JOYSTICK_UP)) {
				soundPlayFX(FX_MENU);
				sel--;
				if(!sel) {
					sel = max;
				}
			} else if((joymove & JOYSTICK_DOWN)) {
				soundPlayFX(FX_MENU);
				sel++;
				if(sel > max) {
					sel = 1;
				}
			} else if((joymove & JOYSTICK_ACTION)) {
				soundPlayFX(FX_MENU);
				if(strcmp(menutexte[sel-1],"Play")==0) {
					menuAddonDisplay();
				} else {
					if(guiYesNoDialog("Do you really want to", "quit BlinkenSisters?", true)) {
						menuRunning = false;
					}
				}
			}
		}


		// Handle Keyboard
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_KEYUP:
					CHECK_BOSSKEY ;
					if ((event.key.keysym.sym == SDLK_UP) ||(event.key.keysym.sym == SDLK_w)) {
						soundPlayFX(FX_MENU);
						sel--;
						if(!sel) {
							sel = max;
						}
					} else if ((event.key.keysym.sym == SDLK_DOWN) ||(event.key.keysym.sym == SDLK_s)) {
						soundPlayFX(FX_MENU);
						sel++;
						if(sel > max) {
							sel = 1;
						}
					} else if (event.key.keysym.sym == SDLK_RETURN) {
						soundPlayFX(FX_MENU);
						if(strcmp(menutexte[sel-1],"Play")==0) {
							menuAddonDisplay();
						} else {
							if(guiYesNoDialog("Do you really want to", "quit BlinkenSisters?", true)) {
								menuRunning = false;
							}
						}
					}
					break;
			}
		}

		// Check for "Attrack mode"
		if(soundPlayOnceFinished()) {
			menuAttrackMode();
		}

		SDL_Delay(50);
	}

	if(!menuIsVideoTest) {
		soundStopMusic();
	}

	return false;
}

// -------- INSTALL FROM LOCAL FILES ---------------------
struct WEBTOC {
	char name[100];
	char desc[100];
	char size[10];
	char fname[100];
};

/* Copy a file from src to dst. Returns true on success. */
static bool copyFile(const char* src, const char* dst) {
	FILE* ifh = fopen(src, "rb");
	if(!ifh) {
		fprintf(stderr, "Cannot open source file: %s\n", src);
		return false;
	}
	FILE* ofh = fopen(dst, "wb");
	if(!ofh) {
		fclose(ifh);
		fprintf(stderr, "Cannot open destination file: %s\n", dst);
		return false;
	}
	char buf[8192];
	size_t n;
	while((n = fread(buf, 1, sizeof(buf), ifh)) > 0) {
		fwrite(buf, 1, n, ofh);
	}
	fclose(ifh);
	fclose(ofh);
	return true;
}

bool menuOnlineDisplay() {
	Uint32 color;
	SDL_Color color2;
	char localfile[1000];
	WEBTOC files[100];

	#ifdef DISABLE_BACKGROUND_ART
		drawrect(0, 0, SCR_WIDTH, SCR_HEIGHT, 0x000000);
#else
		SDL_BlitSurface(menuonlinebg, NULL, gScreen, NULL);
#endif

	renderFontHandlerText(10, 100, "Please wait...", MENUCOLOR_ACTIVE, true, false, FONT_menufont_50);

	BS_Flip(gScreen); /* Update whole screen */

	/* Read TOC from local for_upload directory */
	char tocpath[1000];
	sprintf(tocpath, "%s%s", ADDON_LOCAL_PATH, ADDONHTTPTOCFILE);
	FILE *toc = fopen(tocpath, "r");
	if(!toc) {
		displaymessage(displaymessage_WARNING,"Cannot open local addon TOC file!\nCheck that ADDONS/for_upload/toc exists.\n\n(continuing in 10 seconds)",10000);
		return false;
	}

	Uint32 sel = 1;
	Uint32 offs = 1;
	Uint32 max = 0;
	char line[1001];
	char* tmpchar;
	char* tmpchar2;
	while(!feof(toc)) {
		max++;
		if(!fgets(line, 1000, toc)) {
			break;
		}
		while((tmpchar = strchr(line, '\n'))) {
			*tmpchar = 0;
		}
		while((tmpchar = strchr(line, '\r'))) {
			*tmpchar = 0;
		}


		if(!(tmpchar = strchr(line, '|'))) {
			DIE(ERROR_HTTPTOC, "1|2");
		}
		*tmpchar = 0;
		sprintf(files[max].name, "%s", line);
		tmpchar++;
		tmpchar2 = tmpchar;

		if(!(tmpchar = strchr(tmpchar, '|'))) {
			DIE(ERROR_HTTPTOC, "2|3");
		}
		*tmpchar = 0;
		sprintf(files[max].desc, "%s", tmpchar2);
		tmpchar++;
		tmpchar2 = tmpchar;

		if(!(tmpchar = strchr(tmpchar, '|'))) {
			DIE(ERROR_HTTPTOC, "3|4");
		}
		*tmpchar = 0;
		sprintf(files[max].size, "%s", tmpchar2);
		tmpchar++;
		tmpchar2 = tmpchar;
		sprintf(files[max].fname, "%s", tmpchar2);
	}
	fclose(toc);

	bool menuRunning = true;
	flushJoystick();
	while(menuRunning) {
		#ifdef DISABLE_BACKGROUND_ART
		drawrect(0, 0, SCR_WIDTH, SCR_HEIGHT, 0x000000);
#else
		SDL_BlitSurface(menuonlinebg, NULL, gScreen, NULL);
#endif

		// Scroll to the correct position
		while( ((Sint32)sel - (Sint32)offs) > 3) {
			offs++;
		}
		while( ((Sint32)sel - (Sint32)offs) < 0) {
			offs--;
		}

		for(Uint32 i= offs; i <= SDL_min(max, offs+3); i++) {
			int my_y = 70+(i - offs) *90;

			if(i < max) {
				blend_darkenRect(20, my_y, 600, 80, 0x00a0a0a0);
				if(i == sel) color = 0xd0d0d0 ; else color = 0x606060;
				drawrect(20, my_y, 600, 1, color);
				drawrect(20, my_y, 1, 80, color);
				drawrect(20, my_y+79, 600, 1, color);
				drawrect(619, my_y, 1, 80, color);
			}

			if(i == max) {
				if(i == sel) color2 = MENUCOLOR_ACTIVE ; else color2 = MENUCOLOR_INACTIVE ;
				renderFontHandlerText(10, my_y + 10, "Back", color2, true, false, FONT_menufont_50);
			} else {
				if(i == sel) color2 = MENUCOLOR_ACTIVE ; else color2 = MENUCOLOR_INACTIVE ;
				renderFontHandlerText(23, my_y + 3, files[i].name, color2, false, false, FONT_menufont_50);
				renderFontHandlerText(23, my_y + 57, files[i].desc, color2, false, false, FONT_menufont_20);
				renderFontHandlerText(520, my_y + 57, files[i].size, color2, false, false, FONT_menufont_20);
			}
		}
		BS_Flip(gScreen); /* Update whole screen */

		// Handle Joystick. Act on RELEASE, not press: firing on the press left
		// this key's release queued for whatever screen the action opened, and
		// that screen consumed it as its own input.
		Uint32 joymove = getJoystickReleases();
		if(joymove != JOYSTICK_NONE) {
			if((joymove & JOYSTICK_UP)) {
				soundPlayFX(FX_MENU);
				sel--;
				if(!sel) {
					sel = max;
				}
			} else if((joymove & JOYSTICK_DOWN)) {
				soundPlayFX(FX_MENU);
				sel++;
				if(sel > max) {
					sel = 1;
				}
			} else if((joymove & JOYSTICK_ACTION)) {
				soundPlayFX(FX_MENU);
				if(sel != max) {
					if(guiYesNoDialog("Install AddOn?", files[sel].name, true)) {
						soundPlayFX(FX_MENU);
						sprintf(localfile, "%s%s", ADDON_LOCAL_PATH, files[sel].fname);
						if(!copyFile(localfile, configGetPath("temp_web.bmf"))) {
							displaymessage(displaymessage_WARNING,"Cannot read local addon file!\n\n(continuing in 5 seconds)",5000);
						} else if(!extractMetaBMF(configGetPath("temp_web.bmf"))) {
							DIE(ERROR_BMFPARSE, configGetPath("temp_web.bmf"));
						}
						soundPlayFX(FX_MENU);
					}
				} else {
					menuRunning = false;
				}
			}
		}


		// Handle Keyboard
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_KEYUP:
					CHECK_BOSSKEY ;
					if ((event.key.keysym.sym == SDLK_UP) || (event.key.keysym.sym == SDLK_w)) {
						soundPlayFX(FX_MENU);
						sel--;
						if(!sel) {
							sel = max;
						}
					} else if ((event.key.keysym.sym == SDLK_DOWN) || (event.key.keysym.sym == SDLK_s)) {
						soundPlayFX(FX_MENU);
						sel++;
						if(sel > max) {
							sel = 1;
						}
					} else if (event.key.keysym.sym == SDLK_RETURN) {
						soundPlayFX(FX_MENU);
						if(sel != max) {
							if(guiYesNoDialog("Install AddOn?", files[sel].name, true)) {
								soundPlayFX(FX_MENU);
								sprintf(localfile, "%s%s", ADDON_LOCAL_PATH, files[sel].fname);
								if(!copyFile(localfile, configGetPath("temp_web.bmf"))) {
									displaymessage(displaymessage_WARNING,"Cannot read local addon file!\n\n(continuing in 5 seconds)",5000);
								} else if(!extractMetaBMF(configGetPath("temp_web.bmf"))) {
									DIE(ERROR_BMFPARSE, configGetPath("temp_web.bmf"));
								}
								soundPlayFX(FX_MENU);
							}
						} else {
							menuRunning = false;
						}
					}
					break;
			}
		}
		SDL_Delay(50);
	}
	return true;
}


// -------- ADDON DISPLAY ---------------------
struct ADDONTOC {
	char name[100];
	char desc[100];
	char fname[100];
};

bool menuAddonDisplay() {
	ADDONTOC files[100];
	Uint32 color;
	SDL_Color color2;

	Uint32 sel = 1;
	Uint32 offs = 1;
	Uint32 max = 0; /* TODO: compute max as the size of the array menutexte */
	FILE *toc = fopen(configGetPath("addons.dat"), "r");
	if(!toc) {
		//DIE(ERROR_FILE_READ, configGetPath("addons.dat"));
		max = 1;
	} else {
		char line[1001];
		char* tmpchar;
		char* tmpchar2;
		while(!feof(toc)) {
			max++;
			if(!fgets(line, 1000, toc)) {
				break;
			}
			while((tmpchar = strchr(line, '\n'))) {
				*tmpchar = 0;
			}
			while((tmpchar = strchr(line, '\r'))) {
				*tmpchar = 0;
			}


			if(!(tmpchar = strchr(line, '|'))) {
				DIE(ERROR_HTTPTOC, "1|2");
			}
			*tmpchar = 0;
			sprintf(files[max].name, "%s", line);
			tmpchar++;
			tmpchar2 = tmpchar;

			if(!(tmpchar = strchr(tmpchar, '|'))) {
				DIE(ERROR_HTTPTOC, "2|3");
			}
			*tmpchar = 0;
			sprintf(files[max].desc, "%s", tmpchar2);
			tmpchar++;
			tmpchar2 = tmpchar;

			sprintf(files[max].fname, "%s", tmpchar2);
		}
		fclose(toc);
	}

	/* LostPixels is the main game, so it always heads the list no matter what
	   order the addons were installed in. Everything else keeps its file
	   order. Entry `max` is the "Back" item, so only [1, max) are addons. */
	for(Uint32 i = 1; i < max; i++) {
		if(strcasecmp(files[i].fname, "LostPixels") == 0) {
			if(i > 1) {
				ADDONTOC lostpixels = files[i];
				for(Uint32 j = i; j > 1; j--) {
					files[j] = files[j-1];
				}
				files[1] = lostpixels;
			}
			break;
		}
	}

	bool menuRunning = true;
	flushJoystick();

	while(menuRunning) {

		#ifdef DISABLE_BACKGROUND_ART
		drawrect(0, 0, SCR_WIDTH, SCR_HEIGHT, 0x000000);
#else
		SDL_BlitSurface(menuonlinebg, NULL, gScreen, NULL);
#endif

		// Scroll to the correct position
		while( ((Sint32)sel - (Sint32)offs) > 3) {
			offs++;
		}
		while( ((Sint32)sel - (Sint32)offs) < 0) {
			offs--;
		}

		for(Uint32 i= offs; i <= SDL_min(max, offs+3); i++) {
			int my_y = 70+(i - offs) *90;

			if(i < max) {
				blend_darkenRect(20, my_y, 600, 80, 0x00a0a0a0);
				if(i == sel) color = 0xd0d0d0 ; else color = 0x606060;
				drawrect(20, my_y, 600, 1, color);
				drawrect(20, my_y, 1, 80, color);
				drawrect(20, my_y+79, 600, 1, color);
				drawrect(619, my_y, 1, 80, color);
			}

			if(i == max) {
				if(i == sel) color2 = MENUCOLOR_ACTIVE ; else color2 = MENUCOLOR_INACTIVE ;
				renderFontHandlerText(10, my_y + 10, "Back", color2, true, false, FONT_menufont_50);
			} else {
				if(i == sel) color2 = MENUCOLOR_ACTIVE ; else color2 = MENUCOLOR_INACTIVE ;
				renderFontHandlerText(23, my_y + 3, files[i].name, color2, false, false, FONT_menufont_50);
				renderFontHandlerText(23, my_y + 57, files[i].desc, color2, false, false, FONT_menufont_20);
			}
		}

		BS_Flip(gScreen); /* Update whole screen */

		// Handle Joystick. Act on RELEASE, not press: firing on the press left
		// this key's release queued for whatever screen the action opened, and
		// that screen consumed it as its own input.
		Uint32 joymove = getJoystickReleases();
		if(joymove != JOYSTICK_NONE) {
			if((joymove & JOYSTICK_UP)) {
				soundPlayFX(FX_MENU);
				sel--;
				if(!sel) {
					sel = max;
				}
			} else if((joymove & JOYSTICK_DOWN)) {
				soundPlayFX(FX_MENU);
				sel++;
				if(sel > max) {
					sel = 1;
				}
			} else if((joymove & JOYSTICK_ACTION)) {
				// FIXME: Handle CONFIGURE option
				soundPlayFX(FX_MENU);
				if(sel != max) {
					soundStopMusic();
					configSetAddOn(files[sel].fname);
					initGame();
					playGame();
					configResetAddOn();
					soundStartMusic("ADDON/LostPixels/menuMusic.mp3");
				} else {
					menuRunning = false;
				}
			}
		}


		// Handle Keyboard
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_KEYUP:
					CHECK_BOSSKEY ;
					if ((event.key.keysym.sym == SDLK_UP) || (event.key.keysym.sym == SDLK_w)) {
						soundPlayFX(FX_MENU);
						sel--;
						if(!sel) {
							sel = max;
						}
					} else if ((event.key.keysym.sym == SDLK_DOWN) ||(event.key.keysym.sym == SDLK_s)) {
						soundPlayFX(FX_MENU);
						sel++;
						if(sel > max) {
							sel = 1;
						}
					} else if (event.key.keysym.sym == SDLK_RETURN) {
						soundPlayFX(FX_MENU);
						if(sel != max) {
							soundStopMusic();
							configSetAddOn(files[sel].fname);
							initGame();
							playGame();
							configResetAddOn();
							soundStartMusic("ADDON/LostPixels/menuMusic.mp3");
						} else {
							menuRunning = false;
						}
					}
					break;
			}
		}

		// Check for "Attrack mode"
		if(soundPlayOnceFinished()) {
			menuAttrackMode();
		}

		SDL_Delay(50);
	}

	return true;
}

void menuShowNeedHelp() {
	#ifdef DISABLE_BACKGROUND_ART
		drawrect(0, 0, SCR_WIDTH, SCR_HEIGHT, 0x000000);
#else
		SDL_BlitSurface(menubg, NULL, gScreen, NULL);
#endif
	blend_darkenRect(50, 50, SCR_WIDTH - 100, SCR_HEIGHT - 100, 0x00303030);
	renderFontHandlerText(SCR_WIDTH / 2,100, "We need your help!", BS_Color_RED, true, false, FONT_menufont_30);
	renderFontHandlerText(70, 140, "Blinkensisters needs more levels\ngraphics, movies, story,\ndocumentation and - fun, of course.\n\nContact the Blinkensisters team:\n<team@blinkensisters.at>\nVisit our website at:\nhttp://www.blinkensisters.at", BS_Color_WHITE, false, false, FONT_textfont_30);
	BS_Flip(gScreen); /* Update whole screen */

	flushJoystick();
	while(1) {
		SDL_Delay(50);

		// Handle Keyboard
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_KEYUP:
					soundPlayFX(FX_MENU);
					return;
			}
		}
		if(getJoystickReleases() != JOYSTICK_NONE) {
			soundPlayFX(FX_MENU);
			return;
		}
	}
}

void menuPerformanceTestMode() {
	soundStopMusic();
	attracktModeRunning = true;
	gamedata.player = &gamedata.players[0];
	gamedata.player->level = 1;
	gamedata.player->score = 0;
	gamedata.player->lives = 100;
	configSetAddOn("LostPixels");
	showLoading();
	initEngine(true);
	displayEngine("poscap_level_1.bin");
	deInitEngine();
	gamedata.player->level = 2;
	gamedata.player->score = 0;
	gamedata.player->lives = 100;
	showLoading();
	initEngine(true);
	displayEngine("poscap_level_2.bin");
	deInitEngine();
	gamedata.player = &gamedata.players[0];
	gamedata.player->level = 3;
	gamedata.player->score = 0;
	gamedata.player->lives = 100;
	showLoading();
	initEngine(true);
	displayEngine("poscap_level_3.bin");
	deInitEngine();
	configResetAddOn();
}

void menuAttrackMode() {
	char * filename, * filename2;
	soundStopMusic();

	attracktModeRunning = true;
	attrackModeType = 0;
	while(attracktModeRunning) {
		switch(attrackModeType) {
			case 0:
				configSetAddOn("LostPixels");
				displayIntro();
				configResetAddOn();
				break;
			case 1:
				gamedata.player = &gamedata.players[0];
				gamedata.player->level = 1;
				gamedata.player->score = 0;
				gamedata.player->lives = 2;
				configSetAddOn("LostPixels");
				showLoading();
				initEngine(true);
				displayEngine("poscap_level_1.bin");
				deInitEngine();
				configResetAddOn();
				break;
			case 2:
				gamedata.player = &gamedata.players[0];
				gamedata.player->level = 2;
				gamedata.player->score = 0;
				gamedata.player->lives = 2;
				configSetAddOn("LostPixels");
				showLoading();
				initEngine(true);
				displayEngine("poscap_level_2.bin");
				deInitEngine();
				configResetAddOn();
				break;
			case 3:
				gamedata.player = &gamedata.players[0];
				gamedata.player->level = 3;
				gamedata.player->score = 0;
				gamedata.player->lives = 2;
				configSetAddOn("LostPixels");
				showLoading();
				initEngine(true);
				displayEngine("poscap_level_3.bin");
				deInitEngine();
				configResetAddOn();
				break;
			case 4:
				gamedata.player = &gamedata.players[0];
				gamedata.player->level = 4;
				gamedata.player->score = 0;
				gamedata.player->lives = 2;
				configSetAddOn("LostPixels");
				showLoading();
				initEngine(true);
				displayEngine("poscap_level_4.bin");
				deInitEngine();
				configResetAddOn();
				break;
			case 5:
				gamedata.player = &gamedata.players[0];
				gamedata.player->level = 5;
				gamedata.player->score = 0;
				gamedata.player->lives = 2;
				configSetAddOn("LostPixels");
				showLoading();
				initEngine(true);
				displayEngine("poscap_level_5.bin");
				deInitEngine();
				configResetAddOn();
				break;
			case 6:
				gamedata.player = &gamedata.players[0];
				gamedata.player->level = 6;
				gamedata.player->score = 0;
				gamedata.player->lives = 2;
				configSetAddOn("LostPixels");
				showLoading();
				initEngine(true);
				displayEngine("poscap_level_6.bin");
				deInitEngine();
				configResetAddOn();
				break;
			case 7:
				gamedata.player = &gamedata.players[0];
				gamedata.player->level = 7;
				gamedata.player->score = 0;
				gamedata.player->lives = 2;
				configSetAddOn("LostPixels");
				showLoading();
				initEngine(true);
				displayEngine("poscap_level_7.bin");
				deInitEngine();
				configResetAddOn();
				break;
			case 8:
				gamedata.player = &gamedata.players[0];
				gamedata.player->level = 8;
				gamedata.player->score = 0;
				gamedata.player->lives = 2;
				configSetAddOn("LostPixels");
				showLoading();
				initEngine(true);
				displayEngine("poscap_level_8.bin");
				deInitEngine();
				configResetAddOn();
				break;
			case 9:
				configSetAddOn("LostPixels");
				displayIntro();
				configResetAddOn();
				break;
			case 10:
				gamedata.player = &gamedata.players[0];
				gamedata.player->level = 9;
				gamedata.player->score = 0;
				gamedata.player->lives = 2;
				configSetAddOn("LostPixels");
				showLoading();
				initEngine(true);
				displayEngine("poscap_level_9.bin");
				deInitEngine();
				configResetAddOn();
				break;
			case 11:
				gamedata.player = &gamedata.players[0];
				gamedata.player->level = 10;
				gamedata.player->score = 0;
				gamedata.player->lives = 2;
				configSetAddOn("LostPixels");
				showLoading();
				initEngine(true);
				displayEngine("poscap_level_10.bin");
				deInitEngine();
				configResetAddOn();
				break;
			case 12:
				gamedata.player = &gamedata.players[0];
				gamedata.player->level = 11;
				gamedata.player->score = 0;
				gamedata.player->lives = 2;
				configSetAddOn("LostPixels");
				showLoading();
				initEngine(true);
				displayEngine("poscap_level_11.bin");
				deInitEngine();
				configResetAddOn();
				break;
			case 13:
				gamedata.player = &gamedata.players[0];
				gamedata.player->level = 12;
				gamedata.player->score = 0;
				gamedata.player->lives = 2;
				configSetAddOn("LostPixels");
				showLoading();
				initEngine(true);
				displayEngine("poscap_level_12.bin");
				deInitEngine();
				configResetAddOn();
				break;
			case 14:
				gamedata.player = &gamedata.players[0];
				gamedata.player->level = 13;
				gamedata.player->score = 0;
				gamedata.player->lives = 2;
				configSetAddOn("LostPixels");
				showLoading();
				initEngine(true);
				displayEngine("poscap_level_13.bin");
				deInitEngine();
				configResetAddOn();
				break;
			case 15:
				gamedata.player = &gamedata.players[0];
				gamedata.player->level = 14;
				gamedata.player->score = 0;
				gamedata.player->lives = 2;
				configSetAddOn("LostPixels");
				showLoading();
				initEngine(true);
				displayEngine("poscap_level_14.bin");
				deInitEngine();
				configResetAddOn();
				break;
			case 16:
				gamedata.player = &gamedata.players[0];
				gamedata.player->level = 15;
				gamedata.player->score = 0;
				gamedata.player->lives = 2;
				configSetAddOn("LostPixels");
				showLoading();
				initEngine(true);
				displayEngine("poscap_level_15.bin");
				deInitEngine();
				configResetAddOn();
				break;
			case 17:
				gamedata.player = &gamedata.players[0];
				gamedata.player->level = 16;
				gamedata.player->score = 0;
				gamedata.player->lives = 2;
				configSetAddOn("LostPixels");
				showLoading();
				initEngine(true);
				displayEngine("poscap_level_16.bin");
				deInitEngine();
				configResetAddOn();
				break;
			case 18:
				gamedata.player = &gamedata.players[0];
				gamedata.player->level = 17;
				gamedata.player->score = 0;
				gamedata.player->lives = 2;
				configSetAddOn("LostPixels");
				showLoading();
				initEngine(true);
				displayEngine("poscap_level_17.bin");
				deInitEngine();
				configResetAddOn();
				break;
			case 19:
				gamedata.player = &gamedata.players[0];
				gamedata.player->level = 18;
				gamedata.player->score = 0;
				gamedata.player->lives = 2;
				configSetAddOn("LostPixels");
				showLoading();
				initEngine(true);
				displayEngine("poscap_level_18.bin");
				deInitEngine();
				configResetAddOn();
				break;
			case 20:
				gamedata.player = &gamedata.players[0];
				gamedata.player->level = 19;
				gamedata.player->score = 0;
				gamedata.player->lives = 2;
				configSetAddOn("LostPixels");
				showLoading();
				initEngine(true);
				displayEngine("poscap_level_19.bin");
				deInitEngine();
				configResetAddOn();
				break;
			case 21:
				gamedata.player = &gamedata.players[0];
				gamedata.player->level = 20;
				gamedata.player->score = 0;
				gamedata.player->lives = 2;
				configSetAddOn("LostPixels");
				showLoading();
				initEngine(true);
				displayEngine("poscap_level_20.bin");
				deInitEngine();
				configResetAddOn();
				break;
			case 22:
				gamedata.player = &gamedata.players[0];
				gamedata.player->level = 21;
				gamedata.player->score = 0;
				gamedata.player->lives = 2;
				configSetAddOn("LostPixels");
				showLoading();
				initEngine(true);
				displayEngine("poscap_level_21.bin");
				deInitEngine();
				configResetAddOn();
				break;
			case 23:
				break;
		}
		attrackModeType++;
		if(attrackModeType == 23) {
			attrackModeType = 0;
		}
	}
	soundStartMusic("ADDON/LostPixels/menuMusic.mp3", true);
}
