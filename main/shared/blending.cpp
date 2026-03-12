// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#include "globals.h"
#include "blending.h"

Uint32 blend_avg(const Uint32 source, const Uint32 target)
{
	Uint32 sourcer = (source >>  0) & 0xff;
	Uint32 sourceg = (source >>  8) & 0xff;
	Uint32 sourceb = (source >> 16) & 0xff;
	Uint32 targetr = (target >>  0) & 0xff;
	Uint32 targetg = (target >>  8) & 0xff;
	Uint32 targetb = (target >> 16) & 0xff;

	targetr = (sourcer + targetr) >> 1;
	targetg = (sourceg + targetg) >> 1;
	targetb = (sourceb + targetb) >> 1;

	return (targetr <<  0) |
		(targetg <<  8) |
		(targetb << 16);
}

Uint32 blend_mul(const Uint32 source, const Uint32 target)
{
	Uint32 sourcer = (source >>  0) & 0xff;
	Uint32 sourceg = (source >>  8) & 0xff;
	Uint32 sourceb = (source >> 16) & 0xff;
	Uint32 targetr = (target >>  0) & 0xff;
	Uint32 targetg = (target >>  8) & 0xff;
	Uint32 targetb = (target >> 16) & 0xff;

	targetr = (sourcer * targetr) >> 8;
	targetg = (sourceg * targetg) >> 8;
	targetb = (sourceb * targetb) >> 8;

	return (targetr <<  0) |
		(targetg <<  8) |
		(targetb << 16);
}

Uint32 blend_add(const Uint32 source, const Uint32 target)
{
	Uint32 sourcer = (source >>  0) & 0xff;
	Uint32 sourceg = (source >>  8) & 0xff;
	Uint32 sourceb = (source >> 16) & 0xff;
	Uint32 targetr = (target >>  0) & 0xff;
	Uint32 targetg = (target >>  8) & 0xff;
	Uint32 targetb = (target >> 16) & 0xff;

	targetr += sourcer;
	targetg += sourceg;
	targetb += sourceb;

	if (targetr > 0xff) targetr = 0xff;
	if (targetg > 0xff) targetg = 0xff;
	if (targetb > 0xff) targetb = 0xff;

	return (targetr <<  0) |
		(targetg <<  8) |
		(targetb << 16);
}

Uint32 blend_alpha(const Uint32 source, const Uint32 target, const Uint32 factor)
{

	double sfact = (255.0 - (double)factor) / 255.0;
	double tfact = (double)factor / 255.0;

	Uint32 sourcer = (source >>  0) & 0xff;
	Uint32 sourceg = (source >>  8) & 0xff;
	Uint32 sourceb = (source >> 16) & 0xff;
	Uint32 targetr = (target >>  0) & 0xff;
	Uint32 targetg = (target >>  8) & 0xff;
	Uint32 targetb = (target >> 16) & 0xff;

	sourcer = (Uint32)((double)sourcer * sfact);
	sourceg = (Uint32)((double)sourceg * sfact);
	sourceb = (Uint32)((double)sourceb * sfact);

	if (sourcer > 0xff) sourcer = 0xff;
	if (sourceg > 0xff) sourceg = 0xff;
	if (sourceb > 0xff) sourceb = 0xff;

	targetr = (Uint32)((double)targetr * tfact);
	targetg = (Uint32)((double)targetg * tfact);
	targetb = (Uint32)((double)targetb * tfact);

	if (targetr > 0xff) targetr = 0xff;
	if (targetg > 0xff) targetg = 0xff;
	if (targetb > 0xff) targetb = 0xff;

	targetr = sourcer + targetr;
	targetg = sourcer + targetg;
	targetb = sourcer + targetb;

	return (targetr << 0) | (targetg << 8) | (targetb << 16);

}

void blend_darkenRect(const Sint32 x, const Sint32 y, const Sint32 width, const Sint32 height, const Uint32 factor)
{
	Sint32 i, j;
	Sint32 pitch = gScreen->pitch / 4;

    if ( SDL_MUSTLOCK(gScreen) )
    {
        if ( SDL_LockSurface(gScreen) < 0 ) {
            return;
        }
    }


	for (i = 0; i < height; i++)
	{
		// vertical clipping: (top and bottom)
		if ((y + i) >= 0 && (y + i) < SCR_HEIGHT)
		{
			Sint32 len = width;
			Sint32 xofs = x;

			// left border
			if (xofs < 0)
			{
				len += xofs;
				xofs = 0;
			}

			// right border
			if (xofs + len >= SCR_WIDTH)
			{
				len -= (xofs + len) - SCR_WIDTH;
			}
			Sint32 ofs = (i + y) * pitch + xofs;

			// note that len may be 0 at this point,
			// and no pixels get drawn!

			for (j = 0; j < len; j++) {
				//((Uint32*)gScreen->pixels)[ofs + j] = c;
				((Uint32*)gScreen->pixels)[ofs + j] = blend_mul(((Uint32*)gScreen->pixels)[ofs + j], factor);
			}
		}
	}

	if (SDL_MUSTLOCK(gScreen)) SDL_UnlockSurface(gScreen);
}

void blend_brightenRect(const Sint32 x, const Sint32 y, const Sint32 width, const Sint32 height, const Uint32 factor)
{
	Sint32 i, j;
	Sint32 pitch = gScreen->pitch / 4;

    if ( SDL_MUSTLOCK(gScreen) )
    {
        if ( SDL_LockSurface(gScreen) < 0 ) {
            return;
        }
    }

	for (i = 0; i < height; i++)
	{
		// vertical clipping: (top and bottom)
		if ((y + i) >= 0 && (y + i) < SCR_HEIGHT)
		{
			Sint32 len = width;
			Sint32 xofs = x;

			// left border
			if (xofs < 0)
			{
				len += xofs;
				xofs = 0;
			}

			// right border
			if (xofs + len >= SCR_WIDTH)
			{
				len -= (xofs + len) - SCR_WIDTH;
			}
			Sint32 ofs = (i + y) * pitch + xofs;

			// note that len may be 0 at this point,
			// and no pixels get drawn!

			for (j = 0; j < len; j++) {
				//((Uint32*)gScreen->pixels)[ofs + j] = c;
				((Uint32*)gScreen->pixels)[ofs + j] = blend_add(((Uint32*)gScreen->pixels)[ofs + j], factor);
			}
		}
	}
	if (SDL_MUSTLOCK(gScreen)) SDL_UnlockSurface(gScreen);
}
