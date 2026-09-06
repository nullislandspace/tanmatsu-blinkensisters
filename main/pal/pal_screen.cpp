#include "pal_screen.h"
#include "pal_ppa.h"
#include "shared/profile.h"
extern "C" {
#include "bsp/display.h"
#include "esp_cache.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
}
#include <string.h>

static const char* TAG = "pal_screen";

// PSRAM L2 cache line. The PPA writes phys_fb by DMA, so the buffer and its
// size must both be aligned to it.
#define PHYS_FB_ALIGN 128

// TEMPORARY MEASUREMENT SWITCH.
//
// Use the PPA rotation even when calibrate_flip() cannot reproduce the CPU
// reference, so its cost can be read off the profiler. Colours (and possibly
// the orientation) may be wrong while this is on -- that is the point: it
// separates "how fast is the PPA at this" from "what exactly does it produce".
// What appears on the panel is itself diagnostic: a correct-looking picture in
// the wrong colours means only the channel order differs, while a scrambled
// one means the rotation does.
//
// Set back to 0 once the answer is in.
#define FORCE_PPA_ROTATE 1

// The panel runs in RGB565, so the physical framebuffer is 2 bytes per pixel
// like everything else. See main.cpp, which requests that format.
#define PHYS_FB_SIZE_RAW ((size_t)PHYS_W * PHYS_H * 2)
#define PHYS_FB_SIZE     ((PHYS_FB_SIZE_RAW + PHYS_FB_ALIGN - 1) & ~(size_t)(PHYS_FB_ALIGN - 1))

BS_Surface* gScreen = NULL;
static uint8_t* phys_fb = NULL;
static bool     s_use_ppa = false;
static bool     s_flip_rgb_swap = false;
static bool     s_flip_byte_swap = false;

// Job id for the one PPA op a flip submits. Ids only need to be unique among
// the jobs in flight, and the flip drains its own, so a constant is fine.
#define JOB_FLIP    1u
#define JOB_CALIBRATE 2u

// Rotate one logical pixel buffer into a physical one, the way the CPU path
// has always done it: physical (col, row) = logical (row, log_h - 1 - col),
// written as byte order B,G,R -- which is what this panel's RGB888 wants (the
// same convention tanmatsu-tadoom documents for its PAX framebuffer).
static void rotate_reference(const BS_Pixel* px, int log_w, int log_h,
                             BS_Pixel* out, int phys_w) {
    for (int ly = 0; ly < log_h; ly++) {
        for (int lx = 0; lx < log_w; lx++) {
            out[(size_t)lx * phys_w + (size_t)(log_h - 1 - ly)] =
                px[(size_t)ly * log_w + lx];
        }
    }
}

