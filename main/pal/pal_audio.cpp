#include "pal_audio.h"
extern "C" {
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/i2s_std.h"
#include "bsp/audio.h"
#include "fastopen.h"
}
#include <string.h>
#include <stdio.h>
#include "shared/config.h"

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ONLY_MP3
#define MINIMP3_NO_SIMD
#include "../minimp3.h"

// Must match FX_SOUNDS enum in game/sound.h
#define FX_MAX_PREDEF   6
// Must match MAX_FX_SAMPLES in shared/globals.h
#define MAX_FX_SAMPLES  100

static const char* TAG = "pal_audio";

// ---- Buffer sizes ----
#define READ_BUF_SIZE   (16 * 1024)   // 16KB in PSRAM
#define PCM_BUF_FRAMES  1152          // max frames per MP3 frame
#define PCM_BUF_SIZE    (PCM_BUF_FRAMES * 2 * sizeof(int16_t))  // stereo

// ---- PCM FX storage ----
typedef struct {
    int16_t* samples;
    size_t   num_samples;
    int      channels;
    int      sample_rate;
} PCM_FX_Entry;

static PCM_FX_Entry s_predef_fx[FX_MAX_PREDEF] = {0};
static PCM_FX_Entry s_lua_fx[MAX_FX_SAMPLES]   = {0};
static Uint32       s_lua_fx_count = 0;

// ---- Music streaming state ----
static volatile bool s_music_playing  = false;
static volatile bool s_music_once     = false;
static volatile bool s_music_finished = false;
static char          s_music_path[512] = {0};
static volatile bool s_new_music       = false;
static volatile bool s_stop_music      = false;

static mp3dec_t*  s_mp3dec     = NULL;
static uint8_t*   s_read_buf   = NULL;
static int16_t*   s_pcm_buf    = NULL; // internal RAM, DMA-capable
static i2s_chan_handle_t s_i2s = NULL;

static TaskHandle_t s_music_task = NULL;
static SemaphoreHandle_t s_music_mutex = NULL;

