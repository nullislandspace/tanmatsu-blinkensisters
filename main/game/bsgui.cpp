// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#include "globals.h"
#include "bsgui.h"
#include "errorhandler.h"
#include "fonthandler.h"
#include "sound.h"
#include "blending.h"
#include "drawprimitives.h"
#include "joystick.h"
#include "engine.h"
#include "bsscreen.h"

SDL_Surface *GUI_Surface;

SDL_Color GUICOLOR_ACTIVE   = { 0xff, 0xff, 0xff, 0 };
SDL_Color GUICOLOR_INACTIVE = { 0xa0, 0xa0, 0xa0, 0 };

void initGui() {
	/* Create a 32-bit surface with the bytes of each pixel in R,G,B,A order,
		as expected by OpenGL for textures */
	Uint32 rmask, gmask, bmask, amask;

	/* SDL interprets each pixel as a 32-bit number, so our masks must depend
		on the endianness (byte order) of the machine */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
	amask = 0x000000ff;
#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0xff000000;
#endif

	SDL_Surface* temp = SDL_CreateRGBSurface(SDL_SWSURFACE, SCR_WIDTH, SCR_HEIGHT, 32, rmask, gmask, bmask, amask);
	GUI_Surface = SDL_DisplayFormat(temp);
	SDL_FreeSurface(temp);

}

void deInitGui() {
	SDL_FreeSurface(GUI_Surface);
}

