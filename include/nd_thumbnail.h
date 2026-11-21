#ifndef ND_THUMBNAIL_H
#define ND_THUMBNAIL_H
/* Fixed-size, indexed plugin thumbnails. No host file access or decoder calls. */
#define ND_THUMB_SIDE 32
struct NDThumbnail {unsigned char width,height,rgb[16][3],pixels[1024];};
static int nd_thumbnail_valid(const struct NDThumbnail *p)
{unsigned int i;if(!p)return 0;if(!p->width&&!p->height)return 1;if(p->width!=32||p->height!=32)return 0;for(i=0;i<1024;++i)if(p->pixels[i]>15)return 0;return 1;}
#endif
