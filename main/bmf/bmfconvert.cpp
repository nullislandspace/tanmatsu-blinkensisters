// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information

#include "bmfconvert.h"

void bmfWriteInt(FILE* fh, unsigned int len) {
	unsigned char tmp[4];

	tmp[0] = (char)(len & 0xff);
	tmp[1] = (char)((len >> 8) & 0xff);
	tmp[2] = (char)((len >> 16) & 0xff);
	tmp[3] = (char)((len >> 24) & 0xff);
	
	fwrite(tmp, 4, 1, fh);
}

unsigned int bmfReadInt(FILE* fh) {
	unsigned char tmp[4];
	unsigned int len = 0;

	fread(tmp, 4, 1, fh);

	len = (tmp[0] & 0xff) +
		  ((tmp[1] & 0xff) << 8) +	
		  ((tmp[2] & 0xff) << 16) +	
		  ((tmp[3] & 0xff) << 24);
	
	//printf("Read int %x / %u...\n", len, len);

	return len;
}	
