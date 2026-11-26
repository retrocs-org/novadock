/* Shared native dock and Appearance sample backdrop renderer. */
#include <string.h>
#include <dos/dosextens.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include "nd_backdrop.h"
#include "nd_autohide.h"
#include "nd_theme.h"

#define ND_BACKDROP_GROWTH 128
#define RGBBYTE(n) ((ULONG)(n) * 0x01010101UL)

/* Cache short strips in the screen's format instead of a full dock bitmap.
 * All three strips share the same palette mapping, including on low-depth
 * screens where several artwork colours may have to share a pen. */
void nd_backdrop_free(struct NDBackdrop *background,struct Screen *screen)
{
    int i;
    if (background->parts[0].bitmap) WaitBlit();
    for (i=0;i<3;++i) {
        struct NDBackdropPart *part=&background->parts[i];
        if (part->bitmap) FreeBitMap(part->bitmap);
        if (part->mask) FreeBitMap(part->mask);
        if (part->growth) FreeBitMap(part->growth);
        if (part->growth_mask) FreeBitMap(part->growth_mask);
        memset(part,0,sizeof(*part));
    }
    for (i=0;i<background->color_count;++i) {
        const struct NDBackdropColor *color=&background->palette[i];
        if (color->owned) ReleasePen(screen->ViewPort.ColorMap,color->pen);
    }
    background->color_count=0;
    background->thickness=0;
}

static ULONG artwork_pen(struct NDBackdrop *background,struct ColorMap *map,
                         const UBYTE *rgb,int max_pen,ULONG matte)
{
    ULONG key=((ULONG)rgb[0]<<16)|((ULONG)rgb[1]<<8)|rgb[2];
    struct NDBackdropColor *color;
    int i;
    for (i=0;i<background->color_count;++i)
        if (background->palette[i].rgb==key) return background->palette[i].pen;

    color=&background->palette[background->color_count++];
    color->rgb=key;
    color->pen=ObtainBestPenA(map,RGBBYTE(rgb[0]),RGBBYTE(rgb[1]),RGBBYTE(rgb[2]),NULL);
    color->owned=color->pen>=0;
    if (!color->owned)
        color->pen=FindColor(map,RGBBYTE(rgb[0]),RGBBYTE(rgb[1]),RGBBYTE(rgb[2]),max_pen);
    if (color->pen<0) color->pen=matte;
    return color->pen;
}

