#pragma once
#include "pal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

Uint32 SDL_GetTicks(void);
void   SDL_Delay(Uint32 ms);

#ifdef __cplusplus
}
#endif