// ---- Decode a whole MP3 file into PCM in PSRAM ----
static PCM_FX_Entry decode_mp3_file(const char* path) {
    PCM_FX_Entry entry = {0};
    FILE* f = fastopen(path, "rb");
    if (!f) {
        ESP_LOGW(TAG, "Cannot open FX file: %s", path);
        return entry;
    }

    // Read entire file
    fseek(f, 0, SEEK_END);
    size_t fsize = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t* filedata = (uint8_t*)heap_caps_malloc(fsize, MALLOC_CAP_SPIRAM);
    if (!filedata) {
        ESP_LOGE(TAG, "OOM for FX file read: %s", path);
        fastclose(f);
        return entry;
    }
    fread(filedata, 1, fsize, f);
    fastclose(f);

    // Allocate output buffer (estimate: 4 bytes per PCM sample * 1152 * 2ch * ~100 frames)
    // We'll use a dynamic approach: decode in chunks and realloc
    size_t out_capacity = fsize * 8; // rough overestimate
    int16_t* out_pcm = (int16_t*)heap_caps_malloc(out_capacity * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    if (!out_pcm) {
        ESP_LOGE(TAG, "OOM for FX PCM output: %s", path);
        heap_caps_free(filedata);
        return entry;
    }

    mp3dec_t dec;
    mp3dec_init(&dec);
    mp3dec_frame_info_t info;
    int16_t frame_buf[PCM_BUF_FRAMES * 2];
    size_t out_pos = 0;
    size_t buf_pos = 0;

    while (buf_pos < fsize) {
        int samples = mp3dec_decode_frame(&dec, filedata + buf_pos, (int)(fsize - buf_pos), frame_buf, &info);
        if (info.frame_bytes > 0) buf_pos += info.frame_bytes;
        else { buf_pos++; continue; }
        if (samples > 0) {
            int total = samples * info.channels;
            if ((out_pos + total) > out_capacity) {
                break; // buffer full, stop decoding
            }
            memcpy(out_pcm + out_pos, frame_buf, total * sizeof(int16_t));
            out_pos += total;
            if (entry.channels == 0) {
                entry.channels = info.channels;
                entry.sample_rate = info.hz;
            }
        }
    }

    heap_caps_free(filedata);
    entry.samples = out_pcm;
    entry.num_samples = out_pos;
    ESP_LOGI(TAG, "Loaded FX: %s -> %d samples, %d ch, %d Hz", path, (int)out_pos, entry.channels, entry.sample_rate);
    return entry;
}

// ---- Music streaming task ----
static void music_task(void* arg) {
    (void)arg;
    FILE* f = NULL;
    mp3dec_t* dec = (mp3dec_t*)heap_caps_malloc(sizeof(mp3dec_t), MALLOC_CAP_INTERNAL);

    while (1) {
        if (s_new_music && s_music_path[0]) {
            s_new_music = false;
            s_stop_music = false;
            s_music_finished = false;

            if (f) { fastclose(f); f = NULL; }
            f = fastopen(s_music_path, "rb");
            if (!f) {
                ESP_LOGW(TAG, "Cannot open music: %s", s_music_path);
                s_music_playing = false;
                vTaskDelay(pdMS_TO_TICKS(100));
                continue;
            }
            mp3dec_init(dec);

            size_t buf_len = 0, buf_pos = 0;
            bool done = false;

            while (!done && !s_stop_music && s_music_playing) {
                // Fill read buffer
                if (buf_pos > 0) {
                    memmove(s_read_buf, s_read_buf + buf_pos, buf_len - buf_pos);
                    buf_len -= buf_pos;
                    buf_pos = 0;
                }
                size_t space = READ_BUF_SIZE - buf_len;
                if (space > 0 && f) {
                    size_t n = fread(s_read_buf + buf_len, 1, space, f);
                    buf_len += n;
                    if (n == 0) done = true;
                }

                if (buf_len - buf_pos < 4) {
                    done = true;
                    break;
                }

                mp3dec_frame_info_t info;
                int samples = mp3dec_decode_frame(dec, s_read_buf + buf_pos,
                                                   (int)(buf_len - buf_pos), s_pcm_buf, &info);
                if (info.frame_bytes > 0) buf_pos += info.frame_bytes;
                else { buf_pos++; continue; }

                if (samples > 0 && s_i2s) {
                    size_t bytes = samples * info.channels * sizeof(int16_t);
                    size_t written = 0;
                    i2s_channel_write(s_i2s, s_pcm_buf, bytes, &written, pdMS_TO_TICKS(500));
                }
            }

            if (f) { fastclose(f); f = NULL; }

            if (s_music_once) {
                s_music_finished = true;
                s_music_playing = false;
            } else if (!s_stop_music) {
                // Loop: restart
                s_new_music = true;
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
}

// ---- Public API ----
void PAL_AudioInit(void) {
    if (s_mp3dec) return; // already init

    s_mp3dec = (mp3dec_t*)heap_caps_malloc(sizeof(mp3dec_t), MALLOC_CAP_INTERNAL);
    s_read_buf = (uint8_t*)heap_caps_malloc(READ_BUF_SIZE, MALLOC_CAP_SPIRAM);
    s_pcm_buf = (int16_t*)heap_caps_malloc(PCM_BUF_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    s_music_mutex = xSemaphoreCreateMutex();

    if (!s_mp3dec || !s_read_buf || !s_pcm_buf) {
        ESP_LOGE(TAG, "Audio init: OOM");
        return;
    }

    // Get I2S handle from BSP
    if (bsp_audio_get_i2s_handle(&s_i2s) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get I2S handle");
        s_i2s = NULL;
    }

    // Start music task
    xTaskCreatePinnedToCore(music_task, "music", 32768, NULL, 5, &s_music_task, 1);

    ESP_LOGI(TAG, "PAL Audio initialized");
}

void PAL_AudioDeInit(void) {
    PAL_SoundStopMusic();
    if (s_music_task) {
        vTaskDelete(s_music_task);
        s_music_task = NULL;
    }
    // Free predef FX
    for (int i = 0; i < FX_MAX_PREDEF; i++) {
        if (s_predef_fx[i].samples) {
            heap_caps_free(s_predef_fx[i].samples);
            s_predef_fx[i].samples = NULL;
        }
    }
    PAL_DeInitSoundFXLua();
}

void PAL_SoundStartMusic(const char* fname, bool playOnce, bool fullpathname) {
    if (!fname || !fname[0]) return;
    s_stop_music = true;
    vTaskDelay(pdMS_TO_TICKS(50));
    s_music_once = playOnce;
    if (fullpathname) {
        snprintf(s_music_path, sizeof(s_music_path), "%s", fname);
    } else {
        snprintf(s_music_path, sizeof(s_music_path), "%s", configGetPath(fname));
    }
    s_music_playing = true;
    s_music_finished = false;
    s_new_music = true;
}

void PAL_SoundStopMusic(void) {
    s_stop_music = true;
    s_music_playing = false;
    vTaskDelay(pdMS_TO_TICKS(50));
}

bool PAL_SoundPlayOnceFinished(void) {
    return s_music_finished;
}

void PAL_SoundMusicFinished(void) {
    s_music_finished = true;
}

void PAL_SoundPlayFX(Uint32 fx) {
    if (fx >= FX_MAX_PREDEF) return;
    PCM_FX_Entry* e = &s_predef_fx[fx];
    if (!e->samples || !s_i2s) return;
    size_t bytes = e->num_samples * sizeof(int16_t);
    size_t written = 0;
    i2s_channel_write(s_i2s, e->samples, bytes, &written, pdMS_TO_TICKS(200));
}

Uint32 PAL_SoundAddFX(const char* fname) {
    if (s_lua_fx_count >= MAX_FX_SAMPLES) return 0;
    s_lua_fx[s_lua_fx_count] = decode_mp3_file(fname);
    return s_lua_fx_count++;
}

void PAL_SoundPlayFXLua(Uint32 fx) {
    if (fx >= s_lua_fx_count) return;
    PCM_FX_Entry* e = &s_lua_fx[fx];
    if (!e->samples || !s_i2s) return;
    size_t bytes = e->num_samples * sizeof(int16_t);
    size_t written = 0;
    i2s_channel_write(s_i2s, e->samples, bytes, &written, pdMS_TO_TICKS(200));
}

void PAL_InitSoundFXLua(void) {
    s_lua_fx_count = 0;
    memset(s_lua_fx, 0, sizeof(s_lua_fx));
}

void PAL_DeInitSoundFXLua(void) {
    for (Uint32 i = 0; i < s_lua_fx_count; i++) {
        if (s_lua_fx[i].samples) {
            heap_caps_free(s_lua_fx[i].samples);
            s_lua_fx[i].samples = NULL;
        }
    }
    s_lua_fx_count = 0;
}
