#include "fastopen.h"
#include <string.h>
#include <stdlib.h>
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"

/*
 * fastopen() gives files on the SD card a stdio buffer in PSRAM, aligned to the
 * cache line, instead of the one newlib would malloc itself.
 *
 * That default buffer is 8 KB (CONFIG_FATFS_VFS_FSTAT_BLKSIZE) and counts as a
 * small allocation, so it is taken from internal RAM -- which is short here --
 * and FatFs reads whole sectors straight into it by DMA. When internal SRAM had
 * no free 8 KB block it came from RTC fast memory instead, which the SDMMC DMA
 * path cannot use: the read failed ("esp_cache_msync: invalid addr"), and the
 * loaders reported a missing image or, worse, "out of memory". PSRAM is DMA
 * capable on the P4, so a cache-aligned PSRAM buffer is both safe and
 * zero-copy, and it takes no internal RAM at all.
 *
 * This used to be compiled out: it hung off CONFIG_FATFS_USE_FASTOPEN, which is
 * not a Kconfig symbol, so setting it in sdkconfigs/ never reached the build.
 */

#define FASTOPEN_BUF_SIZE  8192
#define FASTOPEN_MAX_FILES 16

typedef struct {
    FILE* file;
    void* buffer;
} fast_file_entry_t;

static fast_file_entry_t fast_file_table[FASTOPEN_MAX_FILES];
static portMUX_TYPE      fast_file_lock = portMUX_INITIALIZER_UNLOCKED;

static bool path_needs_fast_io(const char* path) {
    return (strncmp(path, "/sd", 3) == 0) || (strncmp(path, "/int", 4) == 0);
}

FILE* fastopen(const char* path, const char* mode) {
    FILE* f = fopen(path, mode);
    if (f == NULL || !path_needs_fast_io(path)) return f;

    void* buf = heap_caps_malloc(FASTOPEN_BUF_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_CACHE_ALIGNED);
    if (buf == NULL) return f;   // newlib's own buffer then; still works

    bool tracked = false;
    taskENTER_CRITICAL(&fast_file_lock);
    for (int i = 0; i < FASTOPEN_MAX_FILES; i++) {
        if (fast_file_table[i].file == NULL) {
            fast_file_table[i].file   = f;
            fast_file_table[i].buffer = buf;
            tracked = true;
            break;
        }
    }
    taskEXIT_CRITICAL(&fast_file_lock);

    if (!tracked || setvbuf(f, buf, _IOFBF, FASTOPEN_BUF_SIZE) != 0) {
        // Untrackable, or the stream refused it: leave newlib's buffer alone.
        if (tracked) {
            taskENTER_CRITICAL(&fast_file_lock);
            for (int i = 0; i < FASTOPEN_MAX_FILES; i++) {
                if (fast_file_table[i].file == f) fast_file_table[i].file = NULL;
            }
            taskEXIT_CRITICAL(&fast_file_lock);
        }
        heap_caps_free(buf);
    }
    return f;
}

void fastclose(FILE* f) {
    if (f == NULL) return;

    void* buf = NULL;
    taskENTER_CRITICAL(&fast_file_lock);
    for (int i = 0; i < FASTOPEN_MAX_FILES; i++) {
        if (fast_file_table[i].file == f) {
            buf = fast_file_table[i].buffer;
            fast_file_table[i].file   = NULL;
            fast_file_table[i].buffer = NULL;
            break;
        }
    }
    taskEXIT_CRITICAL(&fast_file_lock);

    // Close first: fclose flushes through the buffer.
    fclose(f);
    heap_caps_free(buf);
}
