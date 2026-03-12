#include "pal_screen.h"
extern "C" {
#include "bsp/display.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
}
#include <string.h>

static const char* TAG = "pal_screen";

BS_Surface* gScreen = NULL;
static uint8_t* phys_fb = NULL;

void BS_InitScreen(void) {
    if (!gScreen) {
        gScreen = BS_CreateSurface(LOG_W, LOG_H);
        if (!gScreen) {
            ESP_LOGE(TAG, "Failed to create logical screen surface");
            return;
        }
    }
    if (!phys_fb) {
        phys_fb = (uint8_t*)heap_caps_malloc(PHYS_W * PHYS_H * 3, MALLOC_CAP_SPIRAM);
        if (!phys_fb) {
            ESP_LOGE(TAG, "Failed to allocate physical framebuffer");
            return;
        }
        memset(phys_fb, 0, PHYS_W * PHYS_H * 3);
    }
    bsp_display_set_backlight_brightness(100);
    ESP_LOGI(TAG, "Screen initialized: logical %dx%d -> physical %dx%d", LOG_W, LOG_H, PHYS_W, PHYS_H);
}

BS_Surface* BS_SetVideoMode(Uint32 width, Uint32 height, Uint32 depth, Uint32 flags) {
    (void)depth; (void)flags;
    // Ignore width/height - always use fixed 800x480 logical space
    (void)width; (void)height;
    BS_InitScreen();
    return gScreen;
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

    // Rotate logical (800x480 RGBA32) -> physical (480x800 BGR888)
    // Physical pixel at (px, py) corresponds to logical pixel at (lx=py, ly=479-px)
    // physical_index = lx * 1440 + (479 - ly) * 3
    // where lx = physical row (py), ly = physical col (px)
    // So: for lx in [0,800), for ly in [0,480):
    //   phys_fb[lx*1440 + (479-ly)*3] = BGR from pixels[ly*800+lx]
    const Uint32* px = screen->pixels;
    for (int ly = 0; ly < LOG_H; ly++) {
        for (int lx = 0; lx < LOG_W; lx++) {
            Uint32 p = px[ly * LOG_W + lx];
            uint8_t r = (p >>  0) & 0xff;
            uint8_t g = (p >>  8) & 0xff;
            uint8_t b = (p >> 16) & 0xff;
            int pidx = lx * PHYS_STRIDE + (LOG_H - 1 - ly) * 3;
            phys_fb[pidx + 0] = b;
            phys_fb[pidx + 1] = g;
            phys_fb[pidx + 2] = r;
        }
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
