#ifndef PROFILE_H
#define PROFILE_H
// =====================================================================
//  BlinkenSisters -- frame profiler
//
//  Wall-clock timing of the phases of a frame, so "the framerate is bad"
//  can be turned into "this phase costs 40% of the frame". Zones are
//  accumulated per frame and reported as a table every PROF_REPORT_MS.
//
//  Zones must not overlap: begin one, end it, begin the next. Nesting is
//  not tracked, so a nested pair would be counted in both.
//
//  Cost when built in is one esp_timer_get_time() per begin/end, which is
//  a register read -- negligible next to the work being measured. Define
//  DISABLE_FRAME_PROFILER to compile the whole thing out.
// =====================================================================

#include "globals.h"

typedef enum {
    PROF_PHYSICS = 0,   // engineFullPhysics / engineMinimalPhysics
    PROF_BACKGROUND,    // drawBackground + drawBackground2
    PROF_FGOBJS,        // paintFGObjs (both passes)
    PROF_TILES,         // paintLevelTiles
    PROF_PIXELS,        // paintLevelPixels
    PROF_SPRITES,       // monster + player sprites
    PROF_LUA,           // all LUA render stages
    PROF_HUD,           // status line, overlays, output filter
    PROF_ROTATE,        // logical -> panel rotation (PPA or CPU)
    PROF_PANEL,         // bsp_display_blit
    PROF_ZONE_COUNT
} prof_zone_t;

/* Sub-zones are NESTED inside the zones above (cache maintenance and PPA
   waits happen during background, rotate and so on), so they are reported
   separately as "of which" and are not part of the frame total. */
typedef enum {
    PROF_SUB_CACHE = 0,   // esp_cache_msync on surfaces
    PROF_SUB_PPAWAIT,     // blocked waiting for a PPA job
    PROF_SUB_COUNT
} prof_sub_t;

#ifndef DISABLE_FRAME_PROFILER

void profZoneBegin(prof_zone_t zone);
void profZoneEnd(prof_zone_t zone);

void profSubBegin(prof_sub_t sub);
void profSubEnd(prof_sub_t sub);

/* Count one displayed frame, and emit the report when due. */
void profFrameEnd(void);

/* Describe the rotation path in the report ("PPA" / "CPU"). */
void profSetRotationPath(const char* name);

/* Throw away accumulated numbers, e.g. after a level load. */
void profReset(void);

#else

static inline void profZoneBegin(prof_zone_t zone) { (void)zone; }
static inline void profZoneEnd(prof_zone_t zone) { (void)zone; }
static inline void profSubBegin(prof_sub_t sub) { (void)sub; }
static inline void profSubEnd(prof_sub_t sub) { (void)sub; }
static inline void profFrameEnd(void) {}
static inline void profSetRotationPath(const char* name) { (void)name; }
static inline void profReset(void) {}

#endif // DISABLE_FRAME_PROFILER

#endif // PROFILE_H
