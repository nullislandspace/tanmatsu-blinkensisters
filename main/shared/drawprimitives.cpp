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
        BS_Pixel* dst = surf->pixels + (size_t)row * w;
        if (bpp == 8) {
            for (int col = 0; col < w; col++) {
                uint8_t idx = src[col];
                uint8_t b = palette[idx*4+0], g = palette[idx*4+1], r = palette[idx*4+2];
                dst[col] = BS_PackOpaque(BS_MapRGBA(r, g, b, 0xff));
            }
        } else if (bpp == 24) {
            for (int col = 0; col < w; col++) {
                uint8_t b = src[col*3+0], g = src[col*3+1], r = src[col*3+2];
                dst[col] = BS_PackOpaque(BS_MapRGBA(r, g, b, 0xff));
            }
        } else {
            for (int col = 0; col < w; col++) {
                uint8_t b = src[col*4+0], g = src[col*4+1], r = src[col*4+2], a = src[col*4+3];
                // Alpha was only ever a binary skip test, so it becomes the
                // reserved transparent colour.
                dst[col] = a ? BS_PackOpaque(BS_MapRGBA(r, g, b, a)) : BS_TRANSPARENT;
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
        surf->pixels[i] = a ? BS_PackOpaque(BS_MapRGBA(r, g, b, a)) : BS_TRANSPARENT;
    }

    lodepng_free(image);
    ESP_LOGI(TAG, "PNG: loaded %s (%ux%u)", path, w, h);
    return surf;
}

// --- JPEG loader using ESP32-P4 hardware JPEG decoder ---
static jpeg_decoder_handle_t s_jpeg_decoder = NULL;

/* What the SOF0 header says, plus where it sits in the bitstream.
   jpeg_decoder_get_info() reports width, height and a sample_method enum, but
   the driver's own decode path does not use that enum: jpeg_parse_sof_marker()
   takes the MCU size straight from the first component's sampling factors
   (mcux = hi * 8, mcuy = vi * 8). Those two disagree for a single-component
   picture that still carries 2x2 factors -- LostPixels' level6.jpg is exactly
   that -- so the padding is read here the same way the driver computes it,
   rather than inferred from the enum. */
typedef struct {
    uint32_t w, h;
    uint8_t  nf, hi, vi;
    size_t   sof;      // offset of the 0xFF 0xC0 marker
} jpeg_sof_t;

static bool parse_sof0(const uint8_t* buf, size_t len, jpeg_sof_t* out) {
    if (len < 4 || buf[0] != 0xFF || buf[1] != 0xD8) {
        return false;   // not a JPEG
    }
    size_t i = 2;
    while (i + 4 <= len) {
        if (buf[i] != 0xFF) { i++; continue; }
        uint8_t m = buf[i + 1];
        if (m == 0xFF) { i++; continue; }                  // fill byte
        if (m == 0xD8 || m == 0x01 || (m >= 0xD0 && m <= 0xD7)) { i += 2; continue; }
        if (m == 0xD9 || m == 0xDA) { return false; }      // end / scan: no SOF0
        size_t seglen = ((size_t)buf[i + 2] << 8) | buf[i + 3];
        if (seglen < 2 || i + 2 + seglen > len) { return false; }
        if (m == 0xC0) {                                   // baseline SOF0
            if (seglen < 8 + 3) { return false; }
            out->sof = i;
            out->h   = ((uint32_t)buf[i + 5] << 8) | buf[i + 6];
            out->w   = ((uint32_t)buf[i + 7] << 8) | buf[i + 8];
            out->nf  = buf[i + 9];
            out->hi  = (uint8_t)(buf[i + 11] >> 4);
            out->vi  = (uint8_t)(buf[i + 11] & 0x0F);
            return out->nf > 0 && out->hi > 0 && out->vi > 0;
        }
        i += 2 + seglen;
    }
    return false;
}

/* The hardware rejects any picture whose PIXEL COUNT is not a multiple of
   eight -- jpeg_parse_sof_marker() bails out with "Picture sizes not divisible
   by 8 are not supported" on (width * height) % 8. That is a property of the
   product, not of either side, so ordinary sizes fall foul of it: 1341x900,
   1475x661 and 794x1123 are all backgrounds we ship.

   Declaring the picture a few pixels smaller in the SOF gets past it. The
   entropy-coded data is untouched, PROVIDED the smaller size still spans the
   same number of MCUs -- the scan is one long stream of MCUs, so changing the
   count desynchronises the decode and yields garbage. The search below
   therefore holds ceil(w/mcu_w) and ceil(h/mcu_h) fixed, which also leaves the
   padded output size unchanged, and gives up rather than guessing if nothing
   fits.

   Checked against libjpeg on the host over every background shipped here: the
   retained pixels come back identical apart from the final row and column,
   where chroma upsampling replicates a different edge sample and shifts a
   channel by at most 4/255. The visible cost is one to three pixels trimmed
   off the right and bottom of a backdrop. */
