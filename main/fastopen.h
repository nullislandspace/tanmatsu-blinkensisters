#pragma once

#include <stdio.h>

// fopen/fclose for files on /sd and /int with a cache-aligned PSRAM stdio
// buffer, which the SDMMC driver can read into directly and which takes no
// internal RAM (see fastopen.c). Close with fastclose(), which frees it.

#ifdef __cplusplus
extern "C" {
#endif

FILE* fastopen(const char* path, const char* mode);
void  fastclose(FILE* f);

#ifdef __cplusplus
}
#endif
