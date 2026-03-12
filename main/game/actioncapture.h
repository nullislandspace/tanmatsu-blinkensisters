// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#ifdef ALLOW_ACTIONCAPTURE

#ifndef ACTIONCAPTURE_H
#define ACTIONCAPTURE_H

void initActionCapture();
void startActionCapture();
void stopActionCapture();
void renderActionCapture(const Uint32 spriteoffsx, const Uint32 spriteoffsy);
void deInitActionCapture();

#endif // ACTIONCAPTURE_H

#endif // ALLOW_ACTIONCAPTURE
