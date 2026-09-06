#include "osdef.h"
#include <string.h>
#include <stdlib.h>

extern "C" {
#include "bsp/device.h"
}
#include "../pal/pal_audio.h"

char* BS_strdup(const char* s) {
    char* result = (char*)malloc(strlen(s) + 1);
    if (result == NULL) return NULL;
    strcpy(result, s);
    return result;
}

void quitToLauncher(void) {
    // Silence the audio hardware first; the restart does not stop the codec.
    PAL_SoundStopMusic();
    PAL_AudioDeInit();
    bsp_device_restart_to_launcher();
}
