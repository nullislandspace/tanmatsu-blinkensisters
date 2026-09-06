#include "pal_surface.h"
#include "pal_ppa.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "pal_surface";

// PSRAM L2 cache-line size; see BS_CreateSurface.
#define BS_SURFACE_ALIGN 128

BS_Surface* BS_CreateSurface(Sint32 w, Sint32 h) {
    BS_Surface* s = (BS_Surface*)heap_caps_malloc(sizeof(BS_Surface), MALLOC_CAP_INTERNAL);
    if (!s) {
        ESP_LOGE(TAG, "Failed to alloc BS_Surface struct");
        return NULL;
    }
    // Round the allocation up to a whole number of PSRAM cache lines and
    // align it to one. The PPA reads and writes surfaces by DMA, and both the
    // driver's buffer requirements and esp_cache_msync() need that alignment;
    // without it a surface silently cannot be handed to the hardware.
    size_t pixels_size = (size_t)w * h * sizeof(Uint32);
    size_t alloc_size  = (pixels_size + BS_SURFACE_ALIGN - 1) & ~(size_t)(BS_SURFACE_ALIGN - 1);
    s->pixels = (Uint32*)heap_caps_aligned_alloc(BS_SURFACE_ALIGN, alloc_size, MALLOC_CAP_SPIRAM);
    if (!s->pixels) {
        // Fallback to internal RAM
        s->pixels = (Uint32*)heap_caps_aligned_alloc(BS_SURFACE_ALIGN, alloc_size, MALLOC_CAP_DEFAULT);
        if (!s->pixels) {
            ESP_LOGE(TAG, "Failed to alloc %d bytes for %dx%d surface", (int)alloc_size, w, h);
            heap_caps_free(s);
            return NULL;
        }
    }
    s->w = w;
    s->h = h;
    s->pitch = w * 4;
    s->colorkey = 0;
    s->colorkey_enabled = false;
    s->format_flags = 0;
    s->ppa_src_ready = false;
    s->_format_data.BytesPerPixel = 4;
    s->format = &s->_format_data;
    memset(s->pixels, 0, pixels_size);
    return s;
}

void BS_FreeSurface(BS_Surface* s) {
    if (!s) return;
    if (s->pixels) {
        heap_caps_free(s->pixels);
        s->pixels = NULL;
    }
    heap_caps_free(s);
}

void BS_SurfaceFinalize(BS_Surface* s) {
    if (!s || !s->pixels) return;
    PAL_PPA_FlushSurface(s);
    s->ppa_src_ready = true;
}

// Job ids for the composite ops below. Ids only have to be unique among the
// jobs in flight, and each of these drains its own before returning.
#define JOB_COMPOSITE_BLIT 10u
#define JOB_COMPOSITE_FILL 11u

// A PPA op that writes `dst` is only safe if it covers the WHOLE surface.
// Handing the result back to the CPU means invalidating dst's cache lines,
// and an invalidate throws away any dirty line in the range -- which would
// silently drop CPU writes. When the op overwrites every pixel there is
// nothing worth keeping, so the question does not arise. This is exactly the
// shape of the ops worth offloading anyway: the frame-opening background
// copy and the full-screen clear.
static bool covers_whole_surface(const BS_Surface* dst, int x, int y, int w, int h) {
    return x == 0 && y == 0 && w == dst->w && h == dst->h;
}

BS_Surface* BS_DupSurface(const BS_Surface* src) {
    if (!src) return NULL;
    BS_Surface* dst = BS_CreateSurface(src->w, src->h);
    if (!dst) return NULL;
    memcpy(dst->pixels, src->pixels, (size_t)src->w * src->h * sizeof(Uint32));
    dst->colorkey = src->colorkey;
    dst->colorkey_enabled = src->colorkey_enabled;
    return dst;
}

