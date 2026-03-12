// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#ifndef BACKGROUND2_H
#define BACKGROUND2_H

void initBackground2(const char *fname);
void deInitBackground2();
void drawBackground2(Sint32 xoffs, Sint32 yoffs);
Uint32 getBG2Width();
Uint32 getBG2Height();

#endif // BACKGROUND2_H
