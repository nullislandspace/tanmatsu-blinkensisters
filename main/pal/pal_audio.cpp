// =====================================================================
//  BlinkenSisters -- pal_audio.cpp
//
//  One mixer task owns the I2S channel. It decodes the music stream and
//  mixes it with a small pool of one-shot sound-effect voices into a
//  single block, then does a single write.
//
//  This replaces an arrangement where the music task and the GAME TASK
//  both called i2s_channel_write() on the same channel. Two writers on one
//  channel interleave at arbitrary block boundaries, so music and effects
//  chopped each other up; and because effects were written synchronously,
//  every sound stalled the game task for as long as the effect lasted (up
//  to the 200 ms write timeout, after which the sound was simply dropped).
//  Starting and stopping music likewise slept the caller for 50 ms.
//
//  Now the game task never touches I2S: playing an effect claims a voice
//  and returns, and music start/stop just sets a flag.
// =====================================================================

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

// ---- Output format ----
// The I2S channel is configured for 44100 Hz stereo 16-bit in main.cpp.
#define OUT_RATE        44100
#define OUT_CHANNELS    2
// Frames per mix block. 512 frames is ~11.6 ms: short enough that an effect
// starts promptly, long enough that the per-write overhead is negligible.
#define MIX_BLOCK_FRAMES 512
#define MAX_VOICES       8

// ---- Buffer sizes ----
#define READ_BUF_SIZE   (16 * 1024)   // MP3 bitstream read-ahead, PSRAM
#define PCM_BUF_FRAMES  1152          // max frames in one MP3 frame

// ---- PCM FX storage ----
typedef struct {
    int16_t* samples;
    size_t   num_samples;   // total int16 values, i.e. frames * channels
    int      channels;
    int      sample_rate;
} PCM_FX_Entry;

static PCM_FX_Entry s_predef_fx[FX_MAX_PREDEF] = {};
static PCM_FX_Entry s_lua_fx[MAX_FX_SAMPLES]   = {};
static Uint32       s_lua_fx_count = 0;

// ---- Voices ----
// A voice is one playing instance of a PCM_FX_Entry. Claimed by the game
// task, advanced and released by the mixer, so both touch it under s_mix_mux.
typedef struct {
    const int16_t* samples;
    size_t         num_samples;
    int            channels;
    // 16.16 fixed point, and 64-bit on purpose: in 32 bits the cursor wraps
    // after 65536 frames -- 1.49 s at 44100 Hz -- and the voice restarts
    // instead of ending, which left the death sound looping forever.
    uint64_t       pos_q16;     // read cursor in frames, 16.16 fixed point
    uint32_t       step_q16;    // frames advanced per output frame
    bool           active;
} Voice;

static Voice             s_voices[MAX_VOICES];
static SemaphoreHandle_t s_mix_mux = NULL;

// ---- Music streaming state ----
static volatile bool s_music_playing  = false;
static volatile bool s_music_once     = false;
static volatile bool s_music_finished = false;
static char          s_music_path[512] = {};
static volatile bool s_new_music       = false;
static volatile bool s_stop_music      = false;

static uint8_t*   s_read_buf   = NULL;   // MP3 bitstream, PSRAM
static int16_t*   s_pcm_buf    = NULL;   // one decoded MP3 frame
static int32_t*   s_accum      = NULL;   // mix accumulator, internal RAM
static int16_t*   s_out_buf    = NULL;   // I2S block, DMA-capable
static i2s_chan_handle_t s_i2s = NULL;

static TaskHandle_t s_mixer_task = NULL;
static volatile bool s_mixer_run = false;

