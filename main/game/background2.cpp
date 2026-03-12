// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#include "globals.h"
#include "background2.h"
#include "errorhandler.h"
#include "convert.h"

#ifdef DISABLE_BACKGROUND_ART

void initBackground2(const char *fname) {
	(void)fname;
}

void deInitBackground2() {
}

void drawBackground2(Sint32 xoffs, Sint32 yoffs) {
	(void)xoffs;
	(void)yoffs;
}

Uint32 getBG2Width() {
	return SCR_WIDTH * 2;
}

Uint32 getBG2Height() {
	return SCR_HEIGHT * 2;
}

#else // DISABLE_BACKGROUND_ART

SDL_Surface *BG_Surface2;

void initBackground2(const char *fname) {
	char fullfname[MAX_FNAME_LENGTH];
	sprintf(fullfname, "%s", configGetPath(fname));
	SDL_Surface* temp = IMG_Load(fullfname);
	if(!temp) {
		DIE(ERROR_IMAGE_READ, fullfname);
	}
	SDL_Surface* bgtemp = convertToBSSurface(temp);
	SDL_FreeSurface(temp);
	
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
	
	temp = SDL_CreateRGBSurface(SDL_SWSURFACE, bgtemp->w + SCR_WIDTH, bgtemp->h + SCR_HEIGHT, 32, rmask, gmask, bmask, amask);
	BG_Surface2 = convertToBSSurface(temp);
	SDL_FreeSurface(temp);

	// Lock Surfaces if needed
	if (SDL_MUSTLOCK(BG_Surface2))
		if (SDL_LockSurface(BG_Surface2) < 0) 
			return;
	if (SDL_MUSTLOCK(bgtemp))
		if (SDL_LockSurface(bgtemp) < 0) 
			return;

	Uint32 pitch_tmp = bgtemp->pitch / 4;
	Uint32 pitch_bg = BG_Surface2->pitch / 4;	
	Uint32 i, j;
		
	
	// Copy the background into the bigger surface
	for(i=0; i < (Uint32)bgtemp->w; i++) {
		for(j=0; j < (Uint32)bgtemp->h; j++) {
			((Uint32 *)BG_Surface2->pixels)[j * pitch_bg + i] = 
			((Uint32 *)bgtemp->pixels)[j * pitch_tmp + i];
		}
	}
	
	
	// Replicate the first SCR_WIDTH pixels to the right side of the surface for BlitSurface
	Sint32 ii = 0;
	for(i=0; i < (unsigned int)SCR_WIDTH; i++) {
		ii++;
		if(ii == bgtemp->w) {
			ii = 0;
		}
		for(j=0; j < (Uint32)bgtemp->h; j++) {
			((Uint32 *)BG_Surface2->pixels)[j * pitch_bg + i + bgtemp->w] = 
				((Uint32 *)bgtemp->pixels)[j * pitch_tmp + ii];
		}
	}

	// Replicate the first SCR_HEIGHT pixels to the bottom for BlitSurface
	Sint32 jj = 0;
	for(j = bgtemp->h; j < (Uint32)BG_Surface2->h; j++) {
		for(i = 0; i < (Uint32)BG_Surface2->w; i++) {
			((Uint32 *)BG_Surface2->pixels)[j * pitch_bg + i] = 
				((Uint32 *)BG_Surface2->pixels)[jj * pitch_bg + i];
		}
		jj++;
		if(jj == bgtemp->h) {
		   jj = 0;
		}	   
	}
	
	// Unlock Surfaces if needed
	if (SDL_MUSTLOCK(bgtemp)) 
		SDL_UnlockSurface(bgtemp);
	
//	int err = SDL_SetColorKey(BG_Surface2, SDL_SRCCOLORKEY /*| SDL_RLEACCEL*/, 0x00ff00);
//	if(err) {
//		printf("ERROR: %s\n", SDL_GetError());
//	}	
	
	if (SDL_MUSTLOCK(BG_Surface2)) 
		SDL_UnlockSurface(BG_Surface2);
	
	
	SDL_FreeSurface(bgtemp);
	
	// Call Convert for green-to-alpha
	bgtemp = convertToBSSurface(BG_Surface2);
    SDL_FreeSurface(BG_Surface2);
    BG_Surface2 = bgtemp;
}

void deInitBackground2() {
	SDL_FreeSurface(BG_Surface2);
}

void drawBackground2(Sint32 xoffs, Sint32 yoffs) {
	// Background2
	xoffs = xoffs % (BG_Surface2->w - SCR_WIDTH);
	yoffs = yoffs % (BG_Surface2->h - SCR_HEIGHT);
	
	SDL_Rect src, dest;
	dest.x = dest.y = 0;
	src.w = dest.w = SCR_WIDTH;
	src.h = dest.h = SCR_HEIGHT;
	src.x = xoffs;
	src.y = yoffs;
	
	SDL_BlitSurface(BG_Surface2, &src, gScreen, &dest);
	
}

Uint32 getBG2Width() {
	return BG_Surface2->w;
}

Uint32 getBG2Height() {
	return BG_Surface2->h;
}

#endif // DISABLE_BACKGROUND_ART