static bool find_hw_crop(uint32_t w, uint32_t h, uint32_t mcu_w, uint32_t mcu_h,
                         uint32_t* out_w, uint32_t* out_h) {
    const uint32_t mcus_x = (w + mcu_w - 1u) / mcu_w;
    const uint32_t mcus_y = (h + mcu_h - 1u) / mcu_h;
    for (uint32_t budget = 1; budget <= 16; budget++) {
        for (uint32_t dw = 0; dw <= budget; dw++) {
            uint32_t dh = budget - dw;
            if (dw >= w || dh >= h) continue;
            uint32_t nw = w - dw, nh = h - dh;
            if (((uint64_t)nw * (uint64_t)nh) % 8u) continue;
            if ((nw + mcu_w - 1u) / mcu_w != mcus_x) continue;
            if ((nh + mcu_h - 1u) / mcu_h != mcus_y) continue;
            *out_w = nw; *out_h = nh;
            return true;
        }
    }
    return false;
}

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

    jpeg_sof_t sof;
    if (!parse_sof0(inbuf, fsize, &sof) || sof.w == 0 || sof.h == 0) {
        heap_caps_free(inbuf);
        ESP_LOGW(TAG, "JPEG: no baseline SOF0 in %s (progressive?)", path);
        return NULL;
    }

    uint32_t w = sof.w;
    uint32_t h = sof.h;

    // The decoder writes whole MCUs, so its output picture is the image
    // rounded up to the MCU grid. Take the MCU size from the sampling factors,
    // which is what the driver itself does.
    const uint32_t mcu_w = (uint32_t)sof.hi * 8u;
    const uint32_t mcu_h = (uint32_t)sof.vi * 8u;
    if (mcu_w == 0 || mcu_h == 0 || mcu_w > 32 || mcu_h > 32) {
        heap_caps_free(inbuf);
        ESP_LOGW(TAG, "JPEG: odd sampling %u/%u in %s", sof.hi, sof.vi, path);
        return NULL;
    }

    // Work around the hardware's "pixel count must be a multiple of 8" rule by
    // declaring the picture very slightly smaller (see find_hw_crop).
    if (((uint64_t)w * (uint64_t)h) % 8u) {
        uint32_t cw = 0, ch = 0;
        if (!find_hw_crop(w, h, mcu_w, mcu_h, &cw, &ch)) {
            heap_caps_free(inbuf);
            ESP_LOGE(TAG, "JPEG: %s is %ux%u; the pixel count is not a multiple "
                          "of 8 and no crop within the same MCU grid fixes it",
                     path, (unsigned)w, (unsigned)h);
            return NULL;
        }
        ESP_LOGW(TAG, "JPEG: %s is %ux%u (pixel count not a multiple of 8); "
                      "decoding as %ux%u",
                 path, (unsigned)w, (unsigned)h, (unsigned)cw, (unsigned)ch);
        w = cw; h = ch;
        inbuf[sof.sof + 5] = (uint8_t)(h >> 8);
        inbuf[sof.sof + 6] = (uint8_t)(h & 0xFF);
        inbuf[sof.sof + 7] = (uint8_t)(w >> 8);
        inbuf[sof.sof + 8] = (uint8_t)(w & 0xFF);
    }

    uint32_t padded_w = ((w + mcu_w - 1u) / mcu_w) * mcu_w;
    uint32_t padded_h = ((h + mcu_h - 1u) / mcu_h) * mcu_h;
    size_t out_buf_size = (size_t)padded_w * padded_h * 3;

    ESP_LOGI(TAG, "JPEG: %s %" PRIu32 "x%" PRIu32 " (mcu %" PRIu32 "x%" PRIu32
                  ", padded %" PRIu32 "x%" PRIu32 "), need %u bytes",
             path, w, h, mcu_w, mcu_h, padded_w, padded_h, (unsigned)out_buf_size);

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

    // The decoder reports the size it actually produced. If that disagrees
    // with the padding worked out above, the unpack stride below would be
    // wrong, so refuse rather than draw a sheared picture.
    size_t expect = (size_t)padded_w * padded_h * 3;
    if (out_size != expect) {
        ESP_LOGW(TAG, "JPEG: %s produced %u bytes, expected %u (stride mismatch)",
                 path, (unsigned)out_size, (unsigned)expect);
        heap_caps_free(outbuf);
        return NULL;
    }

    BS_Surface* surf = BS_CreateSurface((Sint32)w, (Sint32)h);
    if (!surf) { heap_caps_free(outbuf); return NULL; }

    // BGR888 with padded row stride -> BS_Surface RGBA32
    for (uint32_t row = 0; row < h; row++) {
        const uint8_t* src = outbuf + (size_t)row * padded_w * 3;
        BS_Pixel* dst = surf->pixels + (size_t)row * w;
        for (uint32_t col = 0; col < w; col++) {
            uint8_t b = src[0], g = src[1], r = src[2];
            src += 3;
            dst[col] = BS_PackOpaque(BS_MapRGBA(r, g, b, 0xff));
        }
    }

    heap_caps_free(outbuf);
    ESP_LOGI(TAG, "JPEG: loaded %s (%" PRIu32 "x%" PRIu32 ")", path, w, h);
    return surf;
}

