// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#ifndef BSGUI_H
#define BSGUI_H

void initGui();
void deInitGui();
//void drawGui(Sint32 xoffs, Sint32 yoffs);
bool guiYesNoDialog(const char* line1, const char* line2, bool defaultval);

#endif // BSGUI_H