// Work out what the PPA's input reordering knobs actually do, by doing them.
//
// The driver describes rgb_swap two incompatible ways ("ARGB becomes BGRA"
// reads as a byte reversal, "RGB becomes BGR" as a channel swap) and offers a
// separate byte_swap on top, and getting the combination wrong changes every
// colour on a panel this code cannot see. So try the combinations on the real
// framebuffers and keep whichever reproduces the CPU rotation byte for byte.
//
// This runs at the production geometry, using gScreen and phys_fb themselves.
// An earlier version used a small scratch pair instead and matched nothing,
// which told us only that the PPA behaves differently at that size -- not
// anything useful about the screen.
static bool calibrate_flip(bool* out_rgb_swap, bool* out_byte_swap) {
    const size_t raw = PHYS_FB_SIZE_RAW;
    BS_Pixel* want = (BS_Pixel*)heap_caps_malloc(raw, MALLOC_CAP_SPIRAM);
    if (!want) {
        ESP_LOGW(TAG, "flip calib: no memory for the reference image");
        return false;
    }

    // A pattern with all three channels independent, so no reordering of them
    // can accidentally match.
    for (int y = 0; y < LOG_H; y++) {
        for (int x = 0; x < LOG_W; x++) {
            uint8_t r = (uint8_t)(x & 0xff);
            uint8_t g = (uint8_t)((y * 3) & 0xff);
            uint8_t b = (uint8_t)((255 - (x & 0xff)) ^ (uint8_t)y);
            gScreen->pixels[(size_t)y * LOG_W + x] = BS_PackOpaque(BS_MapRGBA(r, g, b, 0xff));
        }
    }
    rotate_reference(gScreen->pixels, LOG_W, LOG_H, want, PHYS_W);

    bool ok = false;
    for (int combo = 0; combo < 4 && !ok; combo++) {
        bool rgb  = (combo & 1) != 0;
        bool byte = (combo & 2) != 0;

        memset(phys_fb, 0, PHYS_FB_SIZE);
        esp_cache_msync(phys_fb, PHYS_FB_SIZE,
                        ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_TYPE_DATA);
        PAL_PPA_FlushSurface(gScreen);
        if (!PAL_PPA_FlipToPanel(gScreen, JOB_CALIBRATE, phys_fb, PHYS_FB_SIZE,
                                 PHYS_W, PHYS_H, rgb, byte)) {
            ESP_LOGW(TAG, "flip calib: rgb_swap=%d byte_swap=%d refused", (int)rgb, (int)byte);
            continue;
        }
        PAL_PPA_WaitJob(JOB_CALIBRATE);
        // The PPA wrote this by DMA; drop our stale lines before reading it.
        esp_cache_msync(phys_fb, PHYS_FB_SIZE,
                        ESP_CACHE_MSYNC_FLAG_DIR_M2C | ESP_CACHE_MSYNC_FLAG_TYPE_DATA);

        if (memcmp(phys_fb, want, raw) == 0) {
            *out_rgb_swap  = rgb;
            *out_byte_swap = byte;
            ok = true;
            break;
        }

        // Say exactly how it differed, so a mismatch is diagnosable from the
        // log rather than by guessing at the documentation again.
        const BS_Pixel* got = (const BS_Pixel*)phys_fb;
        const size_t npix = raw / sizeof(BS_Pixel);
        size_t bad = 0;
        while (bad < npix && got[bad] == want[bad]) bad++;
        ESP_LOGW(TAG, "flip calib: rgb=%d byte=%d differs at pixel %u/%u "
                      "(want %04x, got %04x)",
                 (int)rgb, (int)byte, (unsigned)bad, (unsigned)npix,
                 bad < npix ? want[bad] : 0, bad < npix ? got[bad] : 0);
        // Where did the source's top-left pixel actually land? Anywhere but
        // the expected slot means the rotation differs, not the colour order.
        BS_Pixel p0 = gScreen->pixels[0];
        for (size_t i = 0; i < npix; i++) {
            if (got[i] == p0) {
                ESP_LOGW(TAG, "  source (0,0) %04x landed at pixel %u "
                              "(row %u col %u); the CPU path puts it at %u",
                         p0, (unsigned)i,
                         (unsigned)(i / (size_t)PHYS_W),
                         (unsigned)(i % (size_t)PHYS_W),
                         (unsigned)(PHYS_W - 1));
                break;
            }
        }
    }

    heap_caps_free(want);
    // Leave the screen black whatever happened; the test pattern is not
    // something anyone wants to see flash up.
    memset(gScreen->pixels, 0, (size_t)LOG_W * LOG_H * sizeof(BS_Pixel));
    memset(phys_fb, 0, PHYS_FB_SIZE);
    esp_cache_msync(phys_fb, PHYS_FB_SIZE,
                    ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_TYPE_DATA);
    return ok;
}

void BS_InitScreen(void) {
    if (!gScreen) {
        gScreen = BS_CreateSurface(LOG_W, LOG_H);
        if (!gScreen) {
            ESP_LOGE(TAG, "Failed to create logical screen surface");
            return;
        }
    }
    if (!phys_fb) {
        phys_fb = (uint8_t*)heap_caps_aligned_alloc(PHYS_FB_ALIGN, PHYS_FB_SIZE, MALLOC_CAP_SPIRAM);
        if (!phys_fb) {
            ESP_LOGE(TAG, "Failed to allocate physical framebuffer");
            return;
        }
        memset(phys_fb, 0, PHYS_FB_SIZE);
        // That memset left dirty lines in cache. Push them out now, so they
        // cannot be written back on top of pixels the PPA puts there later.
        esp_cache_msync(phys_fb, PHYS_FB_SIZE,
                        ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_TYPE_DATA);
    }

    // Hand the logical->panel rotation to the PPA if we can. It is by far the
    // most expensive thing in a frame on the CPU (see BS_FlipCPU below), and
    // it is a single hardware op. A failure here is not fatal: s_use_ppa stays
    // false and the CPU path runs instead.
    s_use_ppa = PAL_PPA_Init();
    if (s_use_ppa) {
        if (calibrate_flip(&s_flip_rgb_swap, &s_flip_byte_swap)) {
            ESP_LOGI(TAG, "PPA flip calibrated: rgb_swap=%s byte_swap=%s",
                     s_flip_rgb_swap ? "true" : "false",
                     s_flip_byte_swap ? "true" : "false");
        } else {
#if FORCE_PPA_ROTATE
            ESP_LOGW(TAG, "PPA flip matches no combination, but FORCE_PPA_ROTATE "
                          "is on: using it anyway (rgb_swap=%s byte_swap=%s). "
                          "Colours and/or orientation may be wrong -- this is a "
                          "measurement, not a fix.",
                     s_flip_rgb_swap ? "true" : "false",
                     s_flip_byte_swap ? "true" : "false");
#else
            ESP_LOGW(TAG, "PPA flip does not match the CPU reference either way; "
                          "falling back to the CPU rotation");
            s_use_ppa = false;
#endif
        }
    }

    profSetRotationPath(s_use_ppa ? "PPA" : "CPU");
    bsp_display_set_backlight_brightness(100);
    ESP_LOGI(TAG, "Screen initialized: logical %dx%d -> physical %dx%d (%s rotation)",
             LOG_W, LOG_H, PHYS_W, PHYS_H, s_use_ppa ? "PPA" : "CPU");
}

