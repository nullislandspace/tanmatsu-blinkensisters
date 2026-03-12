// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-11 The Blinkensisters team
//
// See License.txt for licensing information


#ifndef CONFIG_H
#define CONFIG_H

#include "globals.h"
#include <limits.h>

extern char currentAddonName [PATH_MAX] ;
/* configInit() must be called before using the other functions! */
void configInit(const bool forceUpdate);
void configSetBasePath(const char *path);
char* configGetPath(const char* fname);
void configSetAddOn(const char* addOnName);
# define configResetAddOn() configSetAddOn("")
void configStartupComplete();

/* concatenate RESPATH and filename - or if possible do it relocatable */
char* getrespath(const char* filename);


#endif // CONFIG_H
