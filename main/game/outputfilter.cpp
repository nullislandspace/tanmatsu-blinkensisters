// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#include "globals.h"
#include "outputfilter.h"
#include "drawprimitives.h"
#include "SDL_gfxPrimitives.h"

Uint32 outputfiltertype = OUTPUTFILTER_NONE;
Uint32 outputfilterfading = 255;

void initOutputFilter()
{
    outputfiltertype = OUTPUTFILTER_NONE;
	outputfilterfading = 255;
}

void deInitOutputFilter() 
{

}

void setOutputFilter(const Uint32 type)
{
    outputfiltertype = type;
       
}

Uint32 getOutputFilter()
{
    return outputfiltertype;
}

void setSpecificOutputFilter(const Uint32 type, bool isSet) {
    if(isSet) {
        if(!(outputfiltertype & type)) {
            outputfiltertype += type;
        }
    } else {
        if(outputfiltertype & type) {
            outputfiltertype -= type;
        }
    }
    return;
}

bool getSpecificOutputFilter(const Uint32 type) {
    if(outputfiltertype & type) {
        return true;
    }
    return false;
}


void setOutputFilterFading(const Uint32 fading) {
	outputfilterfading = fading;
	if(fading < 255) {
		outputfiltertype = outputfiltertype | OUTPUTFILTER_FADING;
	} else if((outputfiltertype & OUTPUTFILTER_FADING)) {
		outputfiltertype -= OUTPUTFILTER_FADING;
	}
}

Uint32 getOutputFilterFading() {
    return outputfilterfading;
}


void applyOutputFilter()
{
    if(outputfiltertype == OUTPUTFILTER_NONE) {
        return;
    }
    
    	Uint32 pitch = gScreen->w;	
	Uint32 i, j;
	Uint32 pixel;
	Uint32 r,g,b;

    if((outputfiltertype & OUTPUTFILTER_GREYSCALE) || (outputfiltertype & OUTPUTFILTER_COLORINVERT) || (outputfiltertype & OUTPUTFILTER_FADING)) {
    	// Whole-screen pixel manipulations
    	for(i=0; i < (unsigned int)SCR_WIDTH; i++) {
    		for(j=0; j < (unsigned int)SCR_HEIGHT; j++) {
    			pixel = BS_Unpack(gScreen->pixels[j * pitch + i]);
    			
  				r = pixel & 0xff;
	            g = (pixel /  256) & 0xff;
            	b = (pixel / 65535) & 0xff;
            	
            	if((outputfiltertype & OUTPUTFILTER_COLORINVERT)) {
                    r = 255 - r;
                    g = 255 - g;
                    b = 255 - b;
                }
                if((outputfiltertype & OUTPUTFILTER_GREYSCALE)) {
                    r = g = b = ((r+g+b)/3);
                }
				
				if(outputfilterfading == 0) {
					r = g = b = 0;
				} else if(outputfilterfading < 255) {
					/*
					r = (Uint32)((double)r * fadingfactor);
					g = (Uint32)((double)g * fadingfactor);
					b = (Uint32)((double)b * fadingfactor);
					*/
					r = (r * outputfilterfading) / 255;
					g = (g * outputfilterfading) / 255;
					b = (b * outputfilterfading) / 255;
				}
				
				
                pixel = r + g * 256  + b * 65536; 
                    			
   				gScreen->pixels[j * pitch + i] = BS_PackOpaque(pixel);
    			
    		}
    	}
    }
    
    if((outputfiltertype & OUTPUTFILTER_NOISERECT)) {
        Uint32 count = rand()*20/RAND_MAX + 5;
        
        for(i=0; i < count; i++) {
            int x = rand()*SCR_WIDTH/RAND_MAX;
            int y = rand()*SCR_HEIGHT/RAND_MAX;
            int w = rand()*3/RAND_MAX+1;
            int h = rand()*3/RAND_MAX+1;
            drawrect(x, y, w, h, 0x00ffffff);
        }
    }

    if((outputfiltertype & OUTPUTFILTER_NOISECIRCLE)) {
        Uint32 count = rand()*10/RAND_MAX + 5;
        
        for(i=0; i < count; i++) {
            int x = rand()*SCR_WIDTH/RAND_MAX;
            int y = rand()*SCR_HEIGHT/RAND_MAX;
            int d = rand()*4/RAND_MAX+1;
            circleColor(gScreen, x, y, d, SDL_MapRGB(gScreen->format,0xff,0xff,0xff));
        }
    }
    
    if((outputfiltertype & OUTPUTFILTER_NOISESTRIPE)) {
        Uint32 count = rand()*2/RAND_MAX + 5;
        
        for(i=0; i < count; i++) {
            int x = rand()*SCR_WIDTH/RAND_MAX;
            int y = rand()*SCR_HEIGHT/RAND_MAX;
            int h = rand()*200/RAND_MAX+1;
            drawrect(x, y, 1, h, SDL_MapRGB(gScreen->format,0xff,0xff,0xff));
        }
    }


}
