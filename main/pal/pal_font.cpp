#include "pal_font.h"
#include "pal_screen.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

// Include Hershey font data (hershey_font.h already includes hershey.h)
#include "../hershey_font.h"

static const char* TAG = "pal_font";

// Font instances
static TTF_Font s_font_menufont_50 = {50};
static TTF_Font s_font_menufont_30 = {30};
static TTF_Font s_font_textfont_30 = {30};
static TTF_Font s_font_menufont_20 = {20};
static TTF_Font s_font_textfont_20 = {20};
static TTF_Font s_font_textfont_12 = {12};
static TTF_Font s_font_textfont_8  = {8};

TTF_Font* FONT_menufont_50 = &s_font_menufont_50;
TTF_Font* FONT_menufont_30 = &s_font_menufont_30;
TTF_Font* FONT_textfont_30 = &s_font_textfont_30;
TTF_Font* FONT_menufont_20 = &s_font_menufont_20;
TTF_Font* FONT_textfont_20 = &s_font_textfont_20;
TTF_Font* FONT_textfont_12 = &s_font_textfont_12;
TTF_Font* FONT_textfont_8  = &s_font_textfont_8;

SDL_Color BS_Color_WHITE   = { 0xff, 0xff, 0xff, 0 };
SDL_Color BS_Color_RED     = { 0xff, 0x00, 0x00, 0 };
// MENUCOLOR_ACTIVE and MENUCOLOR_INACTIVE are defined in menu.cpp
// They are declared extern here and in pal_font.h to resolve link order issues
SDL_Color MENUCOLOR_ACTIVE   = { 0xff, 0xff, 0xff, 0 };
SDL_Color MENUCOLOR_INACTIVE = { 0xa0, 0xa0, 0xa0, 0 };

void initFontHandler(void) {
    ESP_LOGI(TAG, "Hershey font handler initialized");
}

void deInitFontHandler(void) {
    // nothing to free
}

// Internal: measure text width in pixels for a single line
static int hershey_text_width(const char* text, float scale) {
    int width = 0;
    for (const char* p = text; *p && *p != '\n'; p++) {
        int idx = (int)(unsigned char)*p - 32;
        if (idx < 0 || idx >= 95) { width += (int)(16 * scale); continue; }
        width += (int)(simplex[idx][1] * scale);
    }
    return width;
}

// Internal: count lines in text
static int count_lines(const char* text) {
    int lines = 1;
    for (const char* p = text; *p; p++) {
        if (*p == '\n') lines++;
    }
    return lines;
}

int fontHandlerTextWidth(const char* text, TTF_Font* renderfont) {
    if (!text) return 0;
    float font_height = renderfont ? (float)renderfont->size : 20.0f;
    float scale = font_height / (float)HERSHEY_BASE_HEIGHT;

    int widest = 0;
    const char* line_start = text;
    while (line_start && *line_start) {
        const char* line_end = strchr(line_start, '\n');
        size_t len = line_end ? (size_t)(line_end - line_start) : strlen(line_start);
        char line_buf[512];
        if (len >= sizeof(line_buf)) len = sizeof(line_buf) - 1;
        memcpy(line_buf, line_start, len);
        line_buf[len] = '\0';
        int w = hershey_text_width(line_buf, scale);
        if (w > widest) widest = w;
        if (!line_end) break;
        line_start = line_end + 1;
    }
    return widest;
}

void renderFontHandlerText(Sint32 x, Sint32 y, const char* text,
                           SDL_Color fontcolor, bool hcentered, bool vcentered,
                           TTF_Font* renderfont) {
    if (!text || !gScreen) return;

    float font_height = renderfont ? (float)renderfont->size : 20.0f;
    float scale = font_height / (float)HERSHEY_BASE_HEIGHT;
    int line_height = (int)(font_height * 1.3f);

    uint8_t r = fontcolor.r;
    uint8_t g = fontcolor.g;
    uint8_t b = fontcolor.b;
    Uint32 pixel_color = 0xff000000 | ((Uint32)b << 16) | ((Uint32)g << 8) | (Uint32)r;

    // Count lines for vcentering
    int num_lines = count_lines(text);
    Sint32 draw_y = y;
    if (vcentered) {
        draw_y = y - (num_lines * line_height) / 2;
    }

    // Draw each line
    const char* line_start = text;
    while (line_start && *line_start) {
        // Find end of line
        const char* line_end = strchr(line_start, '\n');
        size_t len;
        if (line_end) {
            len = (size_t)(line_end - line_start);
        } else {
            len = strlen(line_start);
        }

        // Build line buffer
        char line_buf[512];
        if (len >= sizeof(line_buf)) len = sizeof(line_buf) - 1;
        memcpy(line_buf, line_start, len);
        line_buf[len] = '\0';

        Sint32 draw_x = x;
        if (hcentered) {
            int tw = hershey_text_width(line_buf, scale);
            draw_x = (LOG_W - tw) / 2;
        }

        // Draw each character directly to gScreen->pixels
        int cx = draw_x;
        for (size_t ci = 0; ci < len; ci++) {
            unsigned char c = (unsigned char)line_buf[ci];
            int idx = (int)c - 32;
            if (idx < 0 || idx >= 95) {
                cx += (int)(16 * scale);
                continue;
            }
            const int* glyph = simplex[idx];
            int num_verts = glyph[0];
            int char_width = glyph[1];
            if (num_verts == 0) {
                cx += (int)(char_width * scale);
                continue;
            }

            // Draw glyph strokes using Bresenham line algorithm
            int pen_down = 0;
            int prev_px = 0, prev_py = 0;
            for (int vi = 0; vi < num_verts; vi++) {
                int vx = glyph[2 + vi * 2];
                int vy = glyph[2 + vi * 2 + 1];
                if (vx == -1 && vy == -1) {
                    pen_down = 0;
                    continue;
                }
                int px = cx + (int)(vx * scale);
                int py = draw_y + (int)((HERSHEY_BASE_HEIGHT - vy) * scale);
                if (pen_down) {
                    // Bresenham line from (prev_px, prev_py) to (px, py)
                    int x0 = prev_px, y0 = prev_py, x1 = px, y1 = py;
                    int dx = abs(x1 - x0);
                    int dy = abs(y1 - y0);
                    int sx2 = (x0 < x1) ? 1 : -1;
                    int sy2 = (y0 < y1) ? 1 : -1;
                    int err2 = dx - dy;
                    while (1) {
                        if (x0 >= 0 && x0 < LOG_W && y0 >= 0 && y0 < LOG_H) {
                            gScreen->pixels[y0 * LOG_W + x0] = pixel_color;
                        }
                        if (x0 == x1 && y0 == y1) break;
                        int e2 = 2 * err2;
                        if (e2 > -dy) { err2 -= dy; x0 += sx2; }
                        if (e2 < dx)  { err2 += dx; y0 += sy2; }
                    }
                }
                prev_px = px; prev_py = py;
                pen_down = 1;
            }
            cx += (int)(char_width * scale);
        }

        draw_y += line_height;
        if (line_end) {
            line_start = line_end + 1;
        } else {
            break;
        }
    }
}
