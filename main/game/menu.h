// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#ifndef MENU_H
#define MENU_H

void initMenu();
void deInitMenu();
bool menuDisplay();
bool menuOnlineDisplay();
bool menuAddonDisplay();
void menuShowNeedHelp();
void menuPerformanceTestMode();
void menuAttrackMode();

extern SDL_Color MENUCOLOR_ACTIVE;
extern SDL_Color MENUCOLOR_INACTIVE;


#endif // MENU_H