BS_Surface* BS_SetVideoMode(Uint32 width, Uint32 height, Uint32 depth, Uint32 flags) {
    (void)depth; (void)flags;
    // Ignore width/height - always use fixed 800x480 logical space
    (void)width; (void)height;
    BS_InitScreen();
    return gScreen;
}

// CPU fallback: rotate logical (800x480 RGBA32) -> physical (480x800 BGR888).
//
// Physical pixel (px, py) is logical (lx = py, ly = 479 - px), so
//   phys_fb[lx * PHYS_W + (LOG_H - 1 - ly)] = pixel(lx, ly).
//
// A rotation cannot walk both buffers contiguously, so this works in tiles:
// one TILE x TILE block at a time keeps the strided side inside the cache
// instead of taking a miss on every one of the 384000 pixels, which is what
// the original row-major version did.
#define FLIP_TILE 32

static void BS_FlipCPU(const BS_Surface* screen) {
    // A rotation cannot walk both buffers contiguously, so work in tiles: one
    // TILE x TILE block at a time keeps the strided side inside the cache
    // instead of taking a miss on every one of the 384000 pixels, which is
    // what the original row-major version did.
    const BS_Pixel* px = screen->pixels;
    BS_Pixel* out = (BS_Pixel*)phys_fb;
    for (int ly0 = 0; ly0 < LOG_H; ly0 += FLIP_TILE) {
        int ly_end = ly0 + FLIP_TILE; if (ly_end > LOG_H) ly_end = LOG_H;
        for (int lx0 = 0; lx0 < LOG_W; lx0 += FLIP_TILE) {
            int lx_end = lx0 + FLIP_TILE; if (lx_end > LOG_W) lx_end = LOG_W;
            for (int ly = ly0; ly < ly_end; ly++) {
                const BS_Pixel* srow = px + (size_t)ly * LOG_W;
                BS_Pixel* dcol = out + (size_t)(LOG_H - 1 - ly);
                for (int lx = lx0; lx < lx_end; lx++) {
                    dcol[(size_t)lx * PHYS_W] = srow[lx];
                }
            }
        }
    }
}

int BS_Flip(BS_Surface* screen) {
    if (!screen) {
        ESP_LOGE(TAG, "BS_Flip: screen is NULL");
        return -1;
    }
    if (!phys_fb) {
        ESP_LOGE(TAG, "BS_Flip: phys_fb is NULL");
        return -1;
    }

    profZoneBegin(PROF_ROTATE);
    bool rotated = false;
    if (s_use_ppa) {
        // The CPU has just drawn the frame, so push it out of cache before the
        // PPA's DMA goes looking for it.
        PAL_PPA_FlushSurface(screen);
        if (PAL_PPA_FlipToPanel(screen, JOB_FLIP, phys_fb, PHYS_FB_SIZE,
                                PHYS_W, PHYS_H, s_flip_rgb_swap, s_flip_byte_swap)) {
            PAL_PPA_WaitJob(JOB_FLIP);
            rotated = true;
        }
    }
    if (!rotated) {
        BS_FlipCPU(screen);
    }
    profZoneEnd(PROF_ROTATE);

    profZoneBegin(PROF_PANEL);
    esp_err_t ret = bsp_display_blit(0, 0, PHYS_W, PHYS_H, phys_fb);
    profZoneEnd(PROF_PANEL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "bsp_display_blit failed: %d", ret);
        return -1;
    }
    return 0;
}

void BS_InitDisplay(void) {
    BS_InitScreen();
}
