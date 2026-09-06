#include "pal_screen.h"
#include "pal_ppa.h"
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

#define PHYS_FB_SIZE_RAW ((size_t)PHYS_W * PHYS_H * 3)
#define PHYS_FB_SIZE     ((PHYS_FB_SIZE_RAW + PHYS_FB_ALIGN - 1) & ~(size_t)(PHYS_FB_ALIGN - 1))

BS_Surface* gScreen = NULL;
static uint8_t* phys_fb = NULL;
static bool     s_use_ppa = false;

// Job id for the one PPA op a flip submits. Ids only need to be unique among
// the jobs in flight, and the flip drains its own, so a constant is fine.
#define JOB_FLIP 1u

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
//   phys_fb[lx * PHYS_STRIDE + (LOG_H - 1 - ly) * 3] = pixel(lx, ly).
//
// A rotation cannot walk both buffers contiguously, so this works in tiles:
// one TILE x TILE block at a time keeps the strided side inside the cache
// instead of taking a miss on every one of the 384000 pixels, which is what
// the original row-major version did.
#define FLIP_TILE 32

static void BS_FlipCPU(const BS_Surface* screen) {
    const Uint32* px = screen->pixels;
    for (int ly0 = 0; ly0 < LOG_H; ly0 += FLIP_TILE) {
        int ly_end = ly0 + FLIP_TILE; if (ly_end > LOG_H) ly_end = LOG_H;
        for (int lx0 = 0; lx0 < LOG_W; lx0 += FLIP_TILE) {
            int lx_end = lx0 + FLIP_TILE; if (lx_end > LOG_W) lx_end = LOG_W;
            for (int ly = ly0; ly < ly_end; ly++) {
                const Uint32* srow = px + (size_t)ly * LOG_W;
                uint8_t* dcol = phys_fb + (size_t)(LOG_H - 1 - ly) * 3;
                for (int lx = lx0; lx < lx_end; lx++) {
                    Uint32 p = srow[lx];
                    uint8_t* d = dcol + (size_t)lx * PHYS_STRIDE;
                    d[0] = (uint8_t)((p >> 16) & 0xff);  // B
                    d[1] = (uint8_t)((p >>  8) & 0xff);  // G
                    d[2] = (uint8_t)((p      ) & 0xff);  // R
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

    bool rotated = false;
    if (s_use_ppa) {
        // The CPU has just drawn the frame, so push it out of cache before the
        // PPA's DMA goes looking for it.
        PAL_PPA_FlushSurface(screen);
        if (PAL_PPA_FlipToPanel(screen, JOB_FLIP, phys_fb, PHYS_FB_SIZE, PHYS_W, PHYS_H)) {
            PAL_PPA_WaitJob(JOB_FLIP);
            rotated = true;
        }
    }
    if (!rotated) {
        BS_FlipCPU(screen);
    }

    esp_err_t ret = bsp_display_blit(0, 0, PHYS_W, PHYS_H, phys_fb);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "bsp_display_blit failed: %d", ret);
        return -1;
    }
    return 0;
}

void BS_InitDisplay(void) {
    BS_InitScreen();
}