static int load_part(struct NDBackdrop *background,int index,struct Screen *screen,
                     struct NDILBM *art,const char *directory,const char *filename,
                     ULONG matte,const struct NDRect *desktop)
{
    struct NDBackdropPart *part=&background->parts[index];
    struct BitMap *screen_bitmap=screen->RastPort.BitMap;
    int thickness=background->thickness,rotated=background->vertical;
    int sample_x=0,sample_y=0,sample=0;
    struct RastPort rp,mask_rp;
    char path[350];
    ULONG colors[ND_ICON_MAX_COLORS];
    int across,along,i;
    int max_pen=screen_bitmap->Depth>=8 ? 255 : (1<<screen_bitmap->Depth)-1;

    strcpy(path,directory);
    if (!AddPart(path,(STRPTR)filename,sizeof(path)) || !nd_ilbm_load(path,art)) return 0;
    part->bitmap=AllocBitMap(rotated ? thickness : art->width,
                             rotated ? art->width : thickness,
                             screen_bitmap->Depth,BMF_CLEAR,screen_bitmap);
    if (!part->bitmap) return 0;
    if(art->transparent>=0 && index!=0){
        part->mask=AllocBitMap(rotated ? thickness:art->width,
                              rotated ? art->width:thickness,1,BMF_CLEAR,NULL);
        if(part->mask){InitRastPort(&mask_rp);mask_rp.BitMap=part->mask;SetAPen(&mask_rp,1);}
    }
    part->length=art->width;
    for (i=0;i<art->colors;++i)
        colors[i]=i==art->transparent ? matte :
            artwork_pen(background,screen->ViewPort.ColorMap,art->palette[i],max_pen,matte);
    if (desktop && index!=0 && art->transparent>=0 &&
        art->width<=(rotated ? desktop->height : desktop->width)) {
        /* Before Intuition opens the dock, the public screen still shows
         * what's underneath it. Sample only the masked runs of the caps. */
        sample_x=desktop->left;
        sample_y=desktop->top;
        if (index==2) {
            if (rotated) sample_y+=desktop->height-art->width;
            else sample_x+=desktop->width-art->width;
        }
        sample=1;
    }
    InitRastPort(&rp);
    rp.BitMap=part->bitmap;
    for (across=0;across<thickness;++across) {
        int source_y=across*art->height/thickness;
        const UBYTE *row;
        /* Keep the first and last rows at the outer edges. When scaling a
         * shelf, its outline should stay one pixel thick on both long sides. */
        if (art->height>2 && thickness>1) {
            if (across==thickness-1) source_y=art->height-1;
            else if (across>0)
                source_y=1+(2*across-1)*(art->height-2)/(2*(thickness-2));
        }
        row=art->pixels+source_y*ND_ICON_MAX_DIM;
        for (along=0;along<art->width;) {
            int first=along;
            unsigned int color=row[along++];
            int x,y,w,h;
            while (along<art->width && row[along]==color) ++along;
            x=rotated ? across : first;
            y=rotated ? first : across;
            w=rotated ? 1 : along-first;
            h=rotated ? along-first : 1;
            if(part->mask && (int)color!=art->transparent)
                RectFill(&mask_rp,x,y,x+w-1,y+h-1);
            if (sample && (int)color==art->transparent)
                BltBitMap(screen_bitmap,sample_x+x,sample_y+y,part->bitmap,
                          x,y,w,h,0xc0,0xff,NULL);
            else {
                SetAPen(&rp,color<art->colors ? colors[color] : matte);
                RectFill(&rp,x,y,x+w-1,y+h-1);
            }
        }
    }
    part->growth=AllocBitMap(rotated ? ND_BACKDROP_GROWTH:part->length,
        rotated ? part->length:ND_BACKDROP_GROWTH,screen_bitmap->Depth,BMF_CLEAR,screen_bitmap);
    if(part->mask)part->growth_mask=AllocBitMap(rotated ? ND_BACKDROP_GROWTH:part->length,
        rotated ? part->length:ND_BACKDROP_GROWTH,1,BMF_CLEAR,NULL);
    for(across=0;across<ND_BACKDROP_GROWTH;++across){
        if(part->growth)BltBitMap(part->bitmap,rotated ? thickness/2:0,rotated ? 0:thickness/2,
            part->growth,rotated ? across:0,rotated ? 0:across,
            rotated ? 1:part->length,rotated ? part->length:1,0xc0,0xff,NULL);
        if(part->growth_mask)BltBitMap(part->mask,rotated ? thickness/2:0,rotated ? 0:thickness/2,
            part->growth_mask,rotated ? across:0,rotated ? 0:across,
            rotated ? 1:part->length,rotated ? part->length:1,0xc0,0xff,NULL);
    }
    return 1;
}

void nd_backdrop_load(struct NDBackdrop *background,struct Screen *screen,
                      const char *root,const char *theme_id,int thickness,
                      int vertical,ULONG matte,const struct NDRect *desktop)
{
    static const char *const files[]={"Backdrop.iff","BackdropLeft.iff","BackdropRight.iff"};
    struct Process *self=(struct Process *)FindTask(NULL);
    struct NDILBM *art;
    APTR requesters;
    char directory[320];
    int i;

    nd_backdrop_free(background,screen);
    if (!nd_theme_id_valid(theme_id) || strlen(root)>=sizeof(directory) ||
        thickness<1 || thickness>512) return;
    strcpy(directory,root);
    if (!AddPart(directory,(STRPTR)"Themes",sizeof(directory)) ||
        !AddPart(directory,(STRPTR)theme_id,sizeof(directory)) ||
        !AddPart(directory,(STRPTR)"Images",sizeof(directory))) return;
    if (desktop && (desktop->left<0 || desktop->top<0 ||
                    desktop->left+desktop->width>screen->Width ||
                    desktop->top+desktop->height>screen->Height ||
                    thickness!=(vertical ? desktop->width : desktop->height)))
        desktop=NULL;
    art=(struct NDILBM *)AllocVec(sizeof(*art),MEMF_ANY);
    if (!art) return;
    background->thickness=thickness;
    background->vertical=vertical;
    requesters=self->pr_WindowPtr;
    self->pr_WindowPtr=(APTR)-1L;
    for (i=0;i<3;++i) {
        if (!load_part(background,i,screen,art,directory,files[i],matte,desktop) &&
            i==0) break;
    }
    if (desktop && i==3) WaitBlit();
    self->pr_WindowPtr=requesters;
    FreeVec(art);
}

/* Use the same closed highlight path in the dock and Appearance sample.
 * Draw the fill to the same inset as the outline, including the corners. */
