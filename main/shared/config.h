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

/* Archives. configFindArchive looks for `file` among the downloaded archives,
   then in the app's own directory, and says which. configExtractArchive
   unpacks one if its stamp shows it changed (always, with force) and returns
   whether it did; configForgetArchive drops the stamp. */
bool configFindArchive(const char* file, char* out, size_t outlen, bool* downloaded);
bool configExtractArchive(const char* bmfpath, bool force);
void configForgetArchive(const char* file);
/* Whether an addon's data (by its ADDON/ directory) or the base data is
   unpacked and usable. */
bool configAddonInstalled(const char* dir);
bool configBaseDataInstalled();

/* concatenate RESPATH and filename - or if possible do it relocatable */
char* getrespath(const char* filename);


#endif // CONFIG_H
