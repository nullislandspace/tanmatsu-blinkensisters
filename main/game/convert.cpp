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
    
    /* Pure green is the layer's transparency key; with RGB565 there is no
       alpha channel to clear, so it becomes the reserved transparent colour. */
    const BS_Pixel green = BS_Pack(0x0000ff00u);
    Uint32 pitch = dest->w;
	for(Sint32 y = 0; y < dest->h; y++) { 
		for(Sint32 x = 0; x < dest->w; x++) {
			if(dest->pixels[x + y * pitch] == green) {
			     dest->pixels[x + y * pitch] = BS_TRANSPARENT;
			}
		}
	}
	
	return dest;
    
}
