// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#ifndef SOUND_H
#define SOUND_H

typedef enum _FX_SOUNDS {
	FX_COLLECT_PIXEL = 0,
	FX_KILL_MONSTER,
	FX_KILL_PLAYER,
	FX_LEVEL_FINISHED,
	FX_MENU,
	FX_RESPAWNPOINT,
	FX_MAX_PREDEF
} FX_SOUNDS;

void initSound();
void soundStartMusic(const char* fname, bool playOnce = false, bool fullpathname = false);
void soundStopMusic();
void soundPlayFX(Uint32 fx);
void deInitSound();
void soundMusicFinished();

Uint32 addSoundFXLua(char* fname);
void soundPlayFXLua(Uint32 fx);
void initSoundFXLua();
void deInitSoundFXLua();
void soundStartVideoFX(SDL_RWops* src);
void soundStopVideoFX();

bool soundPlayOnceFinished();


#endif // SOUND_H
