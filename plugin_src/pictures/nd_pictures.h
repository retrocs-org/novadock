#ifndef ND_PICTURES_H
#define ND_PICTURES_H
#include "nd_thumbnail.h"
#include <stdio.h>
#define ND_PICTURES_MAX 32
#define ND_PICTURE_W 128
#define ND_PICTURE_H 96
struct NDPictureImage {unsigned int source_width,source_height;unsigned char rgb[16][3],pixels[ND_PICTURE_W*ND_PICTURE_H];};
struct NDPicturesJob {char drawer[240],names[ND_PICTURES_MAX][96],problem[96];unsigned int count,index,truncated;struct NDPictureImage image;};
int nd_picture_probe(FILE *f,unsigned int *width,unsigned int *height);
int nd_picture_name(const char *name);
void nd_picture_palette(unsigned char rgb[16][3]);
unsigned int nd_picture_color(const unsigned char rgb[3],const unsigned char palette[16][3]);
void nd_picture_thumbnail(const struct NDPictureImage *image,struct NDThumbnail *thumb);
#endif