// ---- Decode a whole MP3 file into PCM in PSRAM ----
static PCM_FX_Entry decode_mp3_file(const char* path) {
    PCM_FX_Entry entry = {};
    FILE* f = fastopen(path, "rb");
    if (!f) {
        ESP_LOGW(TAG, "Cannot open FX file: %s", path);
        return entry;
    }

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

    // Rough overestimate of the decoded size; decoding stops if it is hit.
    size_t out_capacity = fsize * 8;
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

// ---- Voice handling -----------------------------------------------------

static void voice_start(const PCM_FX_Entry* e) {
    if (!e || !e->samples || e->num_samples == 0 || e->channels <= 0) {
        return;
    }
    int rate = e->sample_rate > 0 ? e->sample_rate : OUT_RATE;

    xSemaphoreTake(s_mix_mux, portMAX_DELAY);
    Voice* v = NULL;
    for (int i = 0; i < MAX_VOICES; i++) {
        if (!s_voices[i].active) { v = &s_voices[i]; break; }
    }
    // All voices busy: steal the one furthest through its sample, which is
    // the one closest to finishing anyway.
    if (!v) {
        uint64_t best = 0;
        for (int i = 0; i < MAX_VOICES; i++) {
            if (s_voices[i].pos_q16 >= best) { best = s_voices[i].pos_q16; v = &s_voices[i]; }
        }
    }
    v->samples     = e->samples;
    v->num_samples = e->num_samples;
    v->channels    = e->channels;
    v->pos_q16     = 0;
    // Effects are not always recorded at the output rate; step through the
    // source at the ratio between them so they play at the right pitch.
    v->step_q16    = (uint32_t)(((uint64_t)rate << 16) / OUT_RATE);
    v->active      = true;
    xSemaphoreGive(s_mix_mux);
}

static void voices_stop_all(void) {
    if (!s_mix_mux) return;
    xSemaphoreTake(s_mix_mux, portMAX_DELAY);
    for (int i = 0; i < MAX_VOICES; i++) {
        s_voices[i].active  = false;
        s_voices[i].samples = NULL;
    }
    xSemaphoreGive(s_mix_mux);
}

// Add every active voice into the accumulator for `frames` output frames.
static void mix_voices(int32_t* accum, int frames) {
    xSemaphoreTake(s_mix_mux, portMAX_DELAY);
    for (int i = 0; i < MAX_VOICES; i++) {
        Voice* v = &s_voices[i];
        if (!v->active || !v->samples) continue;

        size_t total_frames = v->num_samples / (size_t)v->channels;
        for (int f = 0; f < frames; f++) {
            size_t src_frame = (size_t)(v->pos_q16 >> 16);
            if (src_frame >= total_frames) { v->active = false; break; }
            const int16_t* sp = v->samples + src_frame * (size_t)v->channels;
            int32_t l = sp[0];
            int32_t r = (v->channels > 1) ? sp[1] : l;   // mono plays centred
            accum[f * 2 + 0] += l;
            accum[f * 2 + 1] += r;
            v->pos_q16 += v->step_q16;
        }
    }
    xSemaphoreGive(s_mix_mux);
}

// ---- Music decoding -----------------------------------------------------
// Decoded music is kept in a small holding buffer between the MP3 frame
// decoder (which emits 1152-frame chunks) and the mixer (which consumes
// MIX_BLOCK_FRAMES at a time).

typedef struct {
    FILE*    f;
    mp3dec_t dec;
    size_t   buf_len;     // bytes of bitstream held
    size_t   buf_pos;     // bytes consumed
    int      pcm_frames;  // decoded frames waiting in s_pcm_buf
    int      pcm_pos;     // frames of those already mixed
    int      channels;
    uint32_t step_q16;    // source frames per output frame
    uint32_t frac_q16;    // resampling cursor within the held chunk
    bool     eof;
} MusicStream;

static void music_close(MusicStream* m) {
    if (m->f) { fastclose(m->f); m->f = NULL; }
    m->buf_len = m->buf_pos = 0;
    m->pcm_frames = m->pcm_pos = 0;
    m->frac_q16 = 0;
    m->eof = false;
}

// Decode the next MP3 frame into s_pcm_buf. Returns false at end of stream.
static bool music_decode_frame(MusicStream* m) {
    for (;;) {
        if (m->buf_pos > 0) {
            memmove(s_read_buf, s_read_buf + m->buf_pos, m->buf_len - m->buf_pos);
            m->buf_len -= m->buf_pos;
            m->buf_pos = 0;
        }
        if (!m->eof && m->buf_len < READ_BUF_SIZE) {
            size_t n = fread(s_read_buf + m->buf_len, 1, READ_BUF_SIZE - m->buf_len, m->f);
            m->buf_len += n;
            if (n == 0) m->eof = true;
        }
        if (m->buf_len - m->buf_pos < 4) {
            return false;
        }

        mp3dec_frame_info_t info;
        int samples = mp3dec_decode_frame(&m->dec, s_read_buf + m->buf_pos,
                                          (int)(m->buf_len - m->buf_pos), s_pcm_buf, &info);
        if (info.frame_bytes > 0) {
            m->buf_pos += info.frame_bytes;
        } else {
            m->buf_pos++;
            continue;
        }
        if (samples > 0) {
            m->pcm_frames = samples;
            m->pcm_pos    = 0;
            m->channels   = info.channels;
            int rate      = info.hz > 0 ? info.hz : OUT_RATE;
            m->step_q16   = (uint32_t)(((uint64_t)rate << 16) / OUT_RATE);
            return true;
        }
    }
}

// Add up to `frames` output frames of music into the accumulator.
static void mix_music(MusicStream* m, int32_t* accum, int frames) {
    for (int f = 0; f < frames; f++) {
        if (m->pcm_pos >= m->pcm_frames) {
            if (!music_decode_frame(m)) {
                return;   // stream exhausted; the caller notices via eof
            }
        }
        const int16_t* sp = s_pcm_buf + (size_t)m->pcm_pos * (size_t)m->channels;
        int32_t l = sp[0];
        int32_t r = (m->channels > 1) ? sp[1] : l;
        accum[f * 2 + 0] += l;
        accum[f * 2 + 1] += r;

        m->frac_q16 += m->step_q16;
        m->pcm_pos  += (int)(m->frac_q16 >> 16);
        m->frac_q16 &= 0xFFFF;
    }
}

static inline int16_t clamp16(int32_t v) {
    if (v >  32767) return  32767;
    if (v < -32768) return -32768;
    return (int16_t)v;
}

// ---- The mixer task -----------------------------------------------------

static void mixer_task(void* arg) {
    (void)arg;
    MusicStream music = {};

    while (s_mixer_run) {
        // Music transport, handled here so callers never block.
        if (s_stop_music) {
            music_close(&music);
            s_stop_music = false;
        }
        if (s_new_music) {
            s_new_music = false;
            music_close(&music);
            music.f = fastopen(s_music_path, "rb");
            if (!music.f) {
                ESP_LOGW(TAG, "Cannot open music: %s", s_music_path);
                s_music_playing = false;
            } else {
                mp3dec_init(&music.dec);
                music.channels = 2;
                music.step_q16 = 1 << 16;
                s_music_finished = false;
            }
        }

        memset(s_accum, 0, sizeof(int32_t) * MIX_BLOCK_FRAMES * OUT_CHANNELS);

        if (music.f && s_music_playing) {
            mix_music(&music, s_accum, MIX_BLOCK_FRAMES);
            // End of file: either loop straight back or report it finished.
            if (music.eof && music.pcm_pos >= music.pcm_frames &&
                music.buf_len - music.buf_pos < 4) {
                music_close(&music);
                if (s_music_once) {
                    s_music_finished = true;
                    s_music_playing  = false;
                } else {
                    s_new_music = true;
                }
            }
        }

        mix_voices(s_accum, MIX_BLOCK_FRAMES);

        for (int i = 0; i < MIX_BLOCK_FRAMES * OUT_CHANNELS; i++) {
            s_out_buf[i] = clamp16(s_accum[i]);
        }

        if (s_i2s) {
            size_t written = 0;
            // This write is what paces the loop: it blocks until the DMA
            // ring has room, so the mixer runs exactly as fast as playback.
            i2s_channel_write(s_i2s, s_out_buf,
                              MIX_BLOCK_FRAMES * OUT_CHANNELS * sizeof(int16_t),
                              &written, pdMS_TO_TICKS(500));
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    music_close(&music);
    s_mixer_task = NULL;
    vTaskDelete(NULL);
}

// ---- Public API ---------------------------------------------------------

void PAL_AudioInit(void) {
    if (s_mixer_task) return; // already init

    s_read_buf = (uint8_t*)heap_caps_malloc(READ_BUF_SIZE, MALLOC_CAP_SPIRAM);
    s_pcm_buf  = (int16_t*)heap_caps_malloc(PCM_BUF_FRAMES * 2 * sizeof(int16_t), MALLOC_CAP_INTERNAL);
    s_accum    = (int32_t*)heap_caps_malloc(MIX_BLOCK_FRAMES * OUT_CHANNELS * sizeof(int32_t), MALLOC_CAP_INTERNAL);
    s_out_buf  = (int16_t*)heap_caps_malloc(MIX_BLOCK_FRAMES * OUT_CHANNELS * sizeof(int16_t),
                                            MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    s_mix_mux  = xSemaphoreCreateMutex();

    if (!s_read_buf || !s_pcm_buf || !s_accum || !s_out_buf || !s_mix_mux) {
        ESP_LOGE(TAG, "Audio init: OOM");
        return;
    }
    memset(s_voices, 0, sizeof(s_voices));

    if (bsp_audio_get_i2s_handle(&s_i2s) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get I2S handle");
        s_i2s = NULL;
    }

    s_mixer_run = true;
    xTaskCreatePinnedToCore(mixer_task, "audio_mix", 32768, NULL, 7, &s_mixer_task, 1);

    ESP_LOGI(TAG, "PAL Audio initialized (mixer: %d voices, %d-frame blocks)",
             MAX_VOICES, MIX_BLOCK_FRAMES);
}

void PAL_AudioDeInit(void) {
    PAL_SoundStopMusic();
    voices_stop_all();

    // Let the mixer notice and leave its loop before the PCM it may still be
    // reading is freed. One block is ~12 ms; give it a generous margin.
    if (s_mixer_task) {
        s_mixer_run = false;
        for (int i = 0; i < 50 && s_mixer_task; i++) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

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
    s_music_once = playOnce;
    if (fullpathname) {
        snprintf(s_music_path, sizeof(s_music_path), "%s", fname);
    } else {
        snprintf(s_music_path, sizeof(s_music_path), "%s", configGetPath(fname));
    }
    s_music_finished = false;
    s_music_playing  = true;
    s_new_music      = true;
}

void PAL_SoundStopMusic(void) {
    s_music_playing = false;
    s_new_music     = false;
    s_stop_music    = true;
}

bool PAL_SoundPlayOnceFinished(void) {
    return s_music_finished;
}

void PAL_SoundMusicFinished(void) {
    s_music_finished = true;
}

// Load the built-in effects. These live at the root of the extracted game
// data, so this cannot run until configInit() has unpacked basedata.bmf --
// hence a separate call rather than doing it in PAL_AudioInit().
//
// Nothing ever populated s_predef_fx before, so every soundPlayFX() call in
// the game was silently a no-op: menu clicks, pixel pickups, kills and level
// completions have all been mute on this port.
void PAL_SoundLoadPredefFX(void) {
    // Order must match FX_SOUNDS in game/sound.h. basedata.bmf ships no menu
    // effect; if none turns up, that slot stays empty and simply stays quiet.
    static const char* const files[FX_MAX_PREDEF] = {
        "fx_collect_pixel.mp3",   // FX_COLLECT_PIXEL
        "fx_kill_monster.mp3",    // FX_KILL_MONSTER
        "fx_killed.mp3",          // FX_KILL_PLAYER
        "fx_level_finished.mp3",  // FX_LEVEL_FINISHED
        "fx_menu.mp3",            // FX_MENU
        "fx_respawnpoint.mp3",    // FX_RESPAWNPOINT
    };
    for (int i = 0; i < FX_MAX_PREDEF; i++) {
        if (s_predef_fx[i].samples) continue;   // already loaded
        s_predef_fx[i] = decode_mp3_file(configGetPath(files[i]));
    }
}

void PAL_SoundPlayFX(Uint32 fx) {
    if (fx >= FX_MAX_PREDEF || !s_mix_mux) return;
    voice_start(&s_predef_fx[fx]);
}

Uint32 PAL_SoundAddFX(const char* fname) {
    if (s_lua_fx_count >= MAX_FX_SAMPLES) return 0;
    s_lua_fx[s_lua_fx_count] = decode_mp3_file(fname);
    return s_lua_fx_count++;
}

void PAL_SoundPlayFXLua(Uint32 fx) {
    if (fx >= s_lua_fx_count || !s_mix_mux) return;
    voice_start(&s_lua_fx[fx]);
}

void PAL_InitSoundFXLua(void) {
    voices_stop_all();
    s_lua_fx_count = 0;
    memset(s_lua_fx, 0, sizeof(s_lua_fx));
}

void PAL_DeInitSoundFXLua(void) {
    // Silence first: a voice may still be reading one of these buffers.
    voices_stop_all();
    for (Uint32 i = 0; i < s_lua_fx_count; i++) {
        if (s_lua_fx[i].samples) {
            heap_caps_free(s_lua_fx[i].samples);
            s_lua_fx[i].samples = NULL;
        }
    }
    s_lua_fx_count = 0;
}
