#include "globals.h"
#include "drawprimitives.h"
#include "errorhandler.h"
#include <math.h>
#include <string.h>
#include <strings.h>
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "fastopen.h"
#include "driver/jpeg_decode.h"
#include "lodepng.h"

static const char* TAG = "drawprim";

// --- lodepng PSRAM allocators (required when LODEPNG_NO_COMPILE_ALLOCATORS is set) ---
void* lodepng_malloc(size_t size) {
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
}
void* lodepng_realloc(void* ptr, size_t new_size) {
    return heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM);
}
void lodepng_free(void* ptr) {
    heap_caps_free(ptr);
}

// --- Basic drawing functions ---

void drawrect(const Sint32 x, const Sint32 y, const Sint32 width, const Sint32 height, const Uint32 color) {
    SDL_Rect r = { (Sint16)x, (Sint16)y, (Uint16)width, (Uint16)height };
    SDL_FillRect(gScreen, &r, color);
}

Uint32 getpixel(SDL_Surface* screen, int x, int y) {
    return BS_GetPixel(screen, x, y);
}

Uint32 SDL_color_to_Uint32(SDL_Color sdc) {
    return BS_MapRGBA(sdc.r, sdc.g, sdc.b, 0xff);
}

// --- BMP loader (24-bit and 32-bit uncompressed) ---
static BS_Surface* load_bmp(const char* path) {
    FILE* f = fastopen(path, "rb");
    if (!f) {
        ESP_LOGW(TAG, "BMP: cannot open %s", path);
        return NULL;
    }

    // File header: 14 bytes
    uint8_t fhdr[14];
    if (fread(fhdr, 1, 14, f) != 14 || fhdr[0] != 'B' || fhdr[1] != 'M') {
        fastclose(f);
        ESP_LOGW(TAG, "BMP: bad magic %s", path);
        return NULL;
    }
    uint32_t px_offset = (uint32_t)fhdr[10] | ((uint32_t)fhdr[11] << 8) |
                         ((uint32_t)fhdr[12] << 16) | ((uint32_t)fhdr[13] << 24);

    // DIB header: 40 bytes (BITMAPINFOHEADER)
    uint8_t dib[40];
    if (fread(dib, 1, 40, f) != 40) {
        fastclose(f);
        ESP_LOGW(TAG, "BMP: short DIB header %s", path);
        return NULL;
    }
    int32_t  bw   = (int32_t)((uint32_t)dib[4]  | ((uint32_t)dib[5]  << 8) | ((uint32_t)dib[6]  << 16) | ((uint32_t)dib[7]  << 24));
    int32_t  bh   = (int32_t)((uint32_t)dib[8]  | ((uint32_t)dib[9]  << 8) | ((uint32_t)dib[10] << 16) | ((uint32_t)dib[11] << 24));
    uint16_t bpp  = (uint16_t)(dib[14] | (dib[15] << 8));
    uint32_t comp = (uint32_t)dib[16] | ((uint32_t)dib[17] << 8) | ((uint32_t)dib[18] << 16) | ((uint32_t)dib[19] << 24);

    if (comp != 0) {
        fastclose(f);
        ESP_LOGW(TAG, "BMP: compressed format unsupported %s", path);
        return NULL;
    }
    if (bpp != 8 && bpp != 24 && bpp != 32) {
        fastclose(f);
        ESP_LOGW(TAG, "BMP: unsupported bpp=%d in %s", bpp, path);
        return NULL;
    }

    int w = (bw < 0) ? -bw : bw;
    bool top_down = (bh < 0);
    int h = top_down ? -bh : bh;

    // Read palette for 8-bit images
    uint8_t palette[256 * 4] = {0};  // BGRA entries
    if (bpp == 8) {
        uint32_t num_colors = (uint32_t)dib[32] | ((uint32_t)dib[33] << 8) |
                              ((uint32_t)dib[34] << 16) | ((uint32_t)dib[35] << 24);
        if (num_colors == 0) num_colors = 256;
        if (num_colors > 256) num_colors = 256;
        // Palette starts right after the DIB header (offset 14+40=54)
        fseek(f, 14 + 40, SEEK_SET);
        fread(palette, 4, num_colors, f);
    }

    int bytes_pp = (bpp == 8) ? 1 : bpp / 8;
    int row_stride = ((w * bytes_pp + 3) / 4) * 4;

    fseek(f, (long)px_offset, SEEK_SET);
    size_t px_size = (size_t)row_stride * (size_t)h;
    uint8_t* px = (uint8_t*)heap_caps_malloc(px_size, MALLOC_CAP_SPIRAM);
    if (!px) {
        fastclose(f);
        ESP_LOGE(TAG, "BMP: OOM reading %s", path);
        return NULL;
    }
    if (fread(px, 1, px_size, f) != px_size) {
        heap_caps_free(px);
        fastclose(f);
        ESP_LOGW(TAG, "BMP: short pixel data %s", path);
        return NULL;
    }
    fastclose(f);

    BS_Surface* surf = BS_CreateSurface(w, h);
    if (!surf) { heap_caps_free(px); return NULL; }

    for (int row = 0; row < h; row++) {
        int src_row = top_down ? row : (h - 1 - row);
        const uint8_t* src = px + src_row * row_stride;
        Uint32* dst = surf->pixels + row * w;
        if (bpp == 8) {
            for (int col = 0; col < w; col++) {
                uint8_t idx = src[col];
                uint8_t b = palette[idx*4+0], g = palette[idx*4+1], r = palette[idx*4+2];
                dst[col] = BS_MapRGBA(r, g, b, 0xff);
            }
        } else if (bpp == 24) {
            for (int col = 0; col < w; col++) {
                uint8_t b = src[col*3+0], g = src[col*3+1], r = src[col*3+2];
                dst[col] = BS_MapRGBA(r, g, b, 0xff);
            }
        } else {
            for (int col = 0; col < w; col++) {
                uint8_t b = src[col*4+0], g = src[col*4+1], r = src[col*4+2], a = src[col*4+3];
                dst[col] = BS_MapRGBA(r, g, b, a);
            }
        }
    }

    heap_caps_free(px);
    ESP_LOGI(TAG, "BMP: loaded %s (%dx%d %dbpp)", path, w, h, bpp);
    return surf;
}

