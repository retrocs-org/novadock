#ifndef ND_COLOUR_JOB_H
#define ND_COLOUR_JOB_H
#include "nd_colour.h"
struct ColourJob {struct NDColourPalette palette;unsigned long rgb;char name[32];int copy_rgb;char problem[96];};
#endif
