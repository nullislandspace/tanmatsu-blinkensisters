// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#include "globals.h"
#include "convert.h"

SDL_Surface* convertToBSSurface(SDL_Surface* src) {
    
    
    SDL_Surface* dest = SDL_DisplayFormatAlpha(src);
    
    Uint32 color;
    Uint32 pitch = dest->pitch / 4;
	for(Sint32 y = 0; y < dest->h; y++) { 
		for(Sint32 x = 0; x < dest->w; x++) {
			color = ((Uint32*)dest->pixels)[x + y * pitch];
			
			if((color & 0x00ffffff) == 0x0000ff00) {
			     ((Uint32*)dest->pixels)[x + y * pitch] = 0x00000000;
			}
		}
	}
	
	return dest;
    
}
