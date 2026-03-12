#pragma once
#include "pal_types.h"
#include <stdbool.h>

// FX_SOUNDS enum is defined in game/sound.h (kept unchanged)
// PAL audio functions use uint32_t for fx index to avoid enum redefinition

#define PAL_MAX_FX_SAMPLES 100

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the audio system (call bsp_audio_initialize before this)
void PAL_AudioInit(void);
void PAL_AudioDeInit(void);

// Background music: MP3 file on SD card
void PAL_SoundStartMusic(const char* fname, bool playOnce, bool fullpathname);
void PAL_SoundStopMusic(void);
bool PAL_SoundPlayOnceFinished(void);
void PAL_SoundMusicFinished(void);

// Sound FX by index (FX_SOUNDS values from game/sound.h)
void PAL_SoundPlayFX(uint32_t fx);

// Dynamic FX from Lua
uint32_t PAL_SoundAddFX(const char* fname);
void     PAL_SoundPlayFXLua(uint32_t fx);
void     PAL_InitSoundFXLua(void);
void     PAL_DeInitSoundFXLua(void);

// Video FX stubs (not used on Tanmatsu)
static inline void PAL_SoundStartVideoFX(SDL_RWops* src) { (void)src; }
static inline void PAL_SoundStopVideoFX(void) {}

#ifdef __cplusplus
}
#endif
