// BlinkenSisters - Tanmatsu port - OS abstraction
#ifndef OSDEPS_H
#define OSDEPS_H

#include <math.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

// mkdir macro
#define MKDIR(fname) mkdir((fname), S_IRWXU)

// strdup replacement
char* BS_strdup(const char* s);

// Terminate the game and hand control back to the badge launcher. This is the
// only way this app can "exit": there is no process to leave, and running the
// libc exit() path from a FreeRTOS task never completes.
void quitToLauncher(void);

// round() is available in C99/ESP-IDF
#ifndef round
#define round(x) floor((x) + 0.5)
#endif

#endif // OSDEPS_H
