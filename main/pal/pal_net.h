// BlinkenSisters - Tanmatsu port
// Network access for the addon downloader: WiFi through the Tanmatsu radio
// coprocessor, using the networks saved in the launcher, and HTTPS.

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Bring the radio and the WiFi stack up. Call once from app_main, before the
// SD card is mounted (the radio's SDIO init must come first), and from a task
// with an internal-RAM stack. Does not connect to anything.
void PAL_NetBootInit(void);

typedef enum {
    PAL_NET_IDLE = 0,       // not asked to connect yet
    PAL_NET_CONNECTING,
    PAL_NET_CONNECTED,
    PAL_NET_NO_NETWORKS,    // the launcher has no WiFi network saved
    PAL_NET_FAILED,         // none of the saved networks could be joined
    PAL_NET_UNAVAILABLE,    // the WiFi stack did not come up at boot
} PAL_NetState;

// Start connecting in the background, unless connected or already trying.
void PAL_NetStartConnect(void);
PAL_NetState PAL_NetGetState(void);

// GET a small resource into a NUL-terminated PSRAM buffer (free with free()).
bool PAL_NetFetch(const char* url, char** body, size_t* len, size_t maxlen,
                  char* err, size_t errlen);

// Called as data arrives; return false to stop. `done` counts bytes of the
// file, including any resumed from an earlier attempt.
typedef bool (*PAL_NetProgress)(void* ctx, uint64_t done, uint64_t total);

// Download `url` to `dest`, which must end up exactly `size` bytes with the
// given SHA-256 (64 hex digits). Data goes to `dest`.part first and the file
// only takes its real name once verified. An interrupted or cancelled download
// keeps its .part and resumes from it next time (HTTP Range); a download that
// arrives complete but wrong is deleted.
bool PAL_NetDownload(const char* url, const char* dest, uint64_t size, const char* sha256hex,
                     PAL_NetProgress progress, void* ctx, char* err, size_t errlen);
