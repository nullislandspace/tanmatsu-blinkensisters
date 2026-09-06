#pragma once
// =====================================================================
//  BlinkenSisters -- pal_ppa.h
//
//  ESP32-P4 PPA (Pixel-Processing Accelerator) offload for the 2D work
//  that builds a frame: the full-screen background copy, full-screen
//  fills, and -- the big one -- the logical-to-panel rotation in BS_Flip.
//
//  Modelled on SynthEngine3D's se_ppa (see
//  ../../../tanmatsu-synthracer-grace/synthengine3D/docs/ppa.md), adapted
//  to BS_Surface instead of pax_buf_t. Same shape: one client per op type,
//  submits are non-blocking enqueues, and a single pump task drains the
//  queue and runs jobs ONE AT A TIME IN SUBMISSION ORDER. The pump exists
//  because the PPA does not guarantee ordering across client types, and
//  the obvious fix -- chaining from the completion callback -- is illegal
//  (that callback runs in ISR context, and the driver's submit functions
//  take blocking locks).
//
//  THREADING: single producer. All submits and waits come from the game
//  task. A failed init is not fatal: every submit then returns false and
//  callers fall back to their CPU path.
//
//  CACHE COHERENCY is the caller's job and is NOT optional. The PPA reads
//  and writes PSRAM by DMA, behind the CPU's cache:
//    * before the PPA READS a surface the CPU drew into -> FlushSurface()
//    * before the CPU reads a surface the PPA WROTE      -> InvalidateSurface()
//  Both are whole-surface bulk operations, cheap next to the per-pixel
//  loops they replace.
// =====================================================================

#include "pal_surface.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Bring up the compositor: register the SRM / FILL clients and start the
// pump task. Idempotent. Returns false (logged) on failure, after which
// PAL_PPA_Available() is false and every submit is a no-op returning false.
bool PAL_PPA_Init(void);

// Whether the hardware path is usable. Callers with a CPU fallback should
// branch on the return value of the submit instead; this is for setup code.
bool PAL_PPA_Available(void);

// --- Cache maintenance -------------------------------------------------

// Push a CPU-drawn surface out of cache to PSRAM, so PPA DMA reads what the
// CPU actually wrote. Call after CPU drawing, before submitting an op that
// reads `s`.
void PAL_PPA_FlushSurface(const BS_Surface* s);

// Drop cached lines for a surface the PPA has written, so CPU reads see the
// new pixels. Call after waiting for an op that wrote `s`, before the CPU
// draws into it.
void PAL_PPA_InvalidateSurface(const BS_Surface* s);

// --- Submits (non-blocking; job_id is the caller's label) --------------
// Each returns true if enqueued, false if refused (queue full, unsupported
// geometry, or PPA not initialised). Never wait on an id whose submit
// returned false -- it never ran, so it never completes.

// FILL the rect (x,y,w,h) of `dst` with `rgba` (a BS_MapRGBA()-style pixel).
bool PAL_PPA_Fill(BS_Surface* dst, uint32_t job_id,
                  int x, int y, int w, int h, Uint32 rgba);

// 1:1 copy of the w*h rect at (sx,sy) in `src` to (dx,dy) in `dst`.
// This is a straight block move: no colour-key, no alpha test, no scaling.
// Sprites that need transparency must stay on the CPU blitter.
bool PAL_PPA_Blit(const BS_Surface* src, uint32_t job_id,
                  int sx, int sy, int w, int h,
                  BS_Surface* dst, int dx, int dy);

// Rotate the logical screen 90 degrees clockwise into the panel's physical
// framebuffer, converting RGBA8888 -> BGR888 on the way. This replaces the
// per-pixel CPU rotate, whose inner loop strode the destination by a full
// row (1440 bytes) and so missed cache on essentially every one of the
// 384000 pixels. `phys` must be cache-line aligned, as must `phys_size`.
// `rgb_swap` asks the PPA to transpose the input's red and blue. Its exact
// meaning is not something the driver documents unambiguously ("ARGB becomes
// BGRA" reads as a byte reversal, "RGB becomes BGR" as a channel swap), so
// callers determine the right value once at start-up by running both against
// the CPU reference -- see BS_InitScreen.
bool PAL_PPA_FlipToPanel(const BS_Surface* screen, uint32_t job_id,
                         void* phys, size_t phys_size,
                         int phys_w, int phys_h, bool rgb_swap);

// --- Completion --------------------------------------------------------

// Block until `job_id` has completed. Because execution is in submission
// order, everything submitted before it is done too. Guarded by a per-op
// timeout so a wedged op cannot hang the game.
void PAL_PPA_WaitJob(uint32_t job_id);

// Block until every submitted job has completed.
void PAL_PPA_WaitAll(void);

// How many submitted jobs have not yet been drained by a wait.
int PAL_PPA_Pending(void);

#ifdef __cplusplus
}
#endif
