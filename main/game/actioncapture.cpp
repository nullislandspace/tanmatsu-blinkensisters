// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#include "globals.h"

#ifdef ALLOW_ACTIONCAPTURE
#include "actioncapture.h"
#include "fonthandler.h"
#include "blending.h"
#include "engine.h"

SDL_Surface* gActionCapSurface[ACTIONCAPTURE_MAX];
Uint32 gActionCapLastTick = 0;
Uint32 gActionCapCount = 0;
bool gActionCapEnabled = false;

void initActionCapture() {
	printf("Initalizing ActionCapture\n");
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
	
	for(Uint32 i = 0; i < ACTIONCAPTURE_MAX; i++) { 
		SDL_Surface* temp = SDL_CreateRGBSurface(SDL_SWSURFACE, ACTIONCAPTURE_WIDTH, ACTIONCAPTURE_HEIGHT, 32,
												 rmask, gmask, bmask, amask);
		gActionCapSurface[i] = SDL_DisplayFormat(temp);
		SDL_FreeSurface(temp);
		
	}
}

void startActionCapture() {
	if(gActionCapEnabled) {
		return;
	}
	printf("Starting ActionCapture...\n");
	gActionCapEnabled = true;
	gActionCapCount = 0;
	gActionCapLastTick = BS_GetTicks() + ACTIONCAPTURE_DELAY;
}

void stopActionCapture() {
	if(!gActionCapEnabled) {
		return;
	}
	printf("Canceling ActionCapture!\n");
	gActionCapEnabled = false;
}

void renderActionCapture(const Uint32 spriteoffsx, const Uint32 spriteoffsy) {
	if(!gActionCapEnabled) {
		return;
	}
	
	Uint32 ticks = BS_GetTicks();
	
	if(ticks < gActionCapLastTick) {
		return;
	}
	//printf("ActionCapture: Capturing image...\n");
	
	// Calculate offset in 
	Sint32 xoffs = spriteoffsx - ACTIONCAPTURE_WIDTH / 2;
	Sint32 yoffs = spriteoffsy - ACTIONCAPTURE_HEIGHT / 2;
	
	// Limit capture when sprite if near borders
	if(xoffs < 0) {
		xoffs = 0;
	} else if(xoffs > (SCR_WIDTH - ACTIONCAPTURE_WIDTH - 1)) {
		xoffs = SCR_WIDTH - ACTIONCAPTURE_WIDTH - 1;
	}
	if(yoffs < 0) {
		yoffs = 0;
	} else if(yoffs > (SCR_HEIGHT - ACTIONCAPTURE_HEIGHT - 1)) {
		yoffs = SCR_HEIGHT - ACTIONCAPTURE_HEIGHT - 1;
	}

	gActionCapLastTick = ticks + ACTIONCAPTURE_DELAY;

	if (SDL_MUSTLOCK(gActionCapSurface[gActionCapCount])) 
		SDL_UnlockSurface(gActionCapSurface[gActionCapCount]);

	SDL_BlitSurface( gScreen, NULL, gActionCapSurface[gActionCapCount], NULL );
	
	gActionCapCount++;
	
	if(gActionCapCount == ACTIONCAPTURE_MAX) {
		printf("ActionCapture: Saving Image sequence...\n");
		gActionCapEnabled = false;
		// Write files
		char tmpfname[255];
		for(Uint32 i=0; i < ACTIONCAPTURE_MAX; i++) {
			sprintf(tmpfname, "%sACTIONCAPTURE_%0d.bmp", configGetPath(""), i);
			if (SDL_MUSTLOCK(gActionCapSurface[i]))
				if (SDL_LockSurface(gActionCapSurface[i]) < 0) 
					return;
			
			// Fade in + Fade out
			//Uint32 = 255 << 16 + 255 << 8 + 255;
			Uint32 fader = 255;
			if(i < 16) {
				fader = i * 16;
			} else if(i > ACTIONCAPTURE_MAX - 17) {
				fader = (ACTIONCAPTURE_MAX - i - 1) * 16;
			}
			
			if(fader < 255) {
				printf("FADER: %d\n", fader);
				Uint32 multi = (fader << 16) + (fader << 8) + fader;
				for(Uint32 x = 0; x < ACTIONCAPTURE_WIDTH; x++) {
					for(Uint32 y = 0; y < ACTIONCAPTURE_HEIGHT; y++) {
						Uint32 pixel = BS_Unpack(gActionCapSurface[i]->pixels[x + y*gActionCapSurface[i]->w]);
						pixel = blend_mul(pixel, multi);
						gActionCapSurface[i]->pixels[x + y*gActionCapSurface[i]->w] = BS_PackOpaque(pixel);
					}
				}
			}
						
			// Add copyright-information
			renderFontHandlerText(20, ACTIONCAPTURE_HEIGHT - 20, "(C) The Blinkensisters Team", BS_Color_WHITE, false, false, FONT_textfont_12);

						
			
			SDL_SaveBMP(gActionCapSurface[i], tmpfname);
			if (SDL_MUSTLOCK(gActionCapSurface[i])) 
				SDL_UnlockSurface(gActionCapSurface[i]);
		}
		gActionCapEnabled = false;
		printf("ActionCapture: Image sequence saved!\n");
	}
}

void deInitActionCapture() {
	printf("De-Initalizing ActionCapture\n");
	for(Uint32 i = 0; i < ACTIONCAPTURE_MAX; i++) {
		SDL_FreeSurface(gActionCapSurface[i]);
	}
}

#endif // ALLOW_ACTIONCAPTURE
