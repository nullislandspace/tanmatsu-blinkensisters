
#ifndef ZOOMSURFACERGBA_H
#define ZOOMSURFACERGBA_H

#ifdef __cplusplus
extern "C" {
#endif

/* the function _zoomSurfaceRGBA() is defined in SDL_gfx, but not exported. We use that function - therefore we have to define it in a header file */
SDL_ROTOZOOM_SCOPE int _zoomSurfaceRGBA(SDL_Surface * src, SDL_Surface * dst, int flipx, int flipy, int smooth);

#ifdef __cplusplus
}
#endif

#endif // ZOOMSURFACERGBA_H
