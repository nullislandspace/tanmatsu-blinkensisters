// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#include "globals.h"
#include "debug.h"
#include "fonthandler.h"

Uint32 fpsLastTick;
Uint32 fpsFrameCount;
Uint32 fpsLastFrameCount;
char fpsText[20];
SDL_Color FPSINFO_COLOR       = { 0xe0, 0xe0, 0xe0, 0 };

void initFPSCounter() {
	fpsLastTick = SDL_GetTicks() + 1000;
	fpsFrameCount = 0;
	fpsLastFrameCount = 0;
	sprintf(fpsText, "FPS:");
}

void displayFPSCounter(bool displayOnly) {
	if(!displayOnly) {
		Uint32 tick = SDL_GetTicks();
		fpsFrameCount++;
		if(fpsLastTick < tick) {
			fpsLastFrameCount = fpsFrameCount;
			fpsFrameCount = 0;
			fpsLastTick = tick + 1000;
			sprintf(fpsText, "FPS: %d", fpsLastFrameCount);
		}
	}
	renderFontHandlerText(SCR_WIDTH-90, SCR_HEIGHT-30, fpsText, FPSINFO_COLOR, false, false, FONT_textfont_20);

}
		
