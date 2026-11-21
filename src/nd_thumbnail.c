#include "dock_internal.h"
struct NDThumbnailCache {struct NDThumbnail picture;struct BitMap *normal,*tiny;};
void release_thumbnail(struct NDDock *d,int i)
{struct NDThumbnailCache *c=d->thumbnails[i];if(!c)return;WaitBlit();if(c->normal)FreeBitMap(c->normal);if(c->tiny)FreeBitMap(c->tiny);FreeVec(c);d->thumbnails[i]=NULL;}
struct BitMap *thumbnail_bitmap(struct NDDock *d,int i,int tiny)
{return d->thumbnails[i] ? (tiny ? d->thumbnails[i]->tiny:d->thumbnails[i]->normal):NULL;}
int receive_thumbnail(struct NDDock *d,int i,const struct NDThumbnail *p)
{struct NDThumbnailCache *c;struct RastPort rp;ULONG pens[16];unsigned int n,x,y,start,side,scale,level;struct BitMap *bitmap;if(!d||!d->screen||!d->screen->RastPort.BitMap||i<0||i>=ND_MAX_ITEMS||!nd_thumbnail_valid(p))return 0;if(!p->width){if(!d->thumbnails[i])return 0;release_thumbnail(d,i);return 1;}if(d->thumbnails[i]&&!memcmp(&d->thumbnails[i]->picture,p,sizeof(*p)))return 0;
 c=AllocVec(sizeof(*c),MEMF_PUBLIC|MEMF_CLEAR);if(!c)return 0;c->picture=*p;for(n=0;n<16;++n)pens[n]=FindColor(d->screen->ViewPort.ColorMap,(ULONG)p->rgb[n][0]*0x01010101UL,(ULONG)p->rgb[n][1]*0x01010101UL,(ULONG)p->rgb[n][2]*0x01010101UL,-1);
 for(level=0;level<2;++level){side=level ? 16:32;scale=level ? 2:1;bitmap=AllocBitMap(side,side,GetBitMapAttr(d->screen->RastPort.BitMap,BMA_DEPTH),BMF_CLEAR,d->screen->RastPort.BitMap);if(!bitmap)goto fail;if(level)c->tiny=bitmap;else c->normal=bitmap;InitRastPort(&rp);rp.BitMap=bitmap;
  for(y=0;y<side;++y)for(x=0;x<side;){unsigned int color=p->pixels[y*scale*32+x*scale];start=x;while(x<side&&p->pixels[y*scale*32+x*scale]==color)++x;SetAPen(&rp,pens[color]);RectFill(&rp,start,y,x-1,y);}}
 WaitBlit();release_thumbnail(d,i);d->thumbnails[i]=c;return 1;
 fail:WaitBlit();if(c->normal)FreeBitMap(c->normal);if(c->tiny)FreeBitMap(c->tiny);FreeVec(c);return 0;}
