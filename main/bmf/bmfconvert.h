// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information

#ifndef BMFCONVERT_H
#define BMFCONVERT_H

#include <stdio.h>
#include <stdlib.h>

#define BMF_VERSION 2

#ifndef SCR_WIDTH
#define SCR_WIDTH 640
#endif
#ifndef SCR_HEIGHT
#define SCR_HEIGHT 480
#endif

typedef enum _BMFTYPE {
	BMFTYPE_MAGIC = 0x0b0c0a0e,
	BMFTYPE_VERSION = 0x00000002,
	BMFTYPE_FPS = 0xff000000,
	BMFTYPE_FRAMECOUNT,
	BMFTYPE_WIDTH,
	BMFTYPE_HEIGHT,
	BMFTYPE_FILENAME,
	BMFTYPE_FRAME_COMPRESSED,    // kept for backward compat detection
	BMFTYPE_FRAME_UNCOMPRESSED,
	BMFTYPE_SND_COMPRESSED,      // kept for backward compat detection
	BMFTYPE_SND_UNCOMPRESSED,
	BMFTYPE_METAFILE_COMPRESSED, // kept for backward compat detection
	BMFTYPE_METAFILE_UNCOMPRESSED,
	BMFTYPE_DIR,
	BMFTYPE_REGISTER_ADDON,
	BMFTYPE_REGISTER_MUSIC,
	BMFTYPE_REGISTER_POSCAP,
	BMFTYPE_END_OF_FILE = 0xffffffff
} BMFTYPE;

void bmfWriteInt(FILE* fh, unsigned int len);
unsigned int bmfReadInt(FILE* fh);

#endif // BMFCONVERT_H