bool guiYesNoDialog(const char* line1, const char* line2, bool defaultval) {
     // we should be called with gScreen NOT locked (during physics etc)

     // ** Save currently displayed frame
	SDL_BlitSurface(gScreen, 0, GUI_Surface, 0);
	Uint32 blinkcolor;
	// The dialog is opened by a key press, so start from a clean slate: without
	// this the release of the key that opened it would immediately confirm it.
	flushJoystick();

    bool yesno = defaultval;
    bool dialogRunning = true;
    Sint32 dialogBlinkVal = 50;
    Sint32 dialogBlinkDir = 1;

	if(enableColor3D) {
		BS_Set3DMode(COLOR3D_NONE);
	}
	
    while(dialogRunning) {

        Uint32 tick = BS_GetTicks();

    	// Delay a bit if we're too fast
    	while (tick <= gLastTick) {
    		SDL_Delay(1);
    		tick = BS_GetTicks();
    	}



	// "Physics"-Engine
    	while (gLastTick < tick) {
    		gLastTick += 1000 / (PHYSICSFPS*4);

            dialogBlinkVal += dialogBlinkDir;
            if(dialogBlinkVal == 255) {
                dialogBlinkDir = -1;
            } else if (dialogBlinkVal == 0) {
                dialogBlinkDir = 1;
            }
    	}

        // Blit the background
        SDL_BlitSurface(GUI_Surface, 0, gScreen, 0);

        // Now we must lock the screen for drawing
    	if (SDL_MUSTLOCK(gScreen))
    		if (SDL_LockSurface(gScreen) < 0)
    			return false;

        // Make the dialog blends
        blend_darkenRect(50, 100, SCR_WIDTH - 100, SCR_HEIGHT - 250, 0x00303030);
        blend_brightenRect(50, 100, SCR_WIDTH - 100, SCR_HEIGHT - 250, 0x00101010);
        blend_darkenRect(150, SCR_HEIGHT - 240, 100, 40, 0x00606060);
        blend_darkenRect(400, SCR_HEIGHT - 240, 100, 40, 0x00606060);

		// Now we must unlock the screen after drawing
    	if (SDL_MUSTLOCK(gScreen))
    		SDL_UnlockSurface(gScreen);

		// Draw the lines
        drawrect(50, 100, 2, SCR_HEIGHT - 250, 0x0000cc00);
        drawrect(50, 100, SCR_WIDTH - 100, 2, 0x0000cc00);
        drawrect(SCR_WIDTH - 52, 100, 2, SCR_HEIGHT - 250, 0x0000cc00);
        drawrect(50, SCR_HEIGHT - 152, SCR_WIDTH - 100, 2, 0x0000cc00);

        // Draw the YES button frame
        if(yesno) {
            blinkcolor = dialogBlinkVal + dialogBlinkVal * 256 + dialogBlinkVal * 65536;
        } else {
            blinkcolor = 0x00505050;
        }

        drawrect(150, SCR_HEIGHT - 240, 2, 40, blinkcolor);
        drawrect(150, SCR_HEIGHT - 240, 100, 2, blinkcolor);
        drawrect(248, SCR_HEIGHT - 240, 2, 40, blinkcolor);
        drawrect(150, SCR_HEIGHT - 200, 100, 2, blinkcolor);

        // Draw the NO button frame
        if(!yesno) {
            blinkcolor = dialogBlinkVal + dialogBlinkVal * 256 + dialogBlinkVal * 65536;
        } else {
            blinkcolor = 0x00505050;
        }
        drawrect(400, SCR_HEIGHT - 240, 2, 40, blinkcolor);
        drawrect(400, SCR_HEIGHT - 240, 100, 2, blinkcolor);
        drawrect(498, SCR_HEIGHT - 240, 2, 40, blinkcolor);
        drawrect(400, SCR_HEIGHT - 200, 100, 2, blinkcolor);

        // Draw the texts
        renderFontHandlerText(0, 150, line1, GUICOLOR_ACTIVE, true, false, FONT_menufont_20);
        renderFontHandlerText(0, 190, line2, GUICOLOR_ACTIVE, true, false, FONT_menufont_20);

        renderFontHandlerText(170, SCR_HEIGHT - 228, "yes", GUICOLOR_ACTIVE, false, false, FONT_menufont_20);
        renderFontHandlerText(430, SCR_HEIGHT - 228, "no", GUICOLOR_ACTIVE, false, false, FONT_menufont_20);

        // Tell SDL to update the whole screen
	   BS_Flip(gScreen);

        		// Handle Keyboard
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_KEYUP:
					CHECK_BOSSKEY ;
                    if ((event.key.keysym.sym == SDLK_UP) ||
                        (event.key.keysym.sym == SDLK_DOWN) ||
                        (event.key.keysym.sym == SDLK_LEFT) ||
                        (event.key.keysym.sym == SDLK_RIGHT)) {
                            yesno = !yesno;
                            soundPlayFX(FX_MENU);
					} else if (event.key.keysym.sym == SDLK_RETURN) {
						soundPlayFX(FX_MENU);
						dialogRunning = false;
					}
					break;
			}
		}

		// Handle Joystick (on release; see getJoystickReleases())
		Uint32 joymove = getJoystickReleases();
		if((joymove & JOYSTICK_ACTION)) {
			soundPlayFX(FX_MENU);
			dialogRunning = false;
		} else if(joymove != JOYSTICK_NONE) {
			yesno = !yesno;
			soundPlayFX(FX_MENU);
		}
    }
    return yesno;
}

/*
void drawGui(Sint32 xoffs, Sint32 yoffs) {

	// Gui
	xoffs = xoffs % (GUI_Surface->w - SCR_WIDTH);
	yoffs = yoffs % (GUI_Surface->h - SCR_HEIGHT);

	SDL_Rect src, dest;
	dest.x = dest.y = src.x = src.y = 0;
	src.w = dest.w = SCR_WIDTH;
	src.h = dest.h = SCR_HEIGHT;
	src.x = xoffs;
	src.y = yoffs;

	// !!! During BLIT, surfaces are MUST NOT BE LOCKED !!!
	// Unlock Surface if needed
	if (SDL_MUSTLOCK(gScreen))
		SDL_UnlockSurface(gScreen);


	SDL_BlitSurface(GUI_Surface, &src, gScreen, &dest);

	// Re-lock screen after BLIT
	if (SDL_MUSTLOCK(gScreen))
		if (SDL_LockSurface(gScreen) < 0)
			return;

}
*/
