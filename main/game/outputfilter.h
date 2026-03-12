// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#ifndef OUTPUTFILTER_H
#define OUTPUTFILTER_H

typedef enum _OUTPUTFILTER_TYPE {
	OUTPUTFILTER_NONE = 0,
	OUTPUTFILTER_GREYSCALE = 1,
	OUTPUTFILTER_COLORINVERT = 2,
	OUTPUTFILTER_NOISERECT = 4,
	OUTPUTFILTER_NOISECIRCLE = 8,
	OUTPUTFILTER_NOISESTRIPE = 16,
	OUTPUTFILTER_FADING = 32
} OUTPUTFILTER_TYPE;
	
void initOutputFilter();
void deInitOutputFilter();
void applyOutputFilter();
void setOutputFilter(const Uint32 type);
Uint32 getOutputFilter();
void setSpecificOutputFilter(const Uint32 type, bool isSet);
bool getSpecificOutputFilter(const Uint32 type);
void setOutputFilterFading(const Uint32 fading);
Uint32 getOutputFilterFading();

#endif // OUTPUTFILTER_H