void nd_tile_highlight(struct RastPort *rp,int x,int y,int w,int h,int shape,
                       ULONG fill,ULONG edge)
{
    int corner=shape==ND_SHAPE_CHAMFER ? 5 : shape==ND_SHAPE_ROUNDED ? 2 : 0;
    int right=x+w-1,bottom=y+h-1,limit,i;
    if (shape==ND_SHAPE_MINIMAL) { ++x; ++y; --right; --bottom; }
    if (right<=x || bottom<=y) return;
    limit=(right-x<bottom-y ? right-x : bottom-y)/2;
    if (corner>limit) corner=limit;
    SetAPen(rp,fill);
    RectFill(rp,x,y+corner,right,bottom-corner);
    for (i=0;i<corner;++i) {
        int inset=corner-i;
        RectFill(rp,x+inset,y+i,right-inset,y+i);
        RectFill(rp,x+inset,bottom-i,right-inset,bottom-i);
    }
    SetAPen(rp,edge);
    SetDrPt(rp,0xffffU);
    Move(rp,x+corner,y); Draw(rp,right-corner,y);
    Draw(rp,right,y+corner); Draw(rp,right,bottom-corner);
    Draw(rp,right-corner,bottom); Draw(rp,x+corner,bottom);
    Draw(rp,x,bottom-corner); Draw(rp,x,y+corner);
    Draw(rp,x+corner,y);
}

/* Keep both outer halves (and their masks) unchanged. Added thickness is
 * taken from a cached centre strip, preserving the theme's colour and corners. */
int nd_backdrop_paint_sized(const struct NDBackdrop *background,struct RastPort *rp,
                      int origin_x,int origin_y,int length,int total,int x,int y,int w,int h)
{
    const struct NDBackdropPart *parts=background->parts,*part;
    int rotated=background->vertical;
    int first=rotated ? y:x,end=first+(rotated ? h:w);
    int across=rotated ? x:y,cross_end=across+(rotated ? w:h);
    int left=parts[1].length,right=parts[2].length;
    int extra=total-background->thickness,split=background->thickness/2;
    if(w<=0 || h<=0 || !parts[0].bitmap || first<0 || end>length ||
       across<0 || cross_end>total || extra<0)return 0;
    if(!left || !right || left+right>=length)left=right=0;
    while(across<cross_end){
        int source=across,band=cross_end-across,growing=0,along=first;
        if(extra && across>=split && across<split+extra){
            growing=1;source=0;
            if(band>split+extra-across)band=split+extra-across;
            if(band>ND_BACKDROP_GROWTH)band=ND_BACKDROP_GROWTH;
        }else if(extra && across>=split+extra)source-=extra;
        else if(extra && band>split-across)band=split-across;
        while(along<end){
            struct BitMap *bitmap,*mask;
            int offset,count,limit,source_cross=source,lines=band;
            if(along<left){part=&parts[1];offset=along;limit=left;}
            else if(along>=length-right){part=&parts[2];offset=along-(length-right);limit=length;}
            else{part=&parts[0];offset=(along-left)%part->length;limit=length-right;}
            count=part->length-offset;
            if(count>limit-along)count=limit-along;
            if(count>end-along)count=end-along;
            bitmap=part->bitmap;mask=part->mask;
            if(growing){
                if(part->growth && (!mask || part->growth_mask)){
                    bitmap=part->growth;mask=part->growth_mask;
                }else{source_cross=split;lines=1;}
            }
            {int row;
             for(row=0;row<band;row+=lines){
                if(mask){
                    if(rotated)BltMaskBitMapRastPort(bitmap,source_cross,offset,rp,
                        origin_x+across+row,origin_y+along,lines,count,0xe0,mask->Planes[0]);
                    else BltMaskBitMapRastPort(bitmap,offset,source_cross,rp,
                        origin_x+along,origin_y+across+row,count,lines,0xe0,mask->Planes[0]);
                }else if(rotated)BltBitMapRastPort(bitmap,source_cross,offset,rp,
                    origin_x+across+row,origin_y+along,lines,count,0xc0);
                else BltBitMapRastPort(bitmap,offset,source_cross,rp,
                    origin_x+along,origin_y+across+row,count,lines,0xc0);
             }
            }
            along+=count;
        }
        across+=band;
    }
    return 1;
}
int nd_backdrop_paint(const struct NDBackdrop *background,struct RastPort *rp,
                      int origin_x,int origin_y,int length,int x,int y,int w,int h)
{
    return nd_backdrop_paint_sized(background,rp,origin_x,origin_y,length,
                                  background->thickness,x,y,w,h);
}