int BS_BlitSurface(BS_Surface* src, const SDL_Rect* srcrect,
                   BS_Surface* dst, SDL_Rect* dstrect) {
    if (!src || !dst) return -1;

    // Source rect
    int sx = 0, sy = 0, sw = src->w, sh = src->h;
    if (srcrect) {
        sx = srcrect->x; sy = srcrect->y;
        sw = srcrect->w; sh = srcrect->h;
    }

    // Dest position
    int dx = 0, dy = 0;
    if (dstrect) {
        dx = dstrect->x; dy = dstrect->y;
    }

    // Clip to destination bounds
    int copy_w = sw, copy_h = sh;
    if (dx < 0) { sx -= dx; copy_w += dx; dx = 0; }
    if (dy < 0) { sy -= dy; copy_h += dy; dy = 0; }
    if (dx + copy_w > dst->w) copy_w = dst->w - dx;
    if (dy + copy_h > dst->h) copy_h = dst->h - dy;
    if (sx + copy_w > src->w) copy_w = src->w - sx;
    if (sy + copy_h > src->h) copy_h = src->h - sy;
    if (copy_w <= 0 || copy_h <= 0) return 0;

    bool use_colorkey = src->colorkey_enabled;
    Uint32 ck = src->colorkey;

    // Whole-surface opaque copy from a finalized source: let the PPA do it.
    // This is the per-frame background blit, the single biggest CPU cost in a
    // frame after the flip. Anything else -- sprites, colour-keyed art, a
    // source the CPU still writes to -- stays on the loop below.
    if (!use_colorkey && src->ppa_src_ready &&
        covers_whole_surface(dst, dx, dy, copy_w, copy_h) &&
        PAL_PPA_Blit(src, JOB_COMPOSITE_BLIT, sx, sy, copy_w, copy_h, dst, dx, dy)) {
        PAL_PPA_WaitJob(JOB_COMPOSITE_BLIT);
        PAL_PPA_InvalidateSurface(dst);
        return 0;
    }

    for (int row = 0; row < copy_h; row++) {
        const Uint32* srow = src->pixels + (sy + row) * src->w + sx;
        Uint32*       drow = dst->pixels + (dy + row) * dst->w + dx;
        if (use_colorkey) {
            for (int col = 0; col < copy_w; col++) {
                Uint32 p = srow[col];
                if ((p & 0x00FFFFFF) != (ck & 0x00FFFFFF)) {
                    drow[col] = p;
                }
            }
        } else {
            for (int col = 0; col < copy_w; col++) {
                Uint32 p = srow[col];
                if (p & 0xFF000000) {  // skip fully transparent (alpha=0) pixels
                    drow[col] = p;
                }
            }
        }
    }
    return 0;
}

int BS_FillRect(BS_Surface* dst, const SDL_Rect* rect, Uint32 color) {
    if (!dst) return -1;
    int x = 0, y = 0, w = dst->w, h = dst->h;
    if (rect) {
        x = rect->x; y = rect->y; w = rect->w; h = rect->h;
    }
    // Clip
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > dst->w) w = dst->w - x;
    if (y + h > dst->h) h = dst->h - y;
    if (w <= 0 || h <= 0) return 0;

    // Whole-surface clear: see covers_whole_surface().
    if (covers_whole_surface(dst, x, y, w, h) &&
        PAL_PPA_Fill(dst, JOB_COMPOSITE_FILL, x, y, w, h, color)) {
        PAL_PPA_WaitJob(JOB_COMPOSITE_FILL);
        PAL_PPA_InvalidateSurface(dst);
        return 0;
    }

    for (int row = 0; row < h; row++) {
        Uint32* drow = dst->pixels + (y + row) * dst->w + x;
        for (int col = 0; col < w; col++) {
            drow[col] = color;
        }
    }
    return 0;
}

int BS_SetColorKey(BS_Surface* s, Uint32 flag, Uint32 key) {
    if (!s) return -1;
    if (flag & SDL_SRCCOLORKEY) {
        s->colorkey_enabled = true;
        s->colorkey = key;
    } else {
        s->colorkey_enabled = false;
    }
    return 0;
}
