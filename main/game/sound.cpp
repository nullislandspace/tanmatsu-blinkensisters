// Tanmatsu port: sound.cpp replaced by PAL audio
#include "globals.h"   // defines Uint32, SDL_RWops etc before sound.h
#include "sound.h"
#include "pal/pal_audio.h"
#include <string.h>

bool soundOK = true;
bool soundInitOK = false;

void initSound() {
    PAL_AudioInit();
    soundInitOK = true;
}

void loadSoundFX() {
    PAL_SoundLoadPredefFX();
}

void deInitSound() {
    PAL_AudioDeInit();
    soundInitOK = false;
}

void soundStartMusic(const char* fname, bool playOnce, bool fullpathname) {
    PAL_SoundStartMusic(fname, playOnce, fullpathname);
}

void soundStopMusic() {
    PAL_SoundStopMusic();
}

void soundPlayFX(Uint32 fx) {
    PAL_SoundPlayFX(fx);
}

void soundMusicFinished() {
    PAL_SoundMusicFinished();
}

Uint32 addSoundFXLua(char* fname) {
    return PAL_SoundAddFX(fname);
}

void soundPlayFXLua(Uint32 fx) {
    PAL_SoundPlayFXLua(fx);
}

void initSoundFXLua() {
    PAL_InitSoundFXLua();
}

void deInitSoundFXLua() {
    PAL_DeInitSoundFXLua();
}

void soundStartVideoFX(SDL_RWops* src) { (void)src; }
void soundStopVideoFX() {}

bool soundPlayOnceFinished() {
    return PAL_SoundPlayOnceFinished();
}