// --- Bilinear rescale ---
// Used to stretch the fixed-size backdrops to the screen. Bilinear rather
// than nearest because these are photographic images and the scale is a
// gentle 1.25x, where nearest leaves visible stair-stepping.
SDL_Surface* BS_ScaleSurface(const SDL_Surface* src, Sint32 dst_w, Sint32 dst_h) {
    if (!src || !src->pixels || dst_w <= 0 || dst_h <= 0) return NULL;
    if (src->w <= 0 || src->h <= 0) return NULL;

    BS_Surface* dst = BS_CreateSurface(dst_w, dst_h);
    if (!dst) return NULL;

    // 16.16 fixed point step through the source. The -1s map the last output
    // pixel onto the last source pixel, so the image reaches both edges.
    uint32_t stepx = (dst_w > 1) ? (uint32_t)(((uint64_t)(src->w - 1) << 16) / (uint32_t)(dst_w - 1)) : 0;
    uint32_t stepy = (dst_h > 1) ? (uint32_t)(((uint64_t)(src->h - 1) << 16) / (uint32_t)(dst_h - 1)) : 0;

    for (Sint32 y = 0; y < dst_h; y++) {
        uint32_t sy   = (uint32_t)y * stepy;
        Sint32   y0   = (Sint32)(sy >> 16);
        Sint32   y1   = (y0 + 1 < src->h) ? y0 + 1 : y0;
        uint32_t fy   = sy & 0xFFFF;
        const BS_Pixel* row0 = src->pixels + (size_t)y0 * src->w;
        const BS_Pixel* row1 = src->pixels + (size_t)y1 * src->w;
        BS_Pixel* out = dst->pixels + (size_t)y * dst_w;

        for (Sint32 x = 0; x < dst_w; x++) {
            uint32_t sx = (uint32_t)x * stepx;
            Sint32   x0 = (Sint32)(sx >> 16);
            Sint32   x1 = (x0 + 1 < src->w) ? x0 + 1 : x0;
            uint32_t fx = sx & 0xFFFF;

            BS_Pixel q00 = row0[x0], q01 = row0[x1];
            BS_Pixel q10 = row1[x0], q11 = row1[x1];

            // Transparency stays binary: if the nearest source pixel is
            // transparent so is the result, rather than blending the
            // reserved colour into its neighbours.
            if (q00 == BS_TRANSPARENT || q01 == BS_TRANSPARENT ||
                q10 == BS_TRANSPARENT || q11 == BS_TRANSPARENT) {
                out[x] = ((fx < 0x8000) && (fy < 0x8000)) ? q00 :
                         ((fx >= 0x8000) && (fy < 0x8000)) ? q01 :
                         ((fx < 0x8000)) ? q10 : q11;
                continue;
            }

            Uint32 p00 = BS_Unpack(q00), p01 = BS_Unpack(q01);
            Uint32 p10 = BS_Unpack(q10), p11 = BS_Unpack(q11);

            uint32_t acc[3];
            for (int c = 0; c < 3; c++) {
                uint32_t shift = (uint32_t)c * 8;
                uint32_t c00 = (p00 >> shift) & 0xFF;
                uint32_t c01 = (p01 >> shift) & 0xFF;
                uint32_t c10 = (p10 >> shift) & 0xFF;
                uint32_t c11 = (p11 >> shift) & 0xFF;
                uint32_t top = c00 + (((c01 - c00) * fx) >> 16);
                uint32_t bot = c10 + (((c11 - c10) * fx) >> 16);
                acc[c] = top + (((bot - top) * fy) >> 16);
            }
            out[x] = BS_PackOpaque(0xff000000u | (acc[2] << 16) | (acc[1] << 8) | acc[0]);
        }
    }
    return dst;
}

SDL_Surface* BS_IMG_Load_Fullscreen(const char* filename, bool die_on_error) {
    BS_Surface* surf = BS_IMG_Load_DisplayFormat(filename, die_on_error);
    if (!surf) return NULL;
    if (surf->w == SCR_WIDTH && surf->h == SCR_HEIGHT) {
        return surf;
    }
    BS_Surface* scaled = BS_ScaleSurface(surf, SCR_WIDTH, SCR_HEIGHT);
    if (!scaled) {
        ESP_LOGW(TAG, "Fullscreen: cannot scale %s (%dx%d), using as-is",
                 filename, (int)surf->w, (int)surf->h);
        return surf;
    }
    ESP_LOGI(TAG, "Fullscreen: stretched %s from %dx%d to %dx%d",
             filename, (int)surf->w, (int)surf->h, SCR_WIDTH, SCR_HEIGHT);
    BS_FreeSurface(surf);
    return scaled;
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
        return NULL;
    }
    return surf;
}
