// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#ifndef BLENDING_H
#define BLENDING_H

Uint32 blend_avg(const Uint32 source, const Uint32 target);
Uint32 blend_mul(const Uint32 source, const Uint32 target);
Uint32 blend_add(const Uint32 source, const Uint32 target);
void blend_darkenRect(const Sint32 x, const Sint32 y, const Sint32 width, const Sint32 height, const Uint32 factor);
void blend_brightenRect(const Sint32 x, const Sint32 y, const Sint32 width, const Sint32 height, const Uint32 factor);
Uint32 blend_alpha(const Uint32 source, const Uint32 target, const Uint32 factor);

#endif // BLENDING_H
