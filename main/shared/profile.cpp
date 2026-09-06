// BlinkenSisters -- frame profiler (see profile.h)

#include "profile.h"

#ifndef DISABLE_FRAME_PROFILER

extern "C" {
#include "esp_log.h"
#include "esp_timer.h"
}
#include <string.h>

static const char* TAG = "profile";

// How often to print the table.
#define PROF_REPORT_MS 3000

static const char* const zone_names[PROF_ZONE_COUNT] = {
    "physics", "background", "fgobjects", "tiles", "pixels",
    "sprites", "lua", "hud", "rotate", "panel blit",
};

static const char* const sub_names[PROF_SUB_COUNT] = { "cache sync", "ppa wait" };

static int64_t  s_sub_start[PROF_SUB_COUNT];
static int64_t  s_sub_total[PROF_SUB_COUNT];
static int64_t  s_zone_start[PROF_ZONE_COUNT];
static int64_t  s_zone_total[PROF_ZONE_COUNT];   // microseconds since the report
static uint32_t s_frames      = 0;
static int64_t  s_window_start = 0;
static int64_t  s_last_frame_end = 0;
static const char* s_rotation_path = "?";

void profSetRotationPath(const char* name) {
    s_rotation_path = name ? name : "?";
}

void profZoneBegin(prof_zone_t zone) {
    if (zone < 0 || zone >= PROF_ZONE_COUNT) return;
    s_zone_start[zone] = esp_timer_get_time();
}

void profZoneEnd(prof_zone_t zone) {
    if (zone < 0 || zone >= PROF_ZONE_COUNT) return;
    if (s_zone_start[zone] == 0) return;          // never begun; ignore
    s_zone_total[zone] += esp_timer_get_time() - s_zone_start[zone];
    s_zone_start[zone] = 0;
}

void profSubBegin(prof_sub_t sub) {
    if (sub < 0 || sub >= PROF_SUB_COUNT) return;
    s_sub_start[sub] = esp_timer_get_time();
}

void profSubEnd(prof_sub_t sub) {
    if (sub < 0 || sub >= PROF_SUB_COUNT) return;
    if (s_sub_start[sub] == 0) return;
    s_sub_total[sub] += esp_timer_get_time() - s_sub_start[sub];
    s_sub_start[sub] = 0;
}

void profReset(void) {
    memset(s_zone_total, 0, sizeof(s_zone_total));
    memset(s_zone_start, 0, sizeof(s_zone_start));
    memset(s_sub_total, 0, sizeof(s_sub_total));
    memset(s_sub_start, 0, sizeof(s_sub_start));
    s_frames = 0;
    s_window_start = esp_timer_get_time();
}

void profFrameEnd(void) {
    int64_t now = esp_timer_get_time();
    if (s_window_start == 0) {
        s_window_start = now;
        s_last_frame_end = now;
        return;
    }
    s_frames++;
    s_last_frame_end = now;

    int64_t elapsed_us = now - s_window_start;
    if (elapsed_us < (int64_t)PROF_REPORT_MS * 1000 || s_frames == 0) {
        return;
    }

    // Everything is per-frame averages: that is the number you can compare
    // against the frame budget.
    double frames   = (double)s_frames;
    double frame_ms = (double)elapsed_us / 1000.0 / frames;
    double fps      = 1000.0 / (frame_ms > 0.0 ? frame_ms : 1.0);

    int64_t accounted = 0;
    for (int i = 0; i < PROF_ZONE_COUNT; i++) {
        accounted += s_zone_total[i];
    }

    ESP_LOGI(TAG, "--- %.1f fps (%.1f ms/frame, %u frames, rotation=%s) ---",
             fps, frame_ms, (unsigned)s_frames, s_rotation_path);
    for (int i = 0; i < PROF_ZONE_COUNT; i++) {
        double ms  = (double)s_zone_total[i] / 1000.0 / frames;
        double pct = elapsed_us ? (100.0 * (double)s_zone_total[i] / (double)elapsed_us) : 0.0;
        if (ms < 0.005 && pct < 0.05) {
            continue;   // idle zone, do not clutter the table
        }
        ESP_LOGI(TAG, "  %-11s %7.2f ms  %5.1f%%", zone_names[i], ms, pct);
    }
    double other_ms = ((double)elapsed_us - (double)accounted) / 1000.0 / frames;
    double other_pct = elapsed_us ? (100.0 * ((double)elapsed_us - (double)accounted) / (double)elapsed_us) : 0.0;
    // "other" is everything outside the measured zones: the frame-rate limiter
    // in renderEngine, input handling, and whatever is not instrumented yet.
    ESP_LOGI(TAG, "  %-11s %7.2f ms  %5.1f%%", "other/idle", other_ms, other_pct);
    for (int i = 0; i < PROF_SUB_COUNT; i++) {
        double ms  = (double)s_sub_total[i] / 1000.0 / frames;
        double pct = elapsed_us ? (100.0 * (double)s_sub_total[i] / (double)elapsed_us) : 0.0;
        if (ms < 0.005) continue;
        ESP_LOGI(TAG, "  (of which %-11s %7.2f ms  %5.1f%%)", sub_names[i], ms, pct);
    }

    memset(s_sub_total, 0, sizeof(s_sub_total));
    memset(s_zone_total, 0, sizeof(s_zone_total));
    s_frames = 0;
    s_window_start = now;
}

#endif // DISABLE_FRAME_PROFILER