// --- PNG loader using lodepng (in-memory decode to PSRAM) ---
static BS_Surface* load_png(const char* path) {
    FILE* f = fastopen(path, "rb");
    if (!f) {
        ESP_LOGW(TAG, "PNG: cannot open %s", path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    size_t fsize = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t* filedata = (uint8_t*)heap_caps_malloc(fsize, MALLOC_CAP_SPIRAM);
    if (!filedata) {
        fastclose(f);
        ESP_LOGE(TAG, "PNG: OOM reading %s", path);
        return NULL;
    }
    fread(filedata, 1, fsize, f);
    fastclose(f);

    // lodepng_decode32 outputs RGBA bytes; allocation goes to PSRAM via custom allocators
    uint8_t* image = NULL;
    unsigned w = 0, h = 0;
    unsigned error = lodepng_decode32(&image, &w, &h, filedata, fsize);
    heap_caps_free(filedata);

    if (error) {
        ESP_LOGW(TAG, "PNG: decode error %u for %s", error, path);
        if (image) lodepng_free(image);
        return NULL;
    }

    BS_Surface* surf = BS_CreateSurface((Sint32)w, (Sint32)h);
    if (!surf) { lodepng_free(image); return NULL; }

    // lodepng RGBA8888: byte[0]=R, [1]=G, [2]=B, [3]=A
    for (unsigned i = 0; i < w * h; i++) {
        uint8_t r = image[i*4+0], g = image[i*4+1], b = image[i*4+2], a = image[i*4+3];
        surf->pixels[i] = BS_MapRGBA(r, g, b, a);
    }

    lodepng_free(image);
    ESP_LOGI(TAG, "PNG: loaded %s (%ux%u)", path, w, h);
    return surf;
}

// --- JPEG loader using ESP32-P4 hardware JPEG decoder ---
static jpeg_decoder_handle_t s_jpeg_decoder = NULL;

static BS_Surface* load_jpeg(const char* path) {
    if (!s_jpeg_decoder) {
        jpeg_decode_engine_cfg_t eng_cfg = {
            .intr_priority = 0,
            .timeout_ms = 5000,
        };
        esp_err_t err = jpeg_new_decoder_engine(&eng_cfg, &s_jpeg_decoder);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "JPEG: failed to create decoder: %d", err);
            return NULL;
        }
    }

    FILE* f = fastopen(path, "rb");
    if (!f) {
        ESP_LOGW(TAG, "JPEG: cannot open %s", path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    size_t fsize = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);

    jpeg_decode_memory_alloc_cfg_t in_cfg  = { .buffer_direction = JPEG_DEC_ALLOC_INPUT_BUFFER };
    size_t in_alloc = 0;
    uint8_t* inbuf = (uint8_t*)jpeg_alloc_decoder_mem(fsize, &in_cfg, &in_alloc);
    if (!inbuf) {
        fastclose(f);
        ESP_LOGE(TAG, "JPEG: OOM input buffer %s", path);
        return NULL;
    }
    fread(inbuf, 1, fsize, f);
    fastclose(f);

    jpeg_decode_picture_info_t info = {};
    if (jpeg_decoder_get_info(inbuf, (uint32_t)fsize, &info) != ESP_OK) {
        heap_caps_free(inbuf);
        ESP_LOGW(TAG, "JPEG: failed to parse header %s", path);
        return NULL;
    }

    uint32_t w = info.width;
    uint32_t h = info.height;
    // Hardware decoder output must be padded to 16-pixel MCU boundaries
    uint32_t padded_w = (w + 15u) & ~15u;
    uint32_t padded_h = (h + 15u) & ~15u;
    size_t out_buf_size = (size_t)padded_w * padded_h * 3;

    ESP_LOGI(TAG, "JPEG: %s %" PRIu32 "x%" PRIu32 " (padded %" PRIu32 "x%" PRIu32 "), need %u bytes",
             path, w, h, padded_w, padded_h, (unsigned)out_buf_size);

    jpeg_decode_memory_alloc_cfg_t out_cfg = { .buffer_direction = JPEG_DEC_ALLOC_OUTPUT_BUFFER };
    size_t out_alloc = 0;
    uint8_t* outbuf = (uint8_t*)jpeg_alloc_decoder_mem(out_buf_size, &out_cfg, &out_alloc);
    if (!outbuf) {
        // Fallback: allocate from PSRAM with 64-byte cache alignment
        out_alloc = (out_buf_size + 63u) & ~63u;
        outbuf = (uint8_t*)heap_caps_aligned_alloc(64, out_alloc, MALLOC_CAP_SPIRAM);
        if (outbuf) {
            memset(outbuf, 0, out_alloc);
            ESP_LOGW(TAG, "JPEG: using manual PSRAM alloc for %s", path);
        }
    }
    if (!outbuf) {
        heap_caps_free(inbuf);
        ESP_LOGE(TAG, "JPEG: OOM output buffer %s (need %u, free PSRAM %u)",
                 path, (unsigned)out_buf_size,
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
        return NULL;
    }

    jpeg_decode_cfg_t decode_cfg = {
        .output_format = JPEG_DECODE_OUT_FORMAT_RGB888,
        .rgb_order     = JPEG_DEC_RGB_ELEMENT_ORDER_BGR,
        .conv_std      = JPEG_YUV_RGB_CONV_STD_BT601,
    };
    uint32_t out_size = 0;
    esp_err_t err = jpeg_decoder_process(s_jpeg_decoder, &decode_cfg,
                                         inbuf, (uint32_t)fsize,
                                         outbuf, (uint32_t)out_alloc, &out_size);
    heap_caps_free(inbuf);
    if (err != ESP_OK) {
        heap_caps_free(outbuf);
        ESP_LOGW(TAG, "JPEG: decode failed %s (err=%d)", path, err);
        return NULL;
    }

    BS_Surface* surf = BS_CreateSurface((Sint32)w, (Sint32)h);
    if (!surf) { heap_caps_free(outbuf); return NULL; }

    // BGR888 with padded row stride -> BS_Surface RGBA32
    for (uint32_t row = 0; row < h; row++) {
        for (uint32_t col = 0; col < w; col++) {
            const uint8_t* src = outbuf + (row * padded_w + col) * 3;
            uint8_t b = src[0], g = src[1], r = src[2];
            surf->pixels[row * w + col] = BS_MapRGBA(r, g, b, 0xff);
        }
    }

    heap_caps_free(outbuf);
    ESP_LOGI(TAG, "JPEG: loaded %s (%" PRIu32 "x%" PRIu32 ")", path, w, h);
    return surf;
}

// --- Main image loader: detect format by extension ---
SDL_Surface* BS_IMG_Load_DisplayFormat(const char* filename, bool die_on_error) {
    if (!filename || !filename[0]) return NULL;

    // Find last '.' in filename
    const char* ext = NULL;
    for (const char* p = filename; *p; p++) {
        if (*p == '.') ext = p;
    }

    BS_Surface* surf = NULL;
    if (ext) {
        if (strcasecmp(ext, ".bmp") == 0) {
            surf = load_bmp(filename);
        } else if (strcasecmp(ext, ".png") == 0) {
            surf = load_png(filename);
        } else if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0) {
            surf = load_jpeg(filename);
        } else {
            ESP_LOGW(TAG, "IMG_Load: unknown extension '%s' for %s", ext, filename);
        }
    } else {
        ESP_LOGW(TAG, "IMG_Load: no extension in %s", filename);
    }

    if (!surf) {
        if (die_on_error) {
            DIE(ERROR_IMAGE_READ, filename);
        }
        // Return 1x1 black placeholder so callers don't crash on NULL
        surf = BS_CreateSurface(1, 1);
        if (surf) surf->pixels[0] = 0xff000000;
    }
    return surf;
}
