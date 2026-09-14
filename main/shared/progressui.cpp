// BlinkenSisters - Tanmatsu port
//
// Themed progress screen for downloads and unpacking; see progressui.h.
//
// The look follows the original HTTP_showProgress() (LostPixels/httpclient.cpp
// upstream): a 600x50 bar, darkened backing, progressbar.png revealed from the
// left as it fills with its green key left transparent, and a light outline,
// over downloadbg.png or decrunchingbg.png. Moved from 640x480 coordinates to
// the centre of this 800-wide screen, with two lines of text added under it.

#include "progressui.h"
#include "drawprimitives.h"
#include "blending.h"
#include "bsscreen.h"
#include "fonthandler.h"
#include "convert.h"
#include "config.h"
#include "fastopen.h"

#include <stdio.h>
#include <string.h>

#define BAR_W 600
#define BAR_H 50
#define BAR_X ((SCR_WIDTH - BAR_W) / 2)
#define BAR_Y 250
#define REDRAW_MS 150

static SDL_Surface* bgSurface = 0;
static SDL_Surface* barSurface = 0;
static Sint32 lastFill = -1;
static Uint32 lastDrawTick = 0;
static char lastLine1[160];
static char lastLine2[160];

static bool fileExists(const char* path) {
	FILE* fh = fastopen(path, "rb");
	if (!fh) return false;
	fastclose(fh);
	return true;
}

void progressUIBegin(PROGRESSUI_KIND kind) {
	progressUIEnd();
#ifndef DISABLE_BACKGROUND_ART
	// Only try what is there: a failed load logs a warning, and on a first
	// start none of this has been unpacked yet.
	const char* bg = (kind == PROGRESSUI_DOWNLOAD) ? "downloadbg.png" : "decrunchingbg.png";
	if (fileExists(configGetPath(bg))) {
		bgSurface = BS_IMG_Load_Fullscreen(configGetPath(bg), IGNORE_FILE_ERROR);
	}
	if (fileExists(configGetPath("progressbar.png"))) {
		SDL_Surface* raw = IMG_Load(configGetPath("progressbar.png"));
		if (raw) {
			barSurface = convertToBSSurface(raw);   // green key -> transparent
			SDL_FreeSurface(raw);
		}
	}
#else
	(void)kind;
#endif
	lastFill = -1;
	lastDrawTick = 0;
	lastLine1[0] = 0;
	lastLine2[0] = 0;
}

void progressUIEnd() {
	SDL_FreeSurface(bgSurface);
	SDL_FreeSurface(barSurface);
	bgSurface = 0;
	barSurface = 0;
}

void progressUIDraw(uint64_t done, uint64_t total, const char* line1, const char* line2, bool force) {
	if (!line1) line1 = "";
	if (!line2) line2 = "";
	if (done > total) done = total;
	Sint32 fill = total ? (Sint32)((done * BAR_W) / total) : 0;

	Uint32 now = SDL_GetTicks();
	bool changed = fill != lastFill || strcmp(line1, lastLine1) || strcmp(line2, lastLine2);
	if (!force && (!changed || (now - lastDrawTick) < REDRAW_MS)) {
		return;
	}
	lastFill = fill;
	lastDrawTick = now;
	snprintf(lastLine1, sizeof(lastLine1), "%s", line1);
	snprintf(lastLine2, sizeof(lastLine2), "%s", line2);

	if (bgSurface) {
		SDL_BlitSurface(bgSurface, NULL, gScreen, NULL);
	} else {
		drawrect(0, 0, SCR_WIDTH, SCR_HEIGHT, 0x000000);
	}

	blend_darkenRect(BAR_X, BAR_Y, BAR_W, BAR_H, 0x00a0a0a0);
	if (fill > 0) {
		blend_darkenRect(BAR_X, BAR_Y, fill, BAR_H, 0x00808080);
		if (barSurface) {
			SDL_Rect src = { 0, 0, (Uint16)fill, (Uint16)BAR_H };
			SDL_Rect dst = { (Sint16)BAR_X, (Sint16)BAR_Y, (Uint16)fill, (Uint16)BAR_H };
			SDL_BlitSurface(barSurface, &src, gScreen, &dst);
		} else {
			drawrect(BAR_X, BAR_Y, fill, BAR_H, 0x2060c0);
		}
	}
	drawrect(BAR_X, BAR_Y, BAR_W, 1, 0xd0d0d0);
	drawrect(BAR_X, BAR_Y, 1, BAR_H, 0xd0d0d0);
	drawrect(BAR_X, BAR_Y + BAR_H, BAR_W, 1, 0xd0d0d0);
	drawrect(BAR_X + BAR_W - 1, BAR_Y, 1, BAR_H, 0xd0d0d0);

	// The artwork is busy; give the text a quiet strip to sit on.
	blend_darkenRect(BAR_X - 20, BAR_Y + BAR_H + 10, BAR_W + 40, 70, 0x00808080);
	renderFontHandlerText(0, BAR_Y + BAR_H + 15, line1, BS_Color_WHITE, true, false, FONT_textfont_20);
	renderFontHandlerText(0, BAR_Y + BAR_H + 45, line2, BS_Color_WHITE, true, false, FONT_textfont_20);

	BS_Flip(gScreen);
}

void progressUIFormatBytes(uint64_t bytes, char* out, size_t outlen) {
	if (bytes >= 1000000ULL) {
		snprintf(out, outlen, "%.1f MB", (double)bytes / 1e6);
	} else if (bytes >= 1000ULL) {
		snprintf(out, outlen, "%.0f KB", (double)bytes / 1e3);
	} else {
		snprintf(out, outlen, "%u bytes", (unsigned)bytes);
	}
}
