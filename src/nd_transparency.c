/* Smooth RTG tinting is cached on open/recapture. Planar screens use a
 * fixed ordered mask. Viewport growth updates newly captured rows. */
#include <string.h>
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <graphics/rastport.h>
#include <intuition/screens.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include "nd_rtg_pixels.h"
#include "nd_transparency.h"
struct Library *NDGlassCyberGfxBase=NULL;
static unsigned char mix(unsigned char desktop,unsigned char tint,int amount)
{return (unsigned char)(((unsigned long)desktop*amount+(unsigned long)tint*(100-amount)+50)/100);}
void nd_transparency_free(struct NDTransparency *cache)
{WaitBlit();if(cache->tint)FreeBitMap(cache->tint);if(cache->mask)FreeBitMap(cache->mask);if(cache->row)FreeVec(cache->row);if(cache->reader)CloseLibrary(cache->reader);memset(cache,0,sizeof(*cache));}
void nd_transparency_prepare(struct NDTransparency *cache,struct Screen *screen,
    struct BitMap *desktop,int width,int height,int amount,const unsigned char rgb[3],
    int glass,int vertical,int origin,int thickness)
{
    static const unsigned char order[8][8]={
        {0,48,12,60,3,51,15,63},{32,16,44,28,35,19,47,31},
        {8,56,4,52,11,59,7,55},{40,24,36,20,43,27,39,23},
        {2,50,14,62,1,49,13,61},{34,18,46,30,33,17,45,29},
        {10,58,6,54,9,57,5,53},{42,26,38,22,41,25,37,21}};
    struct RastPort source,target;unsigned char *row;int x,y,c,ok=0;
    nd_transparency_free(cache);if(!desktop || amount<=0 || amount>100 || width<1 || height<1)return;
    cache->amount=amount;cache->width=width;cache->height=height;
    if(amount==100)return;
    NDGlassCyberGfxBase=OpenLibrary("cybergraphics.library",41);
    row=(unsigned char *)AllocVec((ULONG)width*3UL,MEMF_PUBLIC);
    if(NDGlassCyberGfxBase && row && nd_glass_bitmap(desktop) && nd_glass_depth(desktop)>8){
        cache->tint=AllocBitMap(width,height,screen->RastPort.BitMap->Depth,BMF_CLEAR,screen->RastPort.BitMap);
        if(cache->tint){
            InitRastPort(&source);source.BitMap=desktop;InitRastPort(&target);target.BitMap=cache->tint;ok=1;
            for(y=0;y<height && ok;++y){
                if(nd_glass_rgb_line(row,width,&source,y)!=(unsigned long)width){ok=0;break;}
                for(x=0;x<width;++x){
                    int band=vertical ? x-origin:y-origin,light=0;
                    if(glass && thickness>0){static const int levels[8]={68,52,34,18,8,16,26,46};band=band*8/thickness;if(band<0)band=0;if(band>7)band=7;light=levels[band];}
                    for(c=0;c<3;++c){unsigned char tint=(unsigned char)((rgb[c]*(100-light)+255*light+50)/100);row[x*3+c]=mix(row[x*3+c],tint,amount);}
                }
                if(nd_glass_write_line(row,width,&target,y)!=(unsigned long)width)ok=0;
            }
        }
    }
    if(ok){
        cache->row=row;cache->reader=NDGlassCyberGfxBase;NDGlassCyberGfxBase=NULL;
        memcpy(cache->rgb,rgb,3);cache->glass=glass;cache->vertical=vertical;cache->origin=origin;cache->thickness=thickness;return;
    }
    if(row)FreeVec(row);if(NDGlassCyberGfxBase)CloseLibrary(NDGlassCyberGfxBase);NDGlassCyberGfxBase=NULL;
    if(cache->tint){WaitBlit();FreeBitMap(cache->tint);cache->tint=NULL;}
    cache->mask=AllocBitMap(width,height,1,BMF_CLEAR,NULL);
    if(cache->mask)for(y=0;y<height;++y){unsigned char bits=0;for(x=0;x<8;++x)if((order[y&7][x]*100+32)/64<amount)bits|=(unsigned char)(128>>x);memset((unsigned char*)cache->mask->Planes[0]+(ULONG)y*cache->mask->BytesPerRow,bits,cache->mask->BytesPerRow);}
}
int nd_transparency_refresh(struct NDTransparency *cache,struct BitMap *desktop,int left,int top,int width,int height)
{
    struct RastPort source,target;int x,y,c,right=left+width,bottom=top+height,ok=1;unsigned char *row=cache->row;
    if(!cache->tint)return 1;
    if(!row||!cache->reader||!desktop)return 0;
    if(left<0)left=0;if(top<0)top=0;if(right>cache->width)right=cache->width;if(bottom>cache->height)bottom=cache->height;
    width=right-left;if(width<=0||bottom<=top)return 1;
    InitRastPort(&source);source.BitMap=desktop;InitRastPort(&target);target.BitMap=cache->tint;
    NDGlassCyberGfxBase=cache->reader;
    for(y=top;y<bottom&&ok;++y){
        int band=cache->vertical?0:y-cache->origin;
        if(nd_glass_rgb_span(row,width,&source,left,y)!=(unsigned long)width){ok=0;break;}
        for(x=0;x<width;++x){
            int light=0;if(cache->vertical)band=left+x-cache->origin;
            if(cache->glass&&cache->thickness>0){static const int levels[8]={68,52,34,18,8,16,26,46};int b=band*8/cache->thickness;if(b<0)b=0;if(b>7)b=7;light=levels[b];}
            for(c=0;c<3;++c){unsigned char tint=(unsigned char)((cache->rgb[c]*(100-light)+255*light+50)/100);row[x*3+c]=mix(row[x*3+c],tint,cache->amount);}
        }
        if(nd_glass_write_span(row,width,&target,left,y)!=(unsigned long)width)ok=0;
    }
    NDGlassCyberGfxBase=NULL;return ok;
}
void nd_transparency_paint(const struct NDTransparency *cache,struct RastPort *rp,
    struct BitMap *desktop,int x,int y,int width,int height)
{
    if(cache->tint)BltBitMapRastPort(cache->tint,x,y,rp,x,y,width,height,0xc0);
    else if(cache->amount==100)BltBitMapRastPort(desktop,x,y,rp,x,y,width,height,0xc0);
    else if(cache->mask)BltMaskBitMapRastPort(desktop,x,y,rp,x,y,width,height,0xe0,cache->mask->Planes[0]);
}
