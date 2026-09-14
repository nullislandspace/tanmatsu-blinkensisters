// BlinkenSisters - Tanmatsu port
//
// The original game's themed progress screen -- "Downloading..." or
// "Decrunching..." graffiti with the blue-octopus progress bar -- used for
// addon downloads and for unpacking archives.

#ifndef PROGRESSUI_H
#define PROGRESSUI_H

#include "globals.h"
#include <stdint.h>

typedef enum {
	PROGRESSUI_DOWNLOAD = 0,
	PROGRESSUI_UNPACK,
} PROGRESSUI_KIND;

// Load the artwork for `kind`. Works without it, too: the art lives in
// basedata.bmf, which is not there yet on a first start, and the screen then
// falls back to a plain bar.
void progressUIBegin(PROGRESSUI_KIND kind);

// Redraw with `done` of `total`, and two lines of text under the bar. Cheap to
// call often: it only draws when the bar or the text has changed and a
// fraction of a second has passed, unless `force` is set.
void progressUIDraw(uint64_t done, uint64_t total, const char* line1, const char* line2, bool force);

void progressUIEnd();

// Human-readable byte counts for progress text ("43.1 MB").
void progressUIFormatBytes(uint64_t bytes, char* out, size_t outlen);

#endif // PROGRESSUI_H
