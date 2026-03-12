#pragma once
#include "pal_surface.h"

#ifdef __cplusplus
extern "C" {
#endif

// Physical dimensions (portrait orientation)
#define PHYS_W 480
#define PHYS_H 800
#define PHYS_STRIDE (PHYS_W * 3)  // bytes per row in physical fb

// Logical dimensions (landscape, game space)
#define LOG_W 800
#define LOG_H 480

extern BS_Surface *gScreen;    // Logical 800x480 RGBA32 framebuffer in PSRAM

// Initialize screen (allocates gScreen and phys_fb)
void BS_InitScreen(void);

// Push logical screen -> physical display (270 degree rotation)
int BS_Flip(BS_Surface* screen);

// Stub for SDL_SetVideoMode / BS_SetVideoMode
BS_Surface* BS_SetVideoMode(Uint32 width, Uint32 height, Uint32 depth, Uint32 flags);

// Color3D stubs (no-ops on Tanmatsu)
typedef enum { COLOR3D_NONE = 0, COLOR3D_LEFT, COLOR3D_RIGHT } COLOR3D;
static inline void BS_Set3DMode(COLOR3D m) { (void)m; }
static inline void BS_ColorSurfacesToScreen(void) {}
static inline void BS_ColorToGreyscale(void) {}
static inline void BS_SetRealResolution(Uint32 w, Uint32 h, bool s) { (void)w; (void)h; (void)s; }
static inline void BS_OLPCScreenZoom(BS_Surface* src, BS_Surface* dst) { (void)src; (void)dst; }

// Display initialization
void BS_InitDisplay(void);

#ifdef __cplusplus
}
#endif
