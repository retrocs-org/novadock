#include "nd_trace.h"
#include "nd_wallpaper.h"
#include "nd_hover_ease.h"
#include "nd_metric_art.h"
#include "nd_planar_scale.h"
#include "nd_icon_colors.h"
/* NovaDock: Drawing: colours, fonts, tile geometry and every tile, badge and graph. */
#include "dock_internal.h"
#include "nd_render_policy.h"
#include "nd_volume.h"
#include "nd_whdload_status.h"
#include "nd_indicator.h"
#include "nd_artwork.h"
#include <graphics/scale.h>

/* Defined below, private to this file. */
static void color_pair(struct NDDock *dock, int index, int r, int g, int b);
void remap_brush_colors(struct NDDock *dock);
static void unmap_brush_colors(struct NDDock *dock);
static void fill(struct NDDock *dock, int color, int x, int y, int w, int h);
static void paint_background(struct NDDock *dock,int x,int y,int w,int h);
static void rounded_box(struct NDDock *dock, int color, int x, int y, int w, int h);
static void tile_bevel(struct NDDock *dock,int x,int y,int w,int h);
static void tile_box(struct NDDock *dock,int color,int x,int y,int w,int h);
static void centered_text(struct NDDock *dock, int color, const char *value, int x, int baseline, int w);
static void centered_text_tight(struct NDDock *dock,int color, const char *value,int x,int baseline,int w);
static int tile_label_visible(struct NDDock *dock,int index);
static int launcher_task_name(const struct NDItem *item,char *name,int size);
static struct Task *find_cli_command(const char *wanted);
static void draw_running_indicator(struct NDDock *dock,int index, int x,int y,int w);
static int row_offset_pixels(struct NDDock *dock,int row);
static int item_axis_size(struct NDDock *dock,int index);
static int item_slot_span(struct NDDock *dock,int index);
static void draw_graph(struct NDDock *dock, int index, int x, int y, int tw, int th);
static int tile_interior(struct NDDock *dock,int index);
static int draw_brush(struct NDDock *dock, int index, int x, int y, int tw, int th);
static void draw_analog_clock(struct NDDock *dock,int x,int y,int w,int h);
static void eyes_filled_ellipse(struct NDDock *dock, int cx,int cy,int rx,int ry,int color);
static void eyes_geometry(struct NDDock *dock,int index, int centers_x[2],int centers_y[2], int *rx,int *ry,int *pr);
static void draw_eyes(struct NDDock *dock,int index,int x,int y,int tw,int th);
static void draw_volume_meter(struct NDDock *dock, const struct NDItem *item, int x, int y, int tw, int th);
static int parse_ram_pair(const char *value,unsigned int *chip, unsigned int *fast);
static void draw_ram_tile(struct NDDock *dock,const struct NDItem *item, int x,int y,int tw,int th);
static void draw_ram_graph_tile(struct NDDock *dock,int index, int x,int y,int tw,int th);
static void music_button_box(struct NDDock *dock,int x,int y,int w,int h, int enabled,int focus,int pressed);
static void music_transport_icon(struct NDDock *dock,int kind, int x,int y,int w,int h,int enabled,int playing,int focus,int pressed);
static void draw_music_tile(struct NDDock *dock,const struct NDItem *item, int x,int y,int tw,int th);
static unsigned int music_control_hit(struct NDDock *dock,int index, int mx,int my,unsigned int *volume);
static int draw_plugin_badge(struct NDDock *dock,const struct NDItem *item,
                             int x,int y,int tw);
static void draw_tiny_plugin(struct NDDock *dock,const struct NDItem *item, int x,int y,int tw,int th);
static int dopus_application_running(const struct NDItem *item);
static void draw_plugin_launcher_artwork(struct NDDock *dock,int index, const char *label, int x,int y,int tw,int th);
static void draw_dopus_tile(struct NDDock *dock,int index, const struct NDItem *item, int x,int y,int tw,int th);
static void draw_music_idle_tile(struct NDDock *dock,int index, int x,int y,int tw,int th);
static void draw_running_apps_tile(struct NDDock *dock,int index, int x,int y,int tw,int th);
static int draw_plugin_badge(struct NDDock *dock,const struct NDItem *item, int x,int y,int tw);
static void draw_plugin_running_indicator(struct NDDock *dock, const struct NDItem *item, int x,int y,int tw);
static void draw_scroll_control(struct NDDock *dock, int direction);
static void draw_insertion_marker(struct NDDock *dock);
static void draw_tab(struct NDDock *dock);
static void prepare_hover_layout(struct NDDock *dock);
static int refresh_shelf_tiles(struct NDDock *dock);
static int metric_kind(struct NDDock *dock,const struct NDItem *item);
static int hover_art_size(struct NDDock *dock,int index,int tw,int th,int *w,int *h);
static int wave_icon_size(struct NDDock *dock,int index,int tw,int th,int *w,int *h);

static void color_pair(struct NDDock *dock, int index, int r, int g, int b)
{
    int max_pen;
    if (!dock->screen) { dock->colors[index] = -1; return; }
    max_pen = dock->screen->RastPort.BitMap->Depth >= 8 ? 255 :
              (1 << dock->screen->RastPort.BitMap->Depth) - 1;
    /* Theme colours need real palette entries.  FindColor alone collapses
     * several dark shades to an unrelated Workbench pen on an 8-bit public
     * screen, producing the bright-blue frames seen after the DOpus memory
     * investigation.  Eight shared pens are a small, bounded reservation;
     * release them when the window is rebuilt or NovaDock exits. */
    dock->colors[index] = ObtainBestPenA(dock->screen->ViewPort.ColorMap,
                                        RGBBYTE(r),RGBBYTE(g),RGBBYTE(b),NULL);
    /* Some public screens expose a larger ColorMap than their planar depth.
     * Pens outside that bitmap cannot be displayed by SetAPen. */
    if (dock->colors[index] > max_pen) {
        ReleasePen(dock->screen->ViewPort.ColorMap,dock->colors[index]);
        dock->colors[index] = -1;
    }
    dock->color_owned[index] = dock->colors[index] >= 0 ? 1 : 0;
    if (dock->colors[index] < 0)
        dock->colors[index] = FindColor(dock->screen->ViewPort.ColorMap,
                                        RGBBYTE(r),RGBBYTE(g),RGBBYTE(b),max_pen);
}
void remap_item_brush_colors(struct NDDock *dock,int i)
{
    int j, max_pen;
    if (!dock->screen || i<0 || i>=ND_MAX_ITEMS) return;
    max_pen = dock->screen->RastPort.BitMap->Depth >= 8 ? 255 :
              (1 << dock->screen->RastPort.BitMap->Depth) - 1;
    for (j = 0; j < ND_ICON_MAX_COLORS; ++j) {
        LONG mapped = -1;
        const struct NDILBM *b = dock->brushes[i];
        if (b && j < b->colors && j != b->transparent)
            mapped = FindColor(dock->screen->ViewPort.ColorMap,
                RGBBYTE(b->palette[j][0]), RGBBYTE(b->palette[j][1]),
                RGBBYTE(b->palette[j][2]), max_pen);
        dock->brush_pens[i][j] = mapped;
    }
}
void remap_brush_colors(struct NDDock *dock)
{
    int i;
    for (i = 0; i < ND_MAX_ITEMS; ++i) remap_item_brush_colors(dock,i);
}
static void unmap_brush_colors(struct NDDock *dock)
{
    int i, j;
    if (!dock->screen) return;
    for (i = 0; i < ND_MAX_ITEMS; ++i)
        for (j = 0; j < ND_ICON_MAX_COLORS; ++j)
            dock->brush_pens[i][j] = -1;
}
/* Average colour of the desktop where the dock is about to open (before
 * its window exists), sampled on a 24 x 6 grid.  Palette-based screens only
 * (classic Workbench, up to 8 bit): 0 when it cannot be sampled. */
static int sample_wallpaper(struct NDDock *dock,unsigned char average[3])
{
    struct RastPort *rp;
    struct ColorMap *map;
    const struct NDRect *box=&dock->full_box;
    ULONG rgb[3],sum[3];
    int ix,iy,n=0;
    if (!dock->screen || dock->window || box->width<1 || box->height<1) return 0;
    rp=&dock->screen->RastPort;map=dock->screen->ViewPort.ColorMap;
    if (!rp->BitMap || rp->BitMap->Depth>8 || !map) return 0;
    sum[0]=sum[1]=sum[2]=0;
    for (iy=0;iy<6;++iy)
        for (ix=0;ix<24;++ix) {
            LONG pen;
            int x=box->left+(2*ix+1)*box->width/48,y=box->top+(2*iy+1)*box->height/12;
            if (x<0 || y<0 || x>=dock->screen->Width || y>=dock->screen->Height) continue;
            pen=ReadPixel(rp,x,y);
            if (pen<0) continue;
            GetRGB32(map,(ULONG)pen,1,rgb);
            sum[0]+=rgb[0]>>24;sum[1]+=rgb[1]>>24;sum[2]+=rgb[2]>>24;++n;
        }
    if (!n) return 0;
    average[0]=(unsigned char)(sum[0]/n);average[1]=(unsigned char)(sum[1]/n);
    average[2]=(unsigned char)(sum[2]/n);
    return 1;
}
void alloc_colors(struct NDDock *dock)
{
    unsigned char icons[5][3],average[3],matched[ND_THEME_COLOR_COUNT][3];int i,k;
    const unsigned char (*rgb)[3]=(const unsigned char (*)[3])dock->theme_spec.rgb;
    for (i = 0; i < ND_COLOR_COUNT; ++i) {
        dock->colors[i] = -1;
        dock->color_owned[i] = 0;
    }
    /* Wallpaper background: shelf, tile, hover and text colours follow the
     * desktop behind the dock; the theme's colours are the fallback. */
    if (dock->conf.wallpaper_match && !dock->conf.theme_artwork && sample_wallpaper(dock,average)) {
        nd_wallpaper_palette(average,rgb,matched);
        rgb=(const unsigned char (*)[3])matched;
    }
    for (i=0;i<ND_THEME_COLOR_COUNT;++i)
        color_pair(dock,i,rgb[i][0],rgb[i][1],rgb[i][2]);
    nd_icon_palette(dock->theme_spec.rgb,dock->conf.colourful_icons,icons);
    for(i=0;i<ND_ICON_INKS;++i)color_pair(dock,ND_ICON_DARK+i,icons[i][0],icons[i][1],icons[i][2]);
    for(k=1;k<=4;++k){
        nd_metric_palette(dock->theme_spec.rgb,dock->conf.colourful_icons,k,icons);
        for(i=0;i<5;++i)color_pair(dock,ND_METRIC_COLORS+(k-1)*5+i,icons[i][0],icons[i][1],icons[i][2]);
    }
    map_plugin_artwork(dock);
    remap_brush_colors(dock);
}
/* The desktop snapshot must always match the dock it backs: blits into it
 * use the current dock size, and BltBitMap never clips. */
int ensure_desktop_snapshot(struct NDDock *dock,struct BitMap *friend_bitmap)
{
    if(!dock->screen)return 0;
    if(dock->hover_desktop && !nd_snapshot_needs_alloc(1,dock->hover_desktop_width,
       dock->hover_desktop_height,dock->width,dock->height))return 1;
    release_desktop_snapshot(dock);
    dock->hover_desktop=AllocBitMap(dock->width,dock->height,
        dock->screen->RastPort.BitMap->Depth,BMF_CLEAR,friend_bitmap);
    if(!dock->hover_desktop)return 0;
    dock->hover_desktop_width=dock->width;dock->hover_desktop_height=dock->height;
    return 1;
}
void release_desktop_snapshot(struct NDDock *dock)
{
    if(dock->hover_desktop){WaitBlit();FreeBitMap(dock->hover_desktop);dock->hover_desktop=NULL;}
    dock->hover_desktop_width=dock->hover_desktop_height=0;
}
void free_colors(struct NDDock *dock)
{
    int i;
    if (!dock->screen) return;
    release_hover_graphics(dock);
    unmap_plugin_artwork(dock);
    nd_transparency_free(&dock->transparency);
    nd_backdrop_free(&dock->background,dock->screen);
    release_desktop_snapshot(dock);
    unmap_brush_colors(dock);
    for (i = 0; i < ND_COLOR_COUNT; ++i) {
        if (dock->color_owned[i] && dock->colors[i] >= 0)
            ReleasePen(dock->screen->ViewPort.ColorMap,dock->colors[i]);
        dock->colors[i] = -1;
        dock->color_owned[i] = 0;
    }
}
void release_dock_font(struct NDDock *dock)
{
    if (dock->dock_font_owned && dock->dock_font) CloseFont(dock->dock_font);
    dock->dock_font=NULL;
    /* A new font may be opened at the same address: forget measured widths. */
    memset(dock->hover_label_width,0,sizeof(dock->hover_label_width));
    dock->dock_font_owned=0;
}
void apply_dock_font(struct NDDock *dock)
{
    static struct TextAttr topaz_attr={"topaz.font",8,FS_NORMAL,FPF_ROMFONT};
    struct TextFont *font=NULL;
    if (!dock || !dock->window || !dock->screen) return;
    release_dock_font(dock);
    if (dock->conf.font_mode==ND_FONT_TOPAZ) {
        font=OpenFont(&topaz_attr);
        if (font) dock->dock_font_owned=1;
    }
    if (!font) font=dock->screen->RastPort.Font;
    dock->dock_font=font;
    SetFont(dock->window->RPort,font);
    SetSoftStyle(dock->window->RPort,
                 dock->conf.font_mode==ND_FONT_BOLD ? FSF_BOLD : FS_NORMAL,
                 0xff);
}
ULONG pen(struct NDDock *dock, int color)
{
    if(dock->painting_mask)return color==ND_BG ? 0UL:1UL;
    if (dock->colors[color] >= 0) return (ULONG)dock->colors[color];
    return color == ND_BG || color == ND_BAR_BG ? 0UL : 1UL;
}
static void fill(struct NDDock *dock, int color, int x, int y, int w, int h)
{
    struct RastPort *rp = dock->window->RPort;
    if (w < 1 || h < 1) return;
    SetAPen(rp, pen(dock, color));
    RectFill(rp, x, y, x + w - 1, y + h - 1);
}
void prepare_transparency(struct NDDock *dock)
{
    int v=vertical(dock);
    nd_transparency_prepare(&dock->transparency,dock->screen,dock->hover_desktop,
        dock->width,dock->height,dock->conf.background_transparency,dock->theme_spec.rgb[dock->conf.theme_artwork ? ND_TILE:ND_BG],
        dock->theme_spec.dock_style,v,v ? dock->hover_pad_x:dock->hover_pad_y,
        (v ? dock->width:dock->height)-dock->hover_headroom);
}
void prepare_dock_background(struct NDDock *dock)
{
    int rotated=vertical(dock);
    int thickness=(rotated ? dock->width:dock->height)-dock->hover_headroom;
    struct NDRect shelf=dock->full_box;
    if (!dock->screen) return;
    shelf.left+=dock->hover_pad_x;shelf.top+=dock->hover_pad_y;
    if(rotated){shelf.width-=dock->hover_headroom;shelf.height-=2*dock->hover_axis_padding;}
    else{shelf.height-=dock->hover_headroom;shelf.width-=2*dock->hover_axis_padding;}
    if((dock->hover_headroom || dock->conf.theme_artwork || dock->conf.background_transparency) && !dock->window && !dock->hover_desktop){
        if(ensure_desktop_snapshot(dock,dock->screen->RastPort.BitMap)){
            BltBitMap(dock->screen->RastPort.BitMap,dock->full_box.left,dock->full_box.top,
                dock->hover_desktop,0,0,dock->width,dock->height,0xc0,0xff,NULL);
            WaitBlit();
        }
    }

    prepare_transparency(dock);
    if (!dock->conf.theme_artwork || thickness<1 || thickness>512) {
        if (dock->background.thickness) nd_backdrop_free(&dock->background,dock->screen);
        return;
    }
    if (dock->background.thickness==thickness && dock->background.vertical==rotated)
        return;
    nd_backdrop_load(&dock->background,dock->screen,dock->directory,
                     nd_config_theme_id(&dock->conf),thickness,rotated,pen(dock,ND_BG),
                     dock->window ? NULL : &shelf);
}

/* Paint a clipped continuous solid shelf without overwriting its corner pixels. */
static void paint_shelf_span(struct NDDock *dock,int x,int y,int w,int h)
{
    fill(dock,ND_BG,x,y,w,h);
    if(dock->conf.background_transparency && dock->hover_desktop)
        nd_transparency_paint(&dock->transparency,dock->window->RPort,dock->hover_desktop,x,y,w,h);
}
static void solid_shelf(struct NDDock *dock,int sx,int sy,int sw,int sh,
                        int x,int y,int w,int h)
{
    int row,left,right,inset,edge,rows,radius=dock->conf.shape==ND_SHAPE_SQUARE ? 0:8;
    if(radius>sh/2)radius=sh/2;if(radius>sw/2)radius=sw/2;
    for(row=y;row<y+h;++row){
        edge=row-sy;if(sy+sh-1-row<edge)edge=sy+sh-1-row;
        inset=0;
        if(edge<radius){
            if(dock->conf.shape==ND_SHAPE_CHAMFER)inset=radius-edge;
            else if(edge==0)inset=radius-2;
            else if(edge==1)inset=radius-4;
            else if(edge<4)inset=2;
            else inset=1;
        }
        if(dock->theme_spec.dock_style && !vertical(dock) && sh>1){
            inset=20*(sy+sh-1-row)/(sh-1);
            rows=1;
        }
        if(inset<0)inset=0;
        left=sx+inset;right=sx+sw-inset;
        if(left<x)left=x;if(right>x+w)right=x+w;
        rows=1;
        if(edge>=radius && !dock->theme_spec.dock_style){rows=sy+sh-radius-row;if(rows>y+h-row)rows=y+h-row;if(rows<1)rows=1;}
        if(right>left)paint_shelf_span(dock,left,row,right-left,rows);
        row+=rows-1;
    }
}
static void paint_background(struct NDDock *dock,int x,int y,int w,int h)
{
    int sx=0,sy=0,sw=dock->width,sh=dock->height,length;
    int right=x+w,bottom=y+h;
    if(dock->painting_mask){fill(dock,ND_BG,x,y,w,h);return;}
    if(dock->hover_desktop && !dock->hover_headroom && !dock->hidden)
        BltBitMapRastPort(dock->hover_desktop,x,y,dock->window->RPort,x,y,w,h,0xc0);
    if(dock->hover_headroom && !dock->hidden){
        if(dock->hover_desktop)
            BltBitMapRastPort(dock->hover_desktop,x,y,dock->window->RPort,x,y,w,h,0xc0);
        else fill(dock,ND_BG,x,y,w,h);
        if(dock->painting_shelf || !dock->hover_plan_ready){
            sx=dock->hover_pad_x;sy=dock->hover_pad_y;
            sw-=vertical(dock) ? dock->hover_headroom:2*dock->hover_axis_padding;
            sh-=vertical(dock) ? 2*dock->hover_axis_padding:dock->hover_headroom;
        }else{
            sx=dock->hover_shelf_rect.left;sy=dock->hover_shelf_rect.top;
            sw=dock->hover_shelf_rect.width;sh=dock->hover_shelf_rect.height;
        }
        if(x<sx)x=sx;if(y<sy)y=sy;
        if(right>sx+sw)right=sx+sw;if(bottom>sy+sh)bottom=sy+sh;
        w=right-x;h=bottom-y;if(w<=0 || h<=0)return;
    }
    if(dock->conf.background_transparency && dock->hover_desktop && !dock->hidden){
        if(dock->conf.background_transparency==100)return;
        if(dock->transparency.tint){solid_shelf(dock,sx,sy,sw,sh,x,y,w,h);return;}
    }
    length=vertical(dock) ? sh:sw;
    if(dock->hidden || !nd_backdrop_paint_sized(&dock->background,dock->window->RPort,
        sx,sy,length,vertical(dock) ? sw:sh,x-sx,y-sy,w,h)){
        if(dock->hover_headroom && dock->hover_desktop && !dock->hidden)
            solid_shelf(dock,sx,sy,sw,sh,x,y,w,h);
        else fill(dock,ND_BG,x,y,w,h);
    }
    if(dock->conf.background_transparency && dock->hover_desktop && !dock->hidden)
        nd_transparency_paint(&dock->transparency,dock->window->RPort,dock->hover_desktop,x,y,w,h);
}
static void rounded_box(struct NDDock *dock, int color,
                        int x, int y, int w, int h)
{
    if (w < 6 || h < 6) return;
    fill(dock, color, x + 2, y, w - 4, h);
    fill(dock, color, x, y + 2, w, h - 4);
}
static void tile_bevel(struct NDDock *dock,int x,int y,int w,int h)
{
    struct RastPort *rp=dock->window->RPort;
    if (w<8 || h<8 || dock->conf.shape==ND_SHAPE_MINIMAL) return;
    SetAPen(rp,pen(dock,ND_HOVER));
    if (dock->conf.shape==ND_SHAPE_CHAMFER) {
        Move(rp,x+5,y);Draw(rp,x+w-6,y);
        Move(rp,x+5,y);Draw(rp,x,y+5);Draw(rp,x,y+h-6);
        SetAPen(rp,pen(dock,ND_BAR_BG));
        Move(rp,x+w-6,y);Draw(rp,x+w-1,y+5);
        Draw(rp,x+w-1,y+h-6);Draw(rp,x+w-6,y+h-1);
        Draw(rp,x+5,y+h-1);Draw(rp,x,y+h-6);
    } else if (dock->conf.shape==ND_SHAPE_ROUNDED) {
        Move(rp,x+2,y);Draw(rp,x+w-3,y);
        Move(rp,x+1,y+1);Draw(rp,x,y+2);Draw(rp,x,y+h-3);
        SetAPen(rp,pen(dock,ND_BAR_BG));
        Move(rp,x+w-2,y+1);Draw(rp,x+w-1,y+2);
        Draw(rp,x+w-1,y+h-3);Draw(rp,x+w-2,y+h-2);
        Draw(rp,x+w-3,y+h-1);Draw(rp,x+2,y+h-1);
    } else {
        Move(rp,x,y+h-2);Draw(rp,x,y);Draw(rp,x+w-2,y);
        SetAPen(rp,pen(dock,ND_BAR_BG));
        Move(rp,x+w-1,y);Draw(rp,x+w-1,y+h-1);Draw(rp,x,y+h-1);
    }
}
static void tile_box(struct NDDock *dock,int color,int x,int y,int w,int h)
{
    paint_background(dock,x,y,w,h);
    if(dock->hover_headroom)return;
    if (color==ND_HOVER) {
        nd_tile_highlight(dock->window->RPort,x,y,w,h,dock->conf.shape,
                          pen(dock,ND_HOVER),pen(dock,ND_ACCENT));
        return;
    }
    if (dock->background.parts[0].bitmap) return;
    if (dock->conf.shape==ND_SHAPE_SQUARE) {
        fill(dock,color,x,y,w,h);
    } else if (dock->conf.shape==ND_SHAPE_CHAMFER) {
        fill(dock,color,x+5,y,w-10,h);
        fill(dock,color,x+2,y+3,w-4,h-6);
        fill(dock,color,x,y+6,w,h-12);
    } else if (dock->conf.shape==ND_SHAPE_MINIMAL) {
        /* Minimal means content directly on the dock backdrop.  The former
         * five-pixel baseline looked like an unexplained progress bar and
         * collided visually with real disk/plugin meters. */
    } else rounded_box(dock,color,x,y,w,h);
    tile_bevel(dock,x,y,w,h);
}
static void centered_text(struct NDDock *dock, int color,
                          const char *value, int x, int baseline, int w)
{
    struct RastPort *rp = dock->window->RPort;
    char printable[ND_TEXT_LEN];
    WORD extent;
    int len;
    if (w <= 4) return;
    strncpy(printable, value, sizeof(printable) - 1);
    printable[sizeof(printable) - 1] = '\0';
    len = (int)strlen(printable);
    if(dock->painting_shelf && !dock->painting_mask && dock->hover_plan_ready &&
       dock->hover_layout[dock->painting_item].kind==ND_HOVER_ART &&
       !metric_kind(dock,&dock->conf.items[dock->painting_item]))return;
    SetAPen(rp, pen(dock, color));
    SetBPen(rp, pen(dock, ND_TILE));
    SetDrMd(rp, JAM1);
    while (len && TextLength(rp, printable, len) > w - 6) --len;
    extent = TextLength(rp, printable, len);
    Move(rp, x + (w - extent) / 2, baseline);
    Text(rp, printable, len);
}
static void centered_text_tight(struct NDDock *dock,int color,
                                const char *value,int x,int baseline,int w)
{
    struct RastPort *rp=dock->window->RPort;
    int len=(int)strlen(value),extent;
    if(dock->painting_shelf && !dock->painting_mask && dock->hover_plan_ready &&
       dock->hover_layout[dock->painting_item].kind==ND_HOVER_ART &&
       !metric_kind(dock,&dock->conf.items[dock->painting_item]))return;
    SetAPen(rp,pen(dock,color));
    SetDrMd(rp,JAM1);
    while (len && TextLength(rp,(STRPTR)value,len)>w) --len;
    extent=TextLength(rp,(STRPTR)value,len);
    Move(rp,x+(w-extent)/2,baseline);
    Text(rp,(STRPTR)value,len);
}
static void label_text(struct NDDock *dock,int color,const char *name,int x,int baseline,int w)
{
    char text[ND_LABEL_LEN+4];int n,limit=w-2;struct RastPort *rp=dock->window->RPort;
    if(limit<1)return;
    if(TextLength(rp,(STRPTR)name,strlen(name))>limit){
        if(!strcmp(name,"PREFERENCES"))name="PREFS";
        else if(!strcmp(name,"MUSICPLAYER"))name="MUSIC";
        else if(!strcmp(name,"Preferences"))name="Prefs";
        else if(!strcmp(name,"MusicPlayer"))name="Music";
    }
    strncpy(text,name,sizeof(text)-1);text[sizeof(text)-1]=0;n=strlen(text);
    if(TextLength(rp,text,n)>limit){
        while(n>0 && TextLength(rp,text,n)+TextLength(rp,"...",3)>limit)--n;
        while(n>0 && text[n-1]==' ')--n;
        if(TextLength(rp,"...",3)>limit)return;
        strcpy(text+n,"...");
    }
    centered_text_tight(dock,color,text,x,baseline,w);
}
int vertical(const struct NDDock *dock)
{
    return dock->conf.side == ND_SIDE_LEFT || dock->conf.side == ND_SIDE_RIGHT;
}
int item_visible(const struct NDDock *dock, int index)
{
    if (index < 0 || index >= dock->conf.count) return 0;
    if (!dock->overflow) return 1;
    return index >= dock->view_start &&
           index < dock->view_start + dock->visible_count;
}
static int tile_label_visible(struct NDDock *dock,int index)
{
    return index>=0 && index<dock->conf.count &&
        nd_label_visible(dock->conf.label_mode,
            dock->conf.item_size==ND_ITEM_SIZE_TINY,index==dock->hover);
}

static int floating_caption(struct NDDock *dock,int index)
{
    return dock->conf.hover_magnification && index==dock->hover &&
        dock->conf.label_mode==ND_LABEL_HOVER &&
        tile_label_visible(dock,index);
}
/* Captions are separate from values, graph legends, controls and badges.
 * Widget source frames retain captions; their scaled masks select visibility.
 * Unscaled source frames omit captions and repaint them after moving. */
static void caption_text(struct NDDock *dock,int color,const char *text,
                         int x,int baseline,int w,int tight)
{
    int index=dock->painting_item,source=0;
    if(index<0 || index>=dock->conf.count)return;
    {int tx,ty,tw,th;tile_coords(dock,index,&tx,&ty);tile_dimensions(dock,index,&tw,&th);
     x=tx;w=tw;baseline=ty+th-9;}
    if(dock->painting_shelf && metric_kind(dock,&dock->conf.items[index]))return;
    color=ND_TEXT;
    if(dock->conf.items[index].kind==ND_ITEM_PLUGIN && dock->conf.items[index].label[0])text=dock->conf.items[index].label;
    if(dock->painting_shelf && dock->hover_plan_ready &&
       dock->hover_layout[index].kind!=ND_HOVER_ART){
        source=1;
        if(!dock->painting_mask){
            struct NDCaption *caption=&dock->hover_captions[index];
            caption->x=x;caption->baseline=baseline;caption->width=w;
            caption->color=color;caption->tight=tight;
            strncpy(caption->text,text,sizeof(caption->text)-1);
            caption->text[sizeof(caption->text)-1]=0;
        }
        if(dock->hover_layout[index].kind==ND_HOVER_FIXED)return;
    }
    if(!source && (!tile_label_visible(dock,index) || floating_caption(dock,index)))return;
    label_text(dock,color,text,x,baseline,w);
}
static void caption_text_tight(struct NDDock *dock,int color,const char *text,
                               int x,int baseline,int w)
{
    caption_text(dock,color,text,x,baseline,w,1);
}
static void caption_text_normal(struct NDDock *dock,int color,const char *text,
                                int x,int baseline,int w)
{
    caption_text(dock,color,text,x,baseline,w,0);
}
static void draw_fixed_caption(struct NDDock *dock,int index,int x,int y,
                               int original_x,int original_y)
{
    const struct NDCaption *caption=&dock->hover_captions[index];
    int left=caption->x+x-original_x,baseline=caption->baseline+y-original_y;
    if(!caption->width || !tile_label_visible(dock,index) || floating_caption(dock,index))return;
    label_text(dock,caption->color,caption->text,left,baseline,caption->width);
}
static int launcher_task_name(const struct NDItem *item,char *name,int size)
{
    const char *p,*last,*end;
    int n;
    if (!item || item->kind!=ND_ITEM_LAUNCH || !item->command[0] || size<2 ||
        item->launch_mode==ND_LAUNCH_AREXX || item->command[0]=='@') return 0;
    p=item->command;
    while (*p==' ' || *p=='\t' || *p=='\"') ++p;
    end=p;
    while (*end && *end!=' ' && *end!='\t' && *end!='\"') ++end;
    last=p;
    while (p<end) {
        if (*p=='/' || *p==':') last=p+1;
        ++p;
    }
    n=(int)(end-last);
    if (n<=0) return 0;
    if (n>=size) n=size-1;
    memcpy(name,last,(size_t)n);name[n]='\0';
    return 1;
}
/* Programs started from a shell (which includes everything NovaDock itself
 * launches) run inside a CLI process whose Exec task name is the shell's,
 * not the program's.  The loaded command is in cli_CommandName. */
static struct Task *find_cli_command(const char *wanted)
{
    struct List *lists[2];
    struct Node *node;
    struct Task *found=NULL;
    char name[64];
    int list,length;
    if (!wanted || !*wanted) return NULL;
    lists[0]=&SysBase->TaskReady;
    lists[1]=&SysBase->TaskWait;
    /* Interrupts move tasks between these lists, so walk them disabled and
     * keep the critical section short (no library calls). */
    Disable();
    for (list=0;list<2 && !found;++list)
        for (node=lists[list]->lh_Head;node->ln_Succ;node=node->ln_Succ) {
            struct Process *process=(struct Process *)node;
            struct CommandLineInterface *cli;
            UBYTE *command;
            if (node->ln_Type!=NT_PROCESS || !process->pr_CLI) continue;
            cli=(struct CommandLineInterface *)BADDR(process->pr_CLI);
            if (!cli || !cli->cli_Module || !cli->cli_CommandName) continue;
            command=(UBYTE *)BADDR(cli->cli_CommandName);
            if (!command || !command[0]) continue;
            length=command[0];
            if (length>(int)sizeof(name)-1) length=(int)sizeof(name)-1;
            memcpy(name,command+1,(size_t)length);name[length]='\0';
            if (nd_command_name_matches(name,wanted)) {
                found=(struct Task *)process;break;
            }
        }
    Enable();
    return found;
}
static struct Window *dock_window_for_snapshot;   /* skipped in the window list */
/* Everything running-app detection needs, taken once per refresh: every
 * task's Exec name and CLI command name, and every window's owning task and
 * title.  Each launcher is then matched against this copy, instead of each
 * one searching the task lists and walking all windows with Intuition locked.
 *
 * The task lists are walked with interrupts disabled (interrupts move tasks
 * between them) but only POINTERS are recorded there; the names are copied
 * afterwards.  The whole snapshot runs inside Forbid(): no other task runs,
 * so none can exit and free its name meanwhile (only a running task can
 * remove itself), while interrupts stay enabled for the copying. */
#define ND_CLI_SNAPSHOT 96
#define ND_WINDOW_SNAPSHOT 64
#define ND_WINDOW_TITLE 128
struct NDCliSnapshot {
    int count;
    int complete;              /* 0: more tasks than slots */
    struct Task *task[ND_CLI_SNAPSHOT];
    char name[ND_CLI_SNAPSHOT][64];       /* CLI command name, or "" */
    char task_name[ND_CLI_SNAPSHOT][64];  /* Exec task name, or "" */
    int windows,windows_complete;
    struct Task *window_task[ND_WINDOW_SNAPSHOT];
    char window_title[ND_WINDOW_SNAPSHOT][ND_WINDOW_TITLE];
};
static void copy_snapshot_text(char *out,const char *text,int limit)
{
    int n=0;
    if (text) while (n<limit-1 && text[n]) {out[n]=text[n];++n;}
    out[n]='\0';
}
static void snapshot_cli_commands(struct NDCliSnapshot *snap)
{
    static struct Node *found[ND_CLI_SNAPSHOT];
    static UBYTE *command[ND_CLI_SNAPSHOT];
    struct List *lists[2];
    struct Node *node;
    struct Screen *screen;
    struct Window *window;
    ULONG lock;
    int list,length,i,n=0;
    snap->count=0;
    snap->complete=1;
    lists[0]=&SysBase->TaskReady;
    lists[1]=&SysBase->TaskWait;
    Forbid();
    /* Our own task is in neither list (FindTask() also finds it). */
    found[n]=(struct Node *)FindTask(NULL);command[n++]=NULL;
    Disable();
    for (list=0;list<2;++list)
        for (node=lists[list]->lh_Head;node->ln_Succ;node=node->ln_Succ) {
            if (n>=ND_CLI_SNAPSHOT) {snap->complete=0;break;}
            found[n]=node;command[n]=NULL;
            if (node->ln_Type==NT_PROCESS && ((struct Process *)node)->pr_CLI) {
                struct CommandLineInterface *cli=
                    (struct CommandLineInterface *)BADDR(((struct Process *)node)->pr_CLI);
                if (cli && cli->cli_Module && cli->cli_CommandName)
                    command[n]=(UBYTE *)BADDR(cli->cli_CommandName);
            }
            ++n;
        }
    Enable();
    for (i=0;i<n;++i) {
        snap->task[i]=(struct Task *)found[i];
        copy_snapshot_text(snap->task_name[i],found[i]->ln_Name,64);
        snap->name[i][0]='\0';
        if (command[i] && command[i][0]) {
            length=command[i][0];
            if (length>63) length=63;
            memcpy(snap->name[i],command[i]+1,(size_t)length);
            snap->name[i][length]='\0';
        }
    }
    snap->count=n;
    Permit();
    /* Windows: owner and title, under one short Intuition lock. */
    snap->windows=0;snap->windows_complete=1;
    lock=LockIBase(0);
    for (screen=IntuitionBase->FirstScreen;screen;screen=screen->NextScreen)
        for (window=screen->FirstWindow;window;window=window->NextWindow) {
            if (window==dock_window_for_snapshot) continue;
            if (snap->windows>=ND_WINDOW_SNAPSHOT) {snap->windows_complete=0;break;}
            snap->window_task[snap->windows]=window->UserPort ? window->UserPort->mp_SigTask : NULL;
            copy_snapshot_text(snap->window_title[snap->windows],(const char *)window->Title,ND_WINDOW_TITLE);
            ++snap->windows;
        }
    UnlockIBase(lock);
}
/* FindTask(NAME) against the snapshot (exact Exec task name). */
static struct Task *snapshot_task_find(const struct NDCliSnapshot *snap,const char *wanted)
{
    struct Task *task;
    int i;
    for (i=0;i<snap->count;++i)
        if (!strcmp(snap->task_name[i],wanted)) return snap->task[i];
    if (snap->complete) return NULL;
    Forbid();task=FindTask((STRPTR)wanted);Permit();
    return task;
}
/* launcher_window() against the snapshot: 1 when a window matches. */
static int snapshot_window_match(const struct NDItem *item,const struct NDCliSnapshot *snap,
                                 struct Task *task,struct Task *port_task)
{
    int i,shell;
    const char *pattern=ITEM_TEXT(item,ND_TEXT_MATCH_WINDOW);
    shell=!ITEM_TEXT(item,ND_TEXT_MATCH_TASK)[0] && !pattern[0] &&
          !ITEM_TEXT(item,ND_TEXT_MATCH_PORT)[0] &&
          nd_launcher_matches_shell_window(item->command,"AmigaShell");
    if (!task && !port_task && !shell && !pattern[0]) return 0;
    for (i=0;i<snap->windows;++i) {
        const char *title=snap->window_title[i];
        if ((task && snap->window_task[i]==task) ||
            (port_task && snap->window_task[i]==port_task) ||
            (pattern[0] && title[0] && nd_text_matches_pattern(pattern,title)) ||
            (shell && title[0] && nd_launcher_matches_shell_window(item->command,title)))
            return 1;
    }
    return 0;
}
static struct Task *snapshot_cli_find(const struct NDCliSnapshot *snap,
                                      const char *wanted)
{
    int i;
    if (!wanted || !*wanted) return NULL;
    for (i=0;i<snap->count;++i)
        if (snap->name[i][0] && nd_command_name_matches(snap->name[i],wanted)) return snap->task[i];
    return snap->complete ? NULL : find_cli_command(wanted);
}

/* launcher_task(), optionally matching CLI commands against SNAP. */
static struct Task *launcher_task_using(const struct NDItem *item,
                                        const struct NDCliSnapshot *snap)
{
    char name[64];
    struct Task *task;
    if (!item) return NULL;
    if (ITEM_TEXT(item,ND_TEXT_MATCH_TASK)[0]) {
        strncpy(name,ITEM_TEXT(item,ND_TEXT_MATCH_TASK),sizeof(name)-1);
        name[sizeof(name)-1]='\0';
    } else {
        /* Any explicit identity switches this launcher out of automatic
         * basename matching. This prevents a stale/unrelated task with the
         * executable's name from overriding a deliberate window/port rule. */
        if (ITEM_TEXT(item,ND_TEXT_MATCH_WINDOW)[0] || ITEM_TEXT(item,ND_TEXT_MATCH_PORT)[0]) return NULL;
        if (!launcher_task_name(item,name,sizeof(name))) return NULL;
    }
    if (snap) task=snapshot_task_find(snap,name);
    else {Forbid();task=FindTask((STRPTR)name);Permit();}
    if (!task) task=snap ? snapshot_cli_find(snap,name) : find_cli_command(name);
    return task;
}

struct Task *launcher_task(const struct NDItem *item)
{
    return launcher_task_using(item,NULL);
}
struct Task *launcher_port_task(const struct NDItem *item,int *present)
{
    struct MsgPort *port=NULL;
    struct Task *task=NULL;
    if (present) *present=0;
    if (!item || !ITEM_TEXT(item,ND_TEXT_MATCH_PORT)[0]) return NULL;
    Forbid();
    port=FindPort((STRPTR)ITEM_TEXT(item,ND_TEXT_MATCH_PORT));
    if (port) {
        if (present) *present=1;
        task=port->mp_SigTask;
    }
    Permit();
    return task;
}
struct Window *launcher_window(struct NDDock *dock,
                                      const struct NDItem *item,
                                      struct Task *task,
                                      struct Task *port_task)
{
    struct Screen *screen;
    struct Window *window,*found=NULL;
    ULONG lock;
    int shell;
    if (!dock || !item) return NULL;
    shell=!ITEM_TEXT(item,ND_TEXT_MATCH_TASK)[0] && !ITEM_TEXT(item,ND_TEXT_MATCH_WINDOW)[0] &&
          !ITEM_TEXT(item,ND_TEXT_MATCH_PORT)[0] &&
          nd_launcher_matches_shell_window(item->command,"AmigaShell");
    if (!task && !port_task && !shell && !ITEM_TEXT(item,ND_TEXT_MATCH_WINDOW)[0]) return NULL;
    lock=LockIBase(0);
    for (screen=IntuitionBase->FirstScreen;screen && !found;
         screen=screen->NextScreen)
        for (window=screen->FirstWindow;window;window=window->NextWindow) {
            if (window==dock->window) continue;
            if ((task && window->UserPort &&
                 window->UserPort->mp_SigTask==task) ||
                (port_task && window->UserPort &&
                 window->UserPort->mp_SigTask==port_task) ||
                (ITEM_TEXT(item,ND_TEXT_MATCH_WINDOW)[0] && window->Title &&
                 nd_text_matches_pattern(ITEM_TEXT(item,ND_TEXT_MATCH_WINDOW),window->Title)) ||
                (shell && window->Title &&
                 nd_launcher_matches_shell_window(item->command,
                                                  window->Title))) {
                found=window;break;
            }
        }
    UnlockIBase(lock);
    return found;
}
static int launcher_running_using(struct NDDock *dock,const struct NDItem *item,
                                  const struct NDCliSnapshot *snap)
{
    struct Task *task=launcher_task_using(item,snap);
    struct Task *port_task;
    int port_present=0;
    if (task) return 1;
    port_task=launcher_port_task(item,&port_present);
    if (port_present) return 1;
    if (snap && snap->windows_complete) return snapshot_window_match(item,snap,task,port_task);
    return launcher_window(dock,item,task,port_task)!=NULL;
}

int launcher_running(struct NDDock *dock,const struct NDItem *item)
{
    return launcher_running_using(dock,item,NULL);
}
static int indicator_box(struct NDDock *dock,int index,int *x,int *y,int *w)
{
    if(dock->hover_headroom && dock->hover_plan_ready && index>=0 && index<dock->conf.count &&
       dock->hover_layout[index].kind==ND_HOVER_ART){
        if(dock->painting_shelf)return 0;
        if(dock->painting_wave){const struct NDRect *box=&dock->hover_rects[index];
            *x=box->left;*y=box->top;*w=box->width;return 1;}
    }
    if(dock->hover_headroom && dock->hover_plan_ready && index>=0 && index<dock->conf.count &&
       dock->hover_layout[index].kind!=ND_HOVER_ART)return 1;
    if(index>=0 && index<dock->conf.count){
        int tx,ty,tw,th,iw,ih;tile_coords(dock,index,&tx,&ty);tile_dimensions(dock,index,&tw,&th);iw=tw;ih=th;
        if(hover_art_size(dock,index,tw,th,&iw,&ih)){
            *x=tx+(tw-iw)/2;*y=nd_icon_art_top(ty,th,ih,dock->conf.item_size==ND_ITEM_SIZE_TINY);*w=iw;
        }
    }
    return 1;
}
static void running_dot(struct NDDock *dock,int index,int x,int y,int w)
{
    int th,iw,ih,tw,tx,ty;
    if(!indicator_box(dock,index,&x,&y,&w))return;
    if(dock->hover_headroom && dock->hover_plan_ready && dock->painting_wave &&
       dock->hover_layout[index].kind==ND_HOVER_ART){
        const struct NDRect *box=&dock->hover_rects[index];
        x=box->left;w=box->width;y=box->top+box->height+2;
    }else{
        tile_coords(dock,index,&tx,&ty);tile_dimensions(dock,index,&tw,&th);
        iw=tw;ih=th;
        if(hover_art_size(dock,index,tw,th,&iw,&ih))
            y=nd_icon_art_top(ty,th,ih,dock->conf.item_size==ND_ITEM_SIZE_TINY)+ih+2;
        else y=ty+th-20;
    }
    fill(dock,ND_ACCENT2,x+(w-3)/2,y,3,2);
}
static void draw_running_indicator(struct NDDock *dock,int index,
                                   int x,int y,int w)
{
    if (dock->conf.running_apps && index>=0 && index<ND_MAX_ITEMS &&
        dock->running_items[index])
        running_dot(dock,index,x,y,w);
}
/* Only reuse identities within this refresh. A Task pointer must never be
 * cached across ticks because the process may exit between observations. */
static int same_running_identity(const struct NDItem *a,const struct NDItem *b)
{
    return a->kind==b->kind && a->launch_mode==b->launch_mode &&
           !strcmp(a->command,b->command) &&
           !strcmp(ITEM_TEXT(a,ND_TEXT_MATCH_TASK),ITEM_TEXT(b,ND_TEXT_MATCH_TASK)) &&
           !strcmp(ITEM_TEXT(a,ND_TEXT_MATCH_PORT),ITEM_TEXT(b,ND_TEXT_MATCH_PORT)) &&
           !strcmp(ITEM_TEXT(a,ND_TEXT_MATCH_WINDOW),ITEM_TEXT(b,ND_TEXT_MATCH_WINDOW));
}
void refresh_running_items(struct NDDock *dock)
{
    static struct NDCliSnapshot snap;   /* ~21 KB: too large for the stack */
    int i,j,running,taken=0;
    if (!dock || dock->hidden || dock->covered) return;
    for (i=0;i<dock->conf.count && i<ND_MAX_ITEMS;++i) {
        const struct NDItem *item=&dock->conf.items[i];
        int check=dock->conf.running_apps && item->kind==ND_ITEM_LAUNCH;
        /* Tiles on another page are refreshed when they come into view. */
        if (check && dock->window && !item_visible(dock,i)) continue;
        if (check && !taken) {dock_window_for_snapshot=dock->window;snapshot_cli_commands(&snap);taken=1;}
        running=item->transient==ND_TRANSIENT_TASK;
        if (check && !running) {
            for (j=0;j<i;++j) {
                const struct NDItem *previous=&dock->conf.items[j];
                if (previous->transient==ND_TRANSIENT_TASK ||
                    (dock->window && !item_visible(dock,j))) continue;
                if (same_running_identity(previous,item)) break;
            }
            running=j<i ? dock->running_items[j] :
                          launcher_running_using(dock,item,&snap);
        }
        if (dock->running_items[i]!=(unsigned char)running) {
            dock->running_items[i]=(unsigned char)running;
            if (dock->window && item_visible(dock,i)) mark_tile_dirty(dock,i);
        }
    }
    for (;i<ND_MAX_ITEMS;++i) dock->running_items[i]=0;
}
static int row_offset_pixels(struct NDDock *dock,int row)
{
    int used;
    if (!dock || vertical(dock) || dock->overflow ||
        dock->layout_rows<=1 || dock->layout_columns<1) return 0;
    used=nd_layout_row_used_cells(&dock->conf,row,dock->layout_columns);
    if (used<1 || used>=dock->layout_columns) return 0;
    return (dock->layout_columns-used)*(dock->tile_width+dock->gap)/2;
}
int dock_grip_rect(struct NDDock *dock,struct NDRect *rect)
{
    if(!dock || !dock->conf.drag_bar || dock->hidden)return 0;
    rect->left=dock->hover_pad_x;rect->top=dock->hover_pad_y;
    rect->width=dock->width-2*dock->hover_axis_padding;
    rect->height=dock->height-dock->hover_headroom;
    if(vertical(dock)){
        rect->width=dock->width-dock->hover_headroom;
        rect->height=dock->height-2*dock->hover_axis_padding;
    }
    if(dock->hover_headroom && dock->hover_plan_ready)*rect=dock->hover_shelf_rect;
    if(vertical(dock)){rect->left+=ND_BORDER;rect->width-=2*ND_BORDER;rect->height=ND_DRAG_BAR_SIZE;}
    else{rect->top+=ND_BORDER;rect->height-=2*ND_BORDER;rect->width=ND_DRAG_BAR_SIZE;}
    return rect->width>0 && rect->height>0;
}
int hovered_grip(struct NDDock *dock,int mx,int my)
{
    struct NDRect r;
    return dock_grip_rect(dock,&r) && mx>=r.left && mx<r.left+r.width &&
        my>=r.top && my<r.top+r.height;
}
static void draw_grip(struct NDDock *dock)
{
    struct NDRect r;int i,x,y;
    if(dock->painting_mask || !dock_grip_rect(dock,&r))return;
    x=r.left+r.width/2;y=r.top+r.height/2;
    /* Six high-contrast notches remain readable on planar and RTG screens. */
    for(i=-6;i<=6;i+=4){
        if(vertical(dock)){
            fill(dock,ND_SUBTLE,x+i,y-3,2,6);
            fill(dock,ND_TEXT,x+i,y-3,1,5);
        }else{
            fill(dock,ND_SUBTLE,x-3,y+i,6,2);
            fill(dock,ND_TEXT,x-3,y+i,5,1);
        }
    }
}
void slot_coords(struct NDDock *dock, int slot, int *x, int *y)
{
    int row=0,column=slot,row_offset=0;
    if (!vertical(dock) && dock->layout_rows>1 && dock->layout_columns>0) {
        row=slot/dock->layout_columns;
        column=slot%dock->layout_columns;
        /* Pack items predictably from the left, then center each completed
         * row inside the common dock frame.  In particular, a short final
         * row no longer leaves all of its unused space on the right. */
        row_offset=row_offset_pixels(dock,row);
    }
    *x = ND_BORDER + dock->hover_pad_x + (vertical(dock) ? 0 :
                     (dock->conf.drag_bar ? ND_DRAG_BAR_SIZE:0)+
                     column * (dock->tile_width + dock->gap)+row_offset);
    *y = ND_BORDER + dock->hover_pad_y + (vertical(dock) ?
                     (dock->conf.drag_bar ? ND_DRAG_BAR_SIZE:0)+slot * (dock->tile_height + dock->gap) :
                     row * (dock->tile_height + dock->gap));
}
static int item_axis_size(struct NDDock *dock,int index)
{
    int normal=vertical(dock) ? dock->tile_height : dock->tile_width;
    int custom;
    if (index<0 || index>=dock->conf.count) return normal;
    /* The music transport needs room for metadata and three controls.  It is
     * one logical item, but spans exactly two ordinary horizontal slots. */
    if (!vertical(dock) && dock->conf.item_size!=ND_ITEM_SIZE_TINY &&
        dock->conf.items[index].kind==ND_ITEM_PLUGIN &&
        !strcmp(dock->conf.items[index].id,"MUSIC"))
        return normal*2+dock->gap;
    if (dock->overflow || dock->conf.items[index].kind!=ND_ITEM_SPACER)
        return normal;
    custom=dock->conf.items[index].spacer_width;
    if (custom && dock->conf.item_size==ND_ITEM_SIZE_TINY)
        custom=(custom+1)/2;
    return custom ? custom : (normal+1)/2;
}
static int item_slot_span(struct NDDock *dock,int index)
{
    if (!vertical(dock) && dock->conf.item_size!=ND_ITEM_SIZE_TINY &&
        index>=0 && index<dock->conf.count &&
        dock->conf.items[index].kind==ND_ITEM_PLUGIN &&
        !strcmp(dock->conf.items[index].id,"MUSIC")) return 2;
    return 1;
}
void tile_dimensions(struct NDDock *dock,int index,int *w,int *h)
{
    *w=dock->tile_width;*h=dock->tile_height;
    if (vertical(dock)) *h=item_axis_size(dock,index);
    else *w=item_axis_size(dock,index);
}
void tile_coords(struct NDDock *dock, int index, int *x, int *y)
{
    int slot,i,axis=ND_BORDER+(dock->conf.drag_bar ? ND_DRAG_BAR_SIZE:0);
    if (!item_visible(dock,index)) { *x=-10000; *y=-10000; return; }
    if (dock->overflow || (!vertical(dock) && dock->layout_rows>1)) {
        if (!dock->overflow && dock->conf.row_layout) {
            int row,column,span;
            if (!nd_layout_item_cell(&dock->conf,index,dock->layout_columns,
                                     &row,&column,&span)) {
                *x=-10000;*y=-10000;return;
            }
            slot=row*dock->layout_columns+column;
        } else {
            slot=dock->overflow ? 1 : 0;
            for (i=dock->overflow ? dock->view_start : 0;i<=index;++i) {
                int span=item_slot_span(dock,i);
                int column=dock->layout_columns>0 ?
                           slot%dock->layout_columns : 0;
                if (dock->layout_rows>1 && column+span>dock->layout_columns)
                    slot+=dock->layout_columns-column;
                if (i==index) break;
                slot+=span;
            }
        }
        slot_coords(dock,slot,x,y);return;
    }
    for (i=0;i<index;++i) axis+=item_axis_size(dock,i)+dock->gap;
    *x=(vertical(dock) ? ND_BORDER:axis)+dock->hover_pad_x;
    *y=(vertical(dock) ? axis:ND_BORDER)+dock->hover_pad_y;
}
/* Render at most one dot per stored sample. This is a sparkline, not
 * an oscilloscope: every incoming plugin update adds one sample.
 * Normalization is supplied by each plugin and documented per metric.
 */
/* The graph area of tile INDEX drawn at X,Y (TW x TH). */
static void graph_area(struct NDDock *dock,int index,int x,int y,int tw,int th,
                       int *gx,int *gy,int *gw,int *gh)
{
    *gx=x+6;*gw=tw-12;
    if (index>=0 && index<dock->conf.count &&
        (!strcmp(dock->conf.items[index].id,"RAM") ||
         !strcmp(dock->conf.items[index].id,"NET") ||
         !strcmp(dock->conf.items[index].id,"ROADSHOW"))) {
        /* RAM's two colours identify Chip and Fast memory; its
         * detailed figures live in the tooltip, leaving the graph uncluttered.
         * Reserve the common plugin heading row above the graph. */
        *gy=y+(th>=60 ? 31 : 22);*gh=th-(th>=60 ? 36 : 27);
    } else if (th >= 60) { *gy = y + 34; *gh = th - 39; }
    else { *gy = y + 27; *gh = th - 32; }
}

/* The traces from column FIRST_COL to the right edge (all of them when it
 * is GX).  Drawing rules that let a scrolled graph (draw_graph_step) match a
 * full redraw pixel for pixel (checked by simulation):
 *  - the two series are drawn interleaved, segment by segment, so redrawing
 *    the segments entering a zone restores the same stacking order;
 *  - every segment restarts its dash pattern (SetDrPt + Move), so its pixels
 *    depend only on its end points;
 *  - a full draw also draws the segment entering the leftmost column from
 *    the sample just outside, then clears that outer column;
 *  - a partial draw never draws at the point just left of FIRST_COL: it is
 *    already correct and only starts the first segment into the zone;
 *  - the newest-point marker appears only when the newest sample exists. */
static void graph_series(struct NDDock *dock,int index,int gx,int gy,int gw,
                         int gh,int first_col)
{
    static const UWORD patterns[2]={0xFFFFU,0xCCCCU}; /* TX dashed */
    static const int colors[2]={ND_ACCENT,ND_ACCENT2};
    struct NDGraphHistory *hist = &dock->history[index];
    struct RastPort *rp = dock->window->RPort;
    int count, extra, age, series, px, py;
    int old_x[2], old_y[2], have[2];
    unsigned int sample[2];
    count = (int)hist->count;
    if (count > gw) count = gw;
    extra = first_col==gx && (int)hist->count > count;
    have[0]=have[1]=0;
    old_x[0]=old_x[1]=old_y[0]=old_y[1]=0;
    for (age = count - 1 + extra; age >= 0; --age) {
        if (!nd_graph_get(hist, (unsigned int)age, &sample[0], &sample[1])) {
            have[0]=have[1]=0;
            continue;
        }
        px = gx + gw - 1 - age;
        for (series = 0; series < 2; ++series) {
            if (sample[series] > 100U || px < first_col - 1) {
                have[series] = 0;
                continue;
            }
            py = gy + gh - 1 - (int)(sample[series] * (unsigned int)(gh - 1) / 100U);
            if (px < first_col) {
                old_x[series]=px;old_y[series]=py;have[series]=1;
                continue;
            }
            SetAPen(rp, pen(dock, colors[series]));
            SetDrPt(rp, patterns[series]);
            Move(rp, have[series] ? old_x[series] : px,
                     have[series] ? old_y[series] : py);
            Draw(rp, px, py);
            old_x[series]=px;old_y[series]=py;have[series]=1;
        }
    }
    SetDrPt(rp, 0xFFFFU);
    if (extra) fill(dock, tile_interior(dock,index), gx-1, gy, 1, gh);
    for (series = 0; series < 2; ++series)
        if (have[series] && old_x[series] == gx + gw - 1) {
            SetAPen(rp, pen(dock, colors[series]));
            RectFill(rp,old_x[series]>gx ? old_x[series]-1 : old_x[series],
                     old_y[series],old_x[series],
                     old_y[series]<gy+gh-1 ? old_y[series]+1 : old_y[series]);
        }
}

/* The interior colour a tile is painted on (see tile_box). */
static int tile_interior(struct NDDock *dock,int index)
{
    if (index==dock->hover) return ND_HOVER;
    return dock->conf.shape==ND_SHAPE_MINIMAL ? ND_BG : ND_TILE;
}

static void draw_graph(struct NDDock *dock, int index,
                       int x, int y, int tw, int th)
{
    struct RastPort *rp = dock->window->RPort;
    struct NDGraphView *view = &dock->graph_view[index];
    int gx, gy, gw, gh, s;
    UWORD pattern;
    graph_area(dock,index,x,y,tw,th,&gx,&gy,&gw,&gh);
    if (gw < 2 || gh < 3) return;
    /* One dotted midpoint guide provides scale without looking like a third
     * data series or an unexplained grey progress bar.  Its dots sit where
     * (x-gx+phase) is a multiple of four, so they move with scrolled graphs. */
    s=(4-(view->phase&3))&3;
    pattern=(UWORD)(((0x8888U>>s)|(0x8888U<<(16-s)))&0xFFFFU);
    SetAPen(rp,pen(dock,ND_BAR_BG));
    SetDrPt(rp,pattern);
    Move(rp,gx,gy+gh/2);Draw(rp,gx+gw-1,gy+gh/2);
    SetDrPt(rp,0xFFFFU);
    graph_series(dock,index,gx,gy,gw,gh,gx);
    view->x=(short)x;view->y=(short)y;view->tw=(short)tw;view->th=(short)th;
    view->gx=(short)gx;view->gy=(short)gy;view->gw=(short)gw;view->gh=(short)gh;
    view->bg=(UBYTE)tile_interior(dock,index);
    view->painted=dock->graph_samples[index];
}
static int icon_hovered(const struct NDDock *dock,int index)
{
    return dock->conf.hover_magnification && dock->hover>=0 &&
           (dock->hover_headroom || index==dock->hover) && !dock->is_drag;
}
static int wave_icon_size(struct NDDock *dock,int index,int tw,int th,int *w,int *h)
{
    if(!icon_hovered(dock,index))return 0;
    if(dock->hover_headroom && dock->hover_plan_ready){
        const struct NDHoverLayoutItem *p=&dock->hover_layout[index];
        if(p->width<=*w && p->height<=*h)return 0;
        *w=p->width;*h=p->height;return 1;
    }
    {int iw=*w,ih=*h;
     if(!nd_icon_hover_size(tw,th,dock->conf.item_size==ND_ITEM_SIZE_TINY,w,h))return 0;
     *w=iw+(*w-iw)*dock->conf.hover_magnification/100;
     *h=ih+(*h-ih)*dock->conf.hover_magnification/100;
     return *w>iw || *h>ih;}
}
static int wave_icon_shift(struct NDDock *dock,int index,int image_width)
{
    const struct NDHoverLayoutItem *p=&dock->hover_layout[index];
    (void)image_width;
    return dock->hover_headroom && dock->hover_plan_ready ? p->centre-p->anchor:0;
}
static int wave_icon_top(struct NDDock *dock,int y,int th,int size,int tiny)
{
    if(dock->hover_headroom)return y+th-(tiny ? 4 : 24)-size;
    return nd_icon_art_top(y,th,size,tiny);
}
static void draw_cached_icon(struct NDDock *dock,int index,int x,int y,int tw,int th,
                            struct BitMap *bitmap,struct BitMap *mask,int iw,int ih)
{
    int tiny=dock->conf.item_size==ND_ITEM_SIZE_TINY;
    int zoom_width=iw,zoom_height=ih;
    if(dock->painting_shelf)return;
    if (wave_icon_size(dock,index,tw,th,&zoom_width,&zoom_height))
        magnify_icon(dock,index,&bitmap,&mask,&iw,&ih,zoom_width,zoom_height);
    if(dock->hover_headroom && dock->hover_plan_ready){
        const struct NDRect *box=&dock->hover_rects[index];
        BltMaskBitMapRastPort(bitmap,0,0,dock->window->RPort,box->left,box->top,iw,ih,0xe0,mask->Planes[0]);
    }else BltMaskBitMapRastPort(bitmap,0,0,dock->window->RPort,x+(tw-iw)/2,
                         wave_icon_top(dock,y,th,ih,tiny),iw,ih,0xE0,mask->Planes[0]);
}

/* Palette-based ILBM drawing; no PNG datatype or chunky RTG surface.
 * Coalesce neighbouring pixels of the same colour into horizontal runs. */
static int draw_brush(struct NDDock *dock, int index, int x, int y,
                      int tw, int th)
{
    const struct NDILBM *b = dock->brushes[index];
    struct RastPort *rp = dock->window->RPort;
    int row, col, start, pal, xx, yy,iw,ih,max_width,max_height,tiny;
    if (!b) return 0;
    tiny=dock->conf.item_size==ND_ITEM_SIZE_TINY;
    nd_icon_art_box(tw,th,tiny,&max_width,&max_height);
    iw=dock->scaled_icon_width[index] ? dock->scaled_icon_width[index] : b->width;
    ih=dock->scaled_icon_height[index] ? dock->scaled_icon_height[index] : b->height;
    if (iw>max_width || ih>max_height) return 0;
    xx = x + (tw - iw) / 2;
    yy=nd_icon_art_top(y,th,ih,tiny);
    /* The ILBM is converted once into a screen-friendly bitmap and mask.
     * Repainting then becomes one blit instead of dozens of RectFill calls. */
    if (dock->brush_bitmaps[index] && dock->brush_masks[index]) {
        draw_cached_icon(dock,index,x,y,tw,th,dock->brush_bitmaps[index],
                         dock->brush_masks[index],iw,ih);
        return 1;
    }
    if (iw!=b->width || ih!=b->height) return 0;
    for (row = 0; row < b->height; ++row) {
        col = 0;
        while (col < b->width) {
            pal = b->pixels[row * ND_ICON_MAX_DIM + col];
            start = col++;
            while (col < b->width &&
                   b->pixels[row * ND_ICON_MAX_DIM + col] == pal) ++col;
            if (pal != b->transparent && dock->brush_pens[index][pal] >= 0) {
                SetAPen(rp, (ULONG)dock->brush_pens[index][pal]);
                RectFill(rp, xx + start, yy + row, xx + col - 1, yy + row);
            }
        }
    }
    return 1;
}
/* Analog Clock is drawn by the host, not by an unsafe plugin RastPort.
 * Fixed-point lookup eliminates floating point/libm and supports 68000. */
static const signed char nd_clock_x[60] = {
0, 10, 21, 31, 41, 50, 59, 67, 74, 81, 87, 91, 95, 98, 99, 100, 99, 98, 95, 91, 87, 81, 74, 67, 59, 50, 41, 31, 21, 10, 0, -10, -21, -31, -41, -50, -59, -67, -74, -81, -87, -91, -95, -98, -99, -100, -99, -98, -95, -91, -87, -81, -74, -67, -59, -50, -41, -31, -21, -10
};
static const signed char nd_clock_y[60] = {
-100, -99, -98, -95, -91, -87, -81, -74, -67, -59, -50, -41, -31, -21, -10, 0, 10, 21, 31, 41, 50, 59, 67, 74, 81, 87, 91, 95, 98, 99, 100, 99, 98, 95, 91, 87, 81, 74, 67, 59, 50, 41, 31, 21, 10, 0, -10, -21, -31, -41, -50, -59, -67, -74, -81, -87, -91, -95, -98, -99
};
static void draw_analog_clock(struct NDDock *dock,int x,int y,int w,int h)
{
    struct RastPort *rp=dock->window->RPort;
    struct DateStamp stamp;
    int tiny=dock->conf.item_size==ND_ITEM_SIZE_TINY;
    int cx=x+w/2,cy=tiny ? y+h/2 : y+(h-13)/2;
    int rad=tiny ? (h-8)/2 : (h-23)/2;
    int i,m,hour,hr_index,second,hr_len,min_len,sec_len,tick_len;
    int max_rad=tiny ? (w-8)/2 : (w-20)/2;
    if (rad>max_rad) rad=max_rad;
    if (rad<(tiny ? 6 : 8)) return;
    tick_len=tiny ? 2 : 3;
    hr_len=tiny ? (rad*55)/100 : rad-8;
    min_len=tiny ? (rad*75)/100 : rad-4;
    sec_len=tiny ? (rad*80)/100 : rad-3;
    DateStamp(&stamp);
    hour=(int)((stamp.ds_Minute/60L)%24L);
    m=(int)(stamp.ds_Minute%60L);
    second=(int)(stamp.ds_Tick/50L);
    SetAPen(rp,pen(dock,ND_SUBTLE));
    for (i=0;i<60;i+=5) {
        int ax=cx+(nd_clock_x[i]*rad)/100;
        int ay=cy+(nd_clock_y[i]*rad)/100;
        Move(rp,ax,ay);
        Draw(rp,cx+nd_clock_x[i]*(rad-tick_len)/100,
                cy+nd_clock_y[i]*(rad-tick_len)/100);
    }
    SetAPen(rp,pen(dock,ND_TEXT));
    hr_index=((hour%12)*5+m/12)%60;
    Move(rp,cx,cy);
    Draw(rp,cx+nd_clock_x[hr_index]*hr_len/100,
            cy+nd_clock_y[hr_index]*hr_len/100);
    SetAPen(rp,pen(dock,ND_ACCENT));
    Move(rp,cx,cy);
    Draw(rp,cx+nd_clock_x[m]*min_len/100,
            cy+nd_clock_y[m]*min_len/100);
    if (DOCK_OPTS(dock)->clock_seconds) {
        SetAPen(rp,pen(dock,ND_ACCENT2));
        Move(rp,cx,cy);
        Draw(rp,cx+nd_clock_x[second%60]*sec_len/100,
                cy+nd_clock_y[second%60]*sec_len/100);
    }
    if (!tiny) caption_text_normal(dock,ND_SUBTLE,"CLOCK",x,y+h-7,w);
}
/* The Eyes plugin is an ordinary Exec-message process; screen-pointer
 * rendering stays inside the host because RastPorts are not IPC-safe.
 * Scan-line ellipse fill avoids floating point and extra graphics surfaces.
 */
int eyes_index(const struct NDDock *dock)
{
    int i;
    for (i=0;i<dock->conf.count;++i)
        if (dock->conf.items[i].kind==ND_ITEM_PLUGIN &&
            !strcmp(dock->conf.items[i].id,"EYES") &&
            dock->conf.items[i].online && item_visible(dock,i)) return i;
    return -1;
}
void invalidate_settings_cache(struct NDDock *dock)
{
    if (dock) dock->opts.valid=0;
}
static int eyes_color_option(struct NDDock *dock,const char *key,int white_pen)
{
    const char *v=nd_option_get(&dock->conf,"EYES",key);
    if (v && !strcmp(v,"BLUE")) return ND_ACCENT;
    if (v && !strcmp(v,"GREEN")) return ND_ACCENT2;
    return white_pen;
}
static int option_yes(struct NDDock *dock,const char *id,const char *key)
{
    const char *v=nd_option_get(&dock->conf,id,key);
    return v && !strcmp(v,"YES");
}
void load_settings_cache(struct NDDock *dock)
{
    const char *v;
    v=nd_option_get(&dock->conf,"EYES","POLL_TICKS");
    dock->opts.eyes_poll_ticks=v ? atoi(v) : 10;
    v=nd_option_get(&dock->conf,"EYES","SENSITIVITY");
    dock->opts.eyes_sensitivity=v ? atoi(v) : 100;
    v=nd_option_get(&dock->conf,"EYES","SIZE");
    dock->opts.eyes_radius=v && !strcmp(v,"SMALL") ? 9 :
                           v && !strcmp(v,"LARGE") ? 15 : 12;
    dock->opts.eyes_show_label=option_yes(dock,"EYES","SHOW_LABEL");
    dock->opts.eyes_blink=option_yes(dock,"EYES","BLINK");
    dock->opts.eyes_sclera=eyes_color_option(dock,"EYE_COLOR",ND_TEXT);
    dock->opts.eyes_pupil=eyes_color_option(dock,"PUPIL_COLOR",ND_BG);
    dock->opts.clock_seconds=option_yes(dock,"CLOCK","SECONDS");
    v=nd_option_get(&dock->conf,"CLOCK","FORMAT");
    dock->opts.clock_analog=v && !strcmp(v,"ANALOG");
    dock->opts.valid=1;
}
static void eyes_filled_ellipse(struct NDDock *dock,
                                int cx,int cy,int rx,int ry,int color)
{
    struct RastPort *rp=dock->window->RPort;
    int y,x;
    LONG rx2,ry2,bound;
    if (rx<1 || ry<1) return;
    rx2=(LONG)rx*rx; ry2=(LONG)ry*ry; bound=rx2*ry2;
    SetAPen(rp,pen(dock,color));
    for (y=-ry;y<=ry;++y) {
        for (x=rx;x>0;--x)
            if ((LONG)x*x*ry2+(LONG)y*y*rx2 <= bound) break;
        RectFill(rp,cx-x,cy+y,cx+x,cy+y);
    }
}
static void eyes_geometry(struct NDDock *dock,int index,
                          int centers_x[2],int centers_y[2],
                          int *rx,int *ry,int *pr)
{
    int x,y,w=dock->tile_width,h=dock->tile_height;
    int r = DOCK_OPTS(dock)->eyes_radius;
    int usable_h=h-(dock->conf.item_size!=ND_ITEM_SIZE_TINY && dock->opts.eyes_show_label ? 16 : 6);
    tile_coords(dock,index,&x,&y);
    if (r>(w-20)/4) r=(w-20)/4;
    if (r>(usable_h-8)/2) r=(usable_h-8)/2;
    if (r<3) r=3;
    *rx=r; *ry=r+2; if (*ry>(usable_h-6)/2) *ry=(usable_h-6)/2;
    if (*ry<3) *ry=3;
    *pr=r>=13 ? 4 : r>=9 ? 3 : 2;
    centers_x[0]=dock_left(dock)+x+w/2-r-3;
    centers_x[1]=dock_left(dock)+x+w/2+r+3;
    centers_y[0]=centers_y[1]=dock_top(dock)+y+usable_h/2;
}
void eyes_positions(struct NDDock *dock,int index,
                           int xp[2],int yp[2],int *rx,int *ry,int *pr)
{
    int ex[2],ey[2],i,sensitivity;
    eyes_geometry(dock,index,ex,ey,rx,ry,pr);
    sensitivity=DOCK_OPTS(dock)->eyes_sensitivity;
    for (i=0;i<2;++i)
        nd_eyes_pupil(ex[i],ey[i],dock->screen->MouseX,
                      dock->screen->MouseY,*rx-*pr-2,*ry-*pr-2,
                      sensitivity,&xp[i],&yp[i]);
}
static void draw_eyes(struct NDDock *dock,int index,int x,int y,int tw,int th)
{
    int xp[2],yp[2],rx,ry,pr,ex[2],ey[2],i;
    int sclera=DOCK_OPTS(dock)->eyes_sclera;
    int pupil=dock->opts.eyes_pupil;
    if(dock->painting_mask && pupil==ND_BG)pupil=ND_BAR_BG;
    eyes_positions(dock,index,xp,yp,&rx,&ry,&pr);
    eyes_geometry(dock,index,ex,ey,&rx,&ry,&pr);
    for (i=0;i<2;++i) {
        int xc=ex[i]-dock_left(dock);
        int yc=ey[i]-dock_top(dock);
        if (dock->eyes_blink || !dock->conf.items[index].online) {
            fill(dock,ND_SUBTLE,xc-rx,yc,rx*2+1,2);
        } else {
            eyes_filled_ellipse(dock,xc,yc,rx,ry,sclera);
            SetAPen(dock->window->RPort,pen(dock,ND_ACCENT));
            DrawEllipse(dock->window->RPort,xc,yc,rx,ry);
            eyes_filled_ellipse(dock,xp[i]-dock_left(dock),
                yp[i]-dock_top(dock),pr,pr,pupil);
        }
        dock->eyes_last_x[index][i]=xp[i];
        dock->eyes_last_y[index][i]=yp[i];
    }
    dock->eyes_last_valid[index]=1;
    if (dock->conf.item_size!=ND_ITEM_SIZE_TINY && DOCK_OPTS(dock)->eyes_show_label)
        caption_text_normal(dock,ND_SUBTLE,"EYES",x,y+th-7,tw);
}

static const char *volume_label(const struct NDItem *item)
{static char label[32];if(item->volume_show_free && item->show_volume_meter && item->volume_total && item->volume_total>=item->volume_used && item->volume_block_bytes){nd_volume_size(item->volume_total-item->volume_used,item->volume_block_bytes,label);strcat(label," free");return label;}return item->label;}

static void draw_volume_meter(struct NDDock *dock, const struct NDItem *item,
                              int x, int y, int tw, int th)
{
    int meter_w;
    if (!nd_item_supports_space_meter(item) || !item->show_volume_meter ||
        item->meter > 100U) return;
    meter_w = tw - 16;
    if (meter_w < 4) return;
    /* Leave visible dock backdrop below the meter, especially in Minimal. */
    fill(dock, ND_BAR_BG, x + 8, y + th - 6, meter_w, 2);
    if (item->meter > 0)
        fill(dock, item->volume_low_notified ? ND_ACCENT2:ND_ACCENT, x + 8, y + th - 6,
             (int)(item->meter * (ULONG)meter_w / 100UL), 2);
}

static int parse_ram_pair(const char *value,unsigned int *chip,
                          unsigned int *fast)
{
    unsigned long cu,cd,ct,ctd,fu,fd,ft,ftd;
    unsigned long used,total;
    if (!value || sscanf(value,"C:%lu.%lu/%lu.%lu F:%lu.%lu/%lu.%lu",
        &cu,&cd,&ct,&ctd,&fu,&fd,&ft,&ftd)!=8) return 0;
    used=cu*10UL+cd;total=ct*10UL+ctd;
    *chip=total ? (unsigned int)(used*100UL/total) : 0U;
    used=fu*10UL+fd;total=ft*10UL+ftd;
    *fast=total ? (unsigned int)(used*100UL/total) : 0U;
    if (*chip>100U) *chip=100U;
    if (*fast>100U) *fast=100U;
    return 1;
}

int format_ram_tooltip(const char *value,char *chip_text,
                              unsigned int chip_size,char *fast_text,
                              unsigned int fast_size)
{
    unsigned long gu,gd,gt,gtd,fu,fd,ft,ftd;
    unsigned long used,total,free_tenths;
    if (!value || !chip_text || !chip_size || !fast_text || !fast_size ||
        sscanf(value,"C:%lu.%lu/%lu.%lu F:%lu.%lu/%lu.%lu",
               &gu,&gd,&gt,&gtd,&fu,&fd,&ft,&ftd)!=8) return 0;
    used=gu*10UL+gd;total=gt*10UL+gtd;
    free_tenths=total>used ? total-used : 0;
    sprintf(chip_text,"CHIP: %lu.%luM used, %lu.%luM free, %lu.%luM total",
            used/10UL,used%10UL,free_tenths/10UL,free_tenths%10UL,
            total/10UL,total%10UL);
    chip_text[chip_size-1]='\0';
    used=fu*10UL+fd;total=ft*10UL+ftd;
    free_tenths=total>used ? total-used : 0;
    sprintf(fast_text,"Fast: %lu.%luM used, %lu.%luM free, %lu.%luM total",
            used/10UL,used%10UL,free_tenths/10UL,free_tenths%10UL,
            total/10UL,total%10UL);
    fast_text[fast_size-1]='\0';
    return 1;
}

struct MetricPaint {struct NDDock *dock;int x,y,size,kind;};
static void metric_box(void *ctx,int ink,int x,int y,int w,int h)
{
    struct MetricPaint *p=(struct MetricPaint *)ctx;
    int color=ND_METRIC_COLORS+(p->kind-1)*5+ink;
    int left=x*p->size/48,top=y*p->size/48;
    fill(p->dock,color,p->x+left,p->y+top,
        (x+w)*p->size/48-left,(y+h)*p->size/48-top);
}
static void metric_text_box(void *ctx,int ink,int x,int y,int w,int h)
{struct MetricPaint *p=(struct MetricPaint *)ctx;fill(p->dock,ink,x,y,w,h);}
static int metric_kind(struct NDDock *dock,const struct NDItem *item)
{
    if(item->kind!=ND_ITEM_PLUGIN || (item->graph && dock->conf.item_size!=ND_ITEM_SIZE_TINY))return 0;
    if(!strcmp(item->id,"CPU"))return 1;
    if(!strcmp(item->id,"RAM"))return 2;
    if(!strcmp(item->id,"NET") || !strcmp(item->id,"ROADSHOW"))return 4;
    if(!strcmp(item->id,"CLOCK") && !DOCK_OPTS(dock)->clock_analog)return 3;
    return 0;
}
static int metric_source_box(struct NDDock *dock,int index,int *x,int *y,int *w,int *h)
{
    int tiny=dock->conf.item_size==ND_ITEM_SIZE_TINY;
    int size,kind=metric_kind(dock,&dock->conf.items[index]);
    if(!kind)return 0;
    size=*h-(tiny ? 4:24);
    if(size>*w-(tiny ? 4:12))size=*w-(tiny ? 4:12);if(size>48)size=48;if(size<16)return 0;
    *x+=(*w-size)/2;*y=nd_icon_art_top(*y,*h,size,tiny);*w=*h=size;return 1;
}
static void draw_metric_tile(struct NDDock *dock,int index,int kind,int x,int y,int tw,int th)
{
    const struct NDItem *item=&dock->conf.items[index];
    struct MetricPaint p;unsigned int chip=0,fast=0;
    int tiny=dock->conf.item_size==ND_ITEM_SIZE_TINY;
    int size,cx=x,cy=y,cw=tw,ch=th;
    if(!metric_source_box(dock,index,&cx,&cy,&cw,&ch))return;
    size=cw;p.dock=dock;p.size=size;p.x=cx;p.y=cy;p.kind=kind;
    nd_metric_art(&p,metric_box,kind);
    if(kind==2){
        (void)parse_ram_pair(item->value,&chip,&fast);
        if(item->online){
            if(chip)metric_box(&p,3,7,16,(int)(34UL*chip/100UL),5);
            if(fast)metric_box(&p,4,7,27,(int)(34UL*fast/100UL),5);
        }
    }else if(kind==4){
        unsigned int rx=0,tx=0;char rates[24];int row,valid;
        valid=item->online && sscanf(item->value,"%u/%u p/s",&rx,&tx)==2;
        for(row=0;row<2;++row){
            unsigned int rate=row ? tx:rx;
            if(!valid)strcpy(rates,"--");
            else if(rate>=1000000000U)sprintf(rates,"%uG",rate/1000000000U);
            else if(rate>=1000000U)sprintf(rates,"%uM",rate/1000000U);
            else if(rate>=1000U)sprintf(rates,"%uk",rate/1000U);
            else sprintf(rates,"%u",rate);
            if(tiny)nd_metric_tiny_text(&p,metric_text_box,ND_METRIC_COLORS+15+(row ? 4:2),rates,p.x+12*size/48,p.y+(row ? 24:13)*size/48,28*size/48);
            else centered_text_tight(dock,ND_METRIC_COLORS+15+(row ? 4:2),rates,
                p.x+12*size/48,p.y+(row ? 29:19)*size/48,28*size/48);
        }
    }else{
        int baseline=p.y+25*size/48+dock->window->RPort->TxBaseline/2;
        /* Native font readings stay legible in the idle artwork, then scale
         * with the cached icon when magnification is active. */
        if(tiny){char time[8];const char *value=item->online ? item->value:"--";if(kind==3&&item->online){nd_metric_tiny_time(item->value,time);value=time;}
            nd_metric_tiny_text(&p,metric_text_box,ND_METRIC_COLORS+(kind-1)*5+(item->online ? 2:1),value,p.x+4*size/48,p.y+25*size/48-2,40*size/48);
        }else centered_text_tight(dock,ND_METRIC_COLORS+(kind-1)*5+(item->online ? 2:1),
            item->online ? item->value:"--",p.x+4*size/48,baseline,40*size/48);
        if(kind==1 && item->online && item->meter<=100U)
            metric_box(&p,3,11,33,(int)(26UL*item->meter/100UL),2);
    }
    if(!tiny)caption_text_normal(dock,ND_TEXT,item->label,x,y+th-9,tw);
}

static void draw_ram_tile(struct NDDock *dock,const struct NDItem *item,
                          int x,int y,int tw,int th)
{
    unsigned int chip=0,fast=0;
    int meter_w=tw-20,bar_h=th>=60 ? 5 : 3;
    int chip_y=y+th-29,fast_y=y+th-13;
    (void)parse_ram_pair(item->value,&chip,&fast);
    caption_text_normal(dock,ND_SUBTLE,"RAM",x,y+(th>=60 ? 24 : 17),tw);
    fill(dock,ND_BAR_BG,x+10,chip_y,meter_w,bar_h);
    if (item->online && chip) fill(dock,ND_ACCENT,x+10,chip_y,
                    (int)(chip*(ULONG)meter_w/100UL),bar_h);
    fill(dock,ND_BAR_BG,x+10,fast_y,meter_w,bar_h);
    if (item->online && fast) fill(dock,ND_ACCENT2,x+10,fast_y,
                    (int)(fast*(ULONG)meter_w/100UL),bar_h);
}

/* Both traffic channels share the configured PPS scale.  State messages
 * replace rates during control operations; never leave stale traffic bars. */
static void draw_network_meters(struct NDDock *dock,int index,
                                 int x,int y,int tw,int th)
{
    struct NDItem *item=&dock->conf.items[index];
    unsigned int incoming=0,outgoing=0,rx=0,tx=0;
    int valid,channel,bar_h=th>=60 ? 5 : 3;
    int label_w=TextLength(dock->window->RPort,(STRPTR)"OUT",3)+6;
    int bar_x=x+8+label_w,bar_w=tw-16-label_w;
    int row[2];
    valid=sscanf(item->value,"%u/%u p/s",&rx,&tx)==2;
    if(valid) (void)nd_graph_get(&dock->history[index],0,&incoming,&outgoing);
    row[0]=y+th-(th>=60 ? 27 : 20);
    row[1]=y+th-10;
    caption_text_normal(dock,ND_SUBTLE,"NETWORK",x,y+(th>=60 ? 24 : 17),tw);
    for(channel=0;channel<2;++channel){
        unsigned int level=channel ? outgoing:incoming;
        int color=channel ? ND_ACCENT2:ND_ACCENT;
        centered_text_tight(dock,color,channel ? "OUT":"IN",
                            x+8,row[channel]+bar_h,label_w-4);
        if(bar_w>0){
            fill(dock,ND_BAR_BG,bar_x,row[channel],bar_w,bar_h);
            if(valid && item->online && level<=100U && level)
                fill(dock,color,bar_x,row[channel],
                     (int)(level*(ULONG)bar_w/100UL),bar_h);
        }
    }
}

static void draw_ram_graph_tile(struct NDDock *dock,int index,
                                int x,int y,int tw,int th)
{
    caption_text_normal(dock,ND_SUBTLE,"RAM",x,y+(th>=60 ? 24 : 17),tw);
    draw_graph(dock,index,x,y,tw,th);
}

static void music_button_box(struct NDDock *dock,int x,int y,int w,int h,
                             int enabled,int focus,int pressed)
{
    struct RastPort *rp=dock->window->RPort;
    int color=enabled ? (focus ? ND_ACCENT:ND_SUBTLE):ND_BAR_BG;
    fill(dock,enabled && focus ? (pressed ? ND_ACCENT:ND_HOVER):ND_BG,x,y,w,h);
    if(pressed){fill(dock,ND_BAR_BG,x+1,y+1,w-2,1);fill(dock,ND_BAR_BG,x+1,y+1,1,h-2);}
    SetAPen(rp,pen(dock,color));
    Move(rp,x,y+h-1);Draw(rp,x,y);Draw(rp,x+w-1,y);
    Draw(rp,x+w-1,y+h-1);Draw(rp,x,y+h-1);
}
static void music_transport_icon(struct NDDock *dock,int kind,
                                 int x,int y,int w,int h,int enabled,int playing,int focus,int pressed)
{
    struct RastPort *rp=dock->window->RPort;
    int cx=x+w/2+(pressed ? 1:0),cy=y+h/2+(pressed ? 1:0),color=enabled ? (pressed ? ND_BG:ND_TEXT):ND_SUBTLE;
    music_button_box(dock,x,y,w,h,enabled,focus,pressed);
    SetAPen(rp,pen(dock,color));
    if (kind<0) {
        Move(rp,cx-4,cy-4);Draw(rp,cx-4,cy+4);
        Move(rp,cx+4,cy-4);Draw(rp,cx-3,cy);Draw(rp,cx+4,cy+4);
    } else if (kind>0) {
        Move(rp,cx+4,cy-4);Draw(rp,cx+4,cy+4);
        Move(rp,cx-4,cy-4);Draw(rp,cx+3,cy);Draw(rp,cx-4,cy+4);
    } else if (playing) {
        Move(rp,cx-3,cy-4);Draw(rp,cx-3,cy+4);
        Move(rp,cx+3,cy-4);Draw(rp,cx+3,cy+4);
    } else {
        Move(rp,cx-3,cy-4);Draw(rp,cx+4,cy);Draw(rp,cx-3,cy+4);
        Draw(rp,cx-3,cy-4);
    }
}
static void draw_music_tile(struct NDDock *dock,const struct NDItem *item,
                            int x,int y,int tw,int th)
{
    int top=th>=60 ? y+31 : y+22;
    int bh=th>=60 ? 14 : 12;
    int gap=2,margin=6,bw=(tw-margin*2-gap*2)/3;
    int track_x=x+8,track_w=tw-16,track_y=y+th-8;
    int volume=item->media_volume<=100U ? item->media_volume : 50;
    int knob;
    unsigned int caps=item->media_capabilities;
    int index=(int)(item-dock->conf.items);
    unsigned int focus=dock->hover==index ? dock->music_hover_action:0;
    unsigned int held=dock->press_item==index ? dock->music_press_action:0;
    if(held)focus=held;
    caption_text_normal(dock,ND_SUBTLE,"MUSIC",x,y+(th>=60 ? 11 : 9),tw);
    centered_text_tight(dock,item->online ? ND_TEXT : ND_SUBTLE,
                        item->value,x,y+(th>=60 ? 25 : 18),tw);
    music_transport_icon(dock,-1,x+margin,top,bw,bh,
                         (caps & ND_MEDIA_CAP_PREVIOUS)!=0,0,focus==ND_EVENT_MEDIA_PREVIOUS,held==ND_EVENT_MEDIA_PREVIOUS);
    music_transport_icon(dock,0,x+margin+bw+gap,top,bw,bh,
                         (caps & ND_MEDIA_CAP_PLAY_PAUSE)!=0,
                         (item->media_state & ND_MEDIA_STATE_PLAYING)!=0,focus==ND_EVENT_MEDIA_PLAY_PAUSE,held==ND_EVENT_MEDIA_PLAY_PAUSE);
    music_transport_icon(dock,1,x+margin+(bw+gap)*2,top,bw,bh,
                         (caps & ND_MEDIA_CAP_NEXT)!=0,0,focus==ND_EVENT_MEDIA_NEXT,held==ND_EVENT_MEDIA_NEXT);
    fill(dock,ND_BAR_BG,track_x,track_y,track_w,2);
    if (caps & ND_MEDIA_CAP_VOLUME) {
        if (volume>0) fill(dock,ND_ACCENT2,track_x,track_y,
            (int)(volume*(ULONG)track_w/100UL),2);
        knob=track_x+(int)(volume*(ULONG)(track_w-1)/100UL);
        fill(dock,focus==ND_EVENT_MEDIA_VOLUME ? ND_ACCENT:ND_TEXT,knob-(focus==ND_EVENT_MEDIA_VOLUME ? 2:1),track_y-2,focus==ND_EVENT_MEDIA_VOLUME ? 5:3,6);
    }
}
/* Returns one ND_EVENT_MEDIA_* action. Volume uses the release position so
 * click-and-drag on the track acts like a compact absolute slider. */
static unsigned int music_control_hit(struct NDDock *dock,int index,
                                      int mx,int my,unsigned int *volume)
{
    struct NDItem *item;
    int x,y,tw,th,top,bh,gap=2,margin=6,bw,which;
    if (volume) *volume=ND_MEDIA_VOLUME_UNKNOWN;
    if (dock->edit_mode) return 0;
    if (index<0 || index>=dock->conf.count) return 0;
    item=&dock->conf.items[index];
    if (item->kind!=ND_ITEM_PLUGIN || strcmp(item->id,"MUSIC") ||
        !item->online || !item->media_capabilities) return 0;
    tile_coords(dock,index,&x,&y);tile_dimensions(dock,index,&tw,&th);
    /* Transport widgets are scaled from this source tile into the wave.
     * Map the visible pointer back through exactly that transform. */
    if(dock->hover_headroom && dock->hover_plan_ready &&
       dock->hover_layout[index].kind==ND_HOVER_WIDGET){
        const struct NDRect *box=&dock->hover_rects[index];
        if(box->width<1 || box->height<1)return 0;
        mx=x+(mx-box->left)*tw/box->width;
        my=y+(my-box->top)*th/box->height;
    }
    /* draw_tile_content reserves the caption row before drawing MUSIC.
     * The inverse transform above still uses the full source bitmap. */
    if(dock->conf.item_size!=ND_ITEM_SIZE_TINY){
        int reserve=dock->window->RPort->TxHeight+8;
        if(th-reserve>=26)th-=reserve;
    }
    if (dock->conf.item_size==ND_ITEM_SIZE_TINY) {
        int button_w=15,button_h=12;
        int button_x=x+(tw-button_w)/2;
        if ((item->media_capabilities&ND_MEDIA_CAP_VOLUME) &&
            my>=y+th-12 && my<y+th && mx>=x+5 && mx<x+tw-5) {
            int start=x+5,width=tw-10,value;
            if (mx<=start) value=0;
            else if (mx>=start+width-1) value=100;
            else value=(mx-start)*100/(width-1);
            if (volume) *volume=(unsigned int)value;
            return ND_EVENT_MEDIA_VOLUME;
        }
        if ((item->media_capabilities&ND_MEDIA_CAP_PLAY_PAUSE) &&
            mx>=button_x && mx<button_x+button_w &&
            my>=y+3 && my<y+3+button_h)
            return ND_EVENT_MEDIA_PLAY_PAUSE;
        return 0;
    }
    if ((item->media_capabilities & ND_MEDIA_CAP_VOLUME) &&
        my>=y+th-13 && my<y+th && mx>=x+5 && mx<x+tw-5) {
        int start=x+8,width=tw-16,value;
        if (mx<=start) value=0;
        else if (mx>=start+width-1) value=100;
        else value=(mx-start)*100/(width-1);
        if (volume) *volume=(unsigned int)value;
        return ND_EVENT_MEDIA_VOLUME;
    }
    top=th>=60 ? y+31 : y+22;bh=th>=60 ? 14 : 12;
    bw=(tw-margin*2-gap*2)/3;
    if (my<top || my>=top+bh || mx<x+margin ||
        mx>=x+margin+bw*3+gap*2) return 0;
    which=(mx-(x+margin))/(bw+gap);
    if (which==0 && (item->media_capabilities & ND_MEDIA_CAP_PREVIOUS))
        return ND_EVENT_MEDIA_PREVIOUS;
    if (which==1 && (item->media_capabilities & ND_MEDIA_CAP_PLAY_PAUSE))
        return ND_EVENT_MEDIA_PLAY_PAUSE;
    if (which==2 && (item->media_capabilities & ND_MEDIA_CAP_NEXT))
        return ND_EVENT_MEDIA_NEXT;
    return 0;
}
static int set_music_control(struct NDDock *dock,int index,unsigned int action,unsigned int volume)
{
    struct NDItem *item=&dock->conf.items[index];
    /* Accumulate until the next plugin exchange: a later click must not
     * discard an earlier one, and a second Play/Pause cancels the first
     * (the tile state below toggles back in step). */
    item->media_event=(unsigned short)nd_media_fold(item->media_event,action,
                                                    ND_EVENT_MEDIA_PLAY_PAUSE);
    if (action==ND_EVENT_MEDIA_VOLUME && volume<=100U) {
        item->media_volume=(unsigned short)volume;
        item->media_volume_request=(unsigned short)volume;
    } else if (action==ND_EVENT_MEDIA_PLAY_PAUSE) {
        item->media_state ^= ND_MEDIA_STATE_PLAYING;
    }
    /* Wake the client before a potentially expensive magnified repaint. */
    wake_pending_plugins(dock);
    draw_tile(dock,index);
    return 1;
}

int queue_music_control(struct NDDock *dock,int index,int mx,int my)
{
    unsigned int action,volume;
    action=music_control_hit(dock,index,mx,my,&volume);
    return action ? set_music_control(dock,index,action,volume):0;
}
int update_music_control_hover(struct NDDock *dock,int index,int mx,int my)
{
    unsigned int volume,focus=music_control_hit(dock,index,mx,my,&volume);
    if(focus==dock->music_hover_action)return 0;
    dock->music_hover_action=focus;
    dock->hover_shelf_valid=0;
    return 1;
}
int begin_music_control(struct NDDock *dock,int index,int mx,int my)
{
    unsigned int volume;
    dock->music_press_action=music_control_hit(dock,index,mx,my,&volume);
    if(dock->music_press_action==ND_EVENT_MEDIA_VOLUME)
        (void)set_music_control(dock,index,ND_EVENT_MEDIA_VOLUME,volume);
    else if(dock->music_press_action)draw_tile(dock,index);
    return dock->music_press_action!=0;
}
/* A held slider owns the gesture, including movement beyond the tile.
 * Its displayed geometry stays fixed until release; endpoints clamp. */
void drag_music_control(struct NDDock *dock,int mx)
{
    struct NDItem *item;int index=dock->press_item,x,y,tw,th,start,width,value;
    if(dock->music_press_action!=ND_EVENT_MEDIA_VOLUME || dock->edit_mode ||
       index<0 || index>=dock->conf.count)return;
    item=&dock->conf.items[index];
    if(!item->online || !(item->media_capabilities&ND_MEDIA_CAP_VOLUME))return;
    tile_coords(dock,index,&x,&y);tile_dimensions(dock,index,&tw,&th);
    if(dock->hover_headroom && dock->hover_plan_ready &&
       dock->hover_layout[index].kind==ND_HOVER_WIDGET){
        const struct NDRect *box=&dock->hover_rects[index];
        if(box->width<1)return;
        mx=x+(mx-box->left)*tw/box->width;
    }
    start=x+(dock->conf.item_size==ND_ITEM_SIZE_TINY ? 5:8);
    width=tw-(dock->conf.item_size==ND_ITEM_SIZE_TINY ? 10:16);
    value=mx<=start ? 0:mx>=start+width-1 ? 100:(mx-start)*100/(width-1);
    if(item->media_volume!=(unsigned int)value)
        (void)set_music_control(dock,index,ND_EVENT_MEDIA_VOLUME,(unsigned int)value);
}
void finish_music_control(struct NDDock *dock,int mx,int my)
{
    unsigned int volume,action=dock->music_press_action;
    int index=dock->press_item;
    if(action==ND_EVENT_MEDIA_VOLUME)drag_music_control(dock,mx);
    else if(action && music_control_hit(dock,index,mx,my,&volume)==action)
        (void)set_music_control(dock,index,action,volume);
    dock->music_press_action=0;
}

/* Half-size plugins retain the two pieces of information which matter at a
 * glance and omit transport buttons, legends and full graphs. */
static void draw_tiny_plugin(struct NDDock *dock,const struct NDItem *item,
                             int x,int y,int tw,int th)
{
    int meter_w=tw-10;
    int first=y+10,second=y+(th>=36 ? 23 : 20);
    const char *caption=item->caption[0] ? item->caption : item->label;
    if (!strcmp(item->id,"ARCHIVE")) {
        centered_text_tight(dock,ND_TEXT,"EXTR",x,y+th/2+3,tw);
        return;
    }
    if (!strcmp(item->id,"CLOCK")) {
        if (DOCK_OPTS(dock)->clock_analog)
            draw_analog_clock(dock,x,y,tw,th);
        else
            centered_text_tight(dock,item->online ? ND_TEXT : ND_SUBTLE,
                                item->value,x,y+th/2+3,tw);
        return;
    }
    if (!strcmp(item->id,"MUSIC")) {
        int caps=item->media_capabilities;
        int button_w=15,button_h=12;
        int button_x=x+(tw-button_w)/2;
        int track_x=x+5,track_w=tw-10,track_y=y+th-6;
        int volume=item->media_volume<=100U ? item->media_volume : 50;
        int knob;
        int index=(int)(item-dock->conf.items);
        unsigned int focus=dock->hover==index ? dock->music_hover_action:0;
        unsigned int held=dock->press_item==index ? dock->music_press_action:0;
        if(held)focus=held;
        /* An idle player still needs an immediately recognisable tile.  Keep
         * it as compact as EXTR; controls replace the label when available. */
        if (!item->online || !caps) {
            centered_text_tight(dock,ND_TEXT,"MUSIC",x,y+th/2+3,tw);
            return;
        }
        music_transport_icon(dock,0,button_x,y+3,button_w,button_h,
            (caps&ND_MEDIA_CAP_PLAY_PAUSE)!=0,
            (item->media_state&ND_MEDIA_STATE_PLAYING)!=0,
            focus==ND_EVENT_MEDIA_PLAY_PAUSE,held==ND_EVENT_MEDIA_PLAY_PAUSE);
        fill(dock,ND_BAR_BG,track_x,track_y,track_w,2);
        if (caps&ND_MEDIA_CAP_VOLUME) {
            if (volume>0) fill(dock,ND_ACCENT2,track_x,track_y,
                (int)(volume*(ULONG)track_w/100UL),2);
            knob=track_x+(int)(volume*(ULONG)(track_w-1)/100UL);
            fill(dock,focus==ND_EVENT_MEDIA_VOLUME ? ND_ACCENT:ND_TEXT,
                knob-(focus==ND_EVENT_MEDIA_VOLUME ? 2:1),track_y-2,
                focus==ND_EVENT_MEDIA_VOLUME ? 5:3,6);
        }
        return;
    }
    if (!strcmp(item->id,"RAM")) {
        unsigned int chip=0,fast=0;
        int bar_h=3,chip_y=y+18,fast_y=y+th-8;
        (void)parse_ram_pair(item->value,&chip,&fast);
        caption_text_tight(dock,ND_SUBTLE,"RAM",x,y+10,tw);
        fill(dock,ND_BAR_BG,x+5,chip_y,meter_w,bar_h);
        if (item->online && chip) fill(dock,ND_ACCENT,x+5,chip_y,
                    (int)(chip*(ULONG)meter_w/100UL),bar_h);
        fill(dock,ND_BAR_BG,x+5,fast_y,meter_w,bar_h);
        if (item->online && fast) fill(dock,ND_ACCENT2,x+5,fast_y,
                    (int)(fast*(ULONG)meter_w/100UL),bar_h);
        return;
    }
    caption_text_tight(dock,ND_SUBTLE,caption,x,first,tw);
    centered_text_tight(dock,item->online ? ND_TEXT : ND_SUBTLE,
                        item->value,x,second,tw);
    if (item->meter<=100U && meter_w>=4) {
        fill(dock,ND_BAR_BG,x+5,y+th-5,meter_w,2);
        if (item->meter)
            fill(dock,ND_ACCENT,x+5,y+th-5,
                 (int)(item->meter*(ULONG)meter_w/100UL),2);
    }
}

static int dopus_application_running(const struct NDItem *item)
{
    if (!item || strcmp(item->id,"DOPUS4")) return 0;
    /* Keep the protocol's more detailed state for click/restore handling,
     * but reduce the tile itself to the same visual language as launchers. */
    return !strcmp(item->value,"READY") ||
           !strcmp(item->value,"ICONIFIED") ||
           !strcmp(item->value,"FRONT FAILED");
}

static void draw_plugin_launcher_artwork(struct NDDock *dock,int index,
                                         const char *label,
                                         int x,int y,int tw,int th)
{
    int tiny=dock->conf.item_size==ND_ITEM_SIZE_TINY;
    struct BitMap *art,*art_mask;
    int iw,ih;
    if (dock->brushes[index] && draw_brush(dock,index,x,y,tw,th)) {
        /* Bundled ILBM artwork uses the same cached brush path as launchers. */
    } else if (tile_icon_art(dock,index,&art,&art_mask,&iw,&ih)) {
        draw_cached_icon(dock,index,x,y,tw,th,art,art_mask,iw,ih);
    } else if (dock->icons[index]) {
        struct Image *im=(struct Image *)
            dock->icons[index]->do_Gadget.GadgetRender;
        if (im && im->Depth>=1 && im->Depth<=4 && im->Width>0 &&
            im->Height>0 && im->Width<=tw-(tiny ? 2 : 14) &&
            im->Height<=th-(tiny ? 2 : 27))
            DrawImage(dock->window->RPort,im,
                      x+(tw-im->Width)/2,
                      nd_icon_art_top(y,th,im->Height,tiny));
    }
    if (!tiny)
        caption_text_tight(dock,ND_TEXT,label,x,y+th-9,tw);
}

static void draw_dopus_tile(struct NDDock *dock,int index,
                            const struct NDItem *item,
                            int x,int y,int tw,int th)
{
    draw_plugin_launcher_artwork(dock,index,"DOpus4",x,y,tw,th);
    if (dopus_application_running(item) && indicator_box(dock,index,&x,&y,&tw))
        fill(dock,ND_ACCENT2,x+tw-9,y+5,4,4);
}

static void draw_music_idle_tile(struct NDDock *dock,int index,
                                 int x,int y,int tw,int th)
{
    draw_plugin_launcher_artwork(dock,index,"MUSIC",x,y,tw,th);
}

static void draw_running_apps_tile(struct NDDock *dock,int index,
                                   int x,int y,int tw,int th)
{
    /* Running Apps is a launcher for a transient application picker, not an
     * application whose resident-plugin state belongs on the tile. */
    draw_plugin_launcher_artwork(dock,index,"APPS",x,y,tw,th);
}

/* A plugin's badge (ND_MSG_BADGE) on the tile's top-right corner: the text
 * on a secondary-accent label, or a dot for "*" and on tiny tiles.  It takes
 * the place of the running dot, which shares that corner. */
static int draw_plugin_badge(struct NDDock *dock,const struct NDItem *item,
                             int x,int y,int tw)
{
    struct RastPort *rp=dock->window->RPort;
    char text[sizeof(item->badge)];
    int length=(int)strlen(item->badge),width,height,bx,by,available=tw;
    int index;
    if (!length) return 0;
    for(index=0;index<dock->conf.count;++index)if(item==&dock->conf.items[index])break;
    if(dock->hover_headroom && dock->hover_plan_ready && index<dock->conf.count &&
       dock->hover_layout[index].kind==ND_HOVER_ART){
        const struct NDRect *box=&dock->hover_rects[index];
        if(dock->painting_shelf)return 0;
        if(dock->painting_wave){
            /* Keep the full slot width even when the icon is only 24 pixels. */
            if(tw<box->width)tw=box->width;
            x=box->left+(box->width-tw)/2;y=box->top;
        }
    }
    available=tw;strcpy(text,item->badge);
    if (!strcmp(item->badge,"*") || dock->conf.item_size==ND_ITEM_SIZE_TINY) {
        fill(dock,ND_ACCENT2,x+tw-11,y+4,6,6);
        return 1;
    }
    height=rp->TxHeight+2;
    while (length>0 &&
           (width=TextLength(rp,(STRPTR)text,length)+6)>available-6){
        --length;if(length>0){text[length-1]='+';text[length]=0;}
    }
    if (!length) {
        fill(dock,ND_ACCENT2,x+tw-11,y+4,6,6);
        return 1;
    }
    bx=x+tw-width-3;by=y+3;
    fill(dock,ND_ACCENT2,bx,by,width,height);
    SetAPen(rp,pen(dock,ND_BG));
    SetDrMd(rp,JAM1);
    Move(rp,bx+3,by+1+rp->TxBaseline);
    Text(rp,(STRPTR)text,length);
    return 1;
}

static void draw_plugin_running_indicator(struct NDDock *dock,
                                          const struct NDItem *item,
                                          int x,int y,int tw)
{
    int mode=nd_indicator_mode(item->id,item->online,dock->conf.running_apps,
                                item->media_capabilities,item->badge);
    if(mode==ND_INDICATOR_BADGE)(void)draw_plugin_badge(dock,item,x,y,tw);
    else if(mode==ND_INDICATOR_DOT){
        int index;
        for(index=0;index<dock->conf.count;++index)if(item==&dock->conf.items[index])break;
        if(index<dock->conf.count)running_dot(dock,index,x,y,tw);
    }
}

/* Health is separate from notification badges and running-app dots. */
static void draw_plugin_state(struct NDDock *dock,int index,int x,int y,int tw)
{
    const struct NDItem *item=&dock->conf.items[index];int state,bx,by,color;
    if(item->kind!=ND_ITEM_PLUGIN || dock->painting_shelf)return;
    state=nd_tile_state(nd_option_schema_id(&dock->conf,item->id),item->online,item->value);
    if(!state || !indicator_box(dock,index,&x,&y,&tw))return;
    if(!dock->hover_headroom || !dock->hover_plan_ready || dock->hover_layout[index].kind!=ND_HOVER_ART){
        int tx,ty,w,h,iw,ih;tile_coords(dock,index,&tx,&ty);tile_dimensions(dock,index,&w,&h);iw=w;ih=h;
        if(hover_art_size(dock,index,w,h,&iw,&ih)){x=tx+(w-iw)/2;y=nd_icon_art_top(ty,h,ih,dock->conf.item_size==ND_ITEM_SIZE_TINY);}
    }
    bx=x;by=y;color=state==ND_TILE_OFFLINE ? ND_SUBTLE:state==ND_TILE_ERROR ? ND_ACCENT2:ND_ACCENT;
    fill(dock,ND_BG,bx,by,8,8);
    if(state==ND_TILE_LOADING){
        fill(dock,color,bx+1,by+1,6,1);fill(dock,color,bx+1,by+6,6,1);
        fill(dock,color,bx+2,by+2,4,1);fill(dock,color,bx+3,by+3,2,2);fill(dock,color,bx+2,by+5,4,1);
    }else if(state==ND_TILE_OFFLINE){
        fill(dock,color,bx+1,by+1,6,1);fill(dock,color,bx+1,by+6,6,1);
        fill(dock,color,bx+1,by+2,1,4);fill(dock,color,bx+6,by+2,1,4);fill(dock,color,bx+3,by+3,2,1);
    }else{
        if(state==ND_TILE_UNAVAILABLE){fill(dock,color,bx+2,by+1,4,1);fill(dock,color,bx+5,by+2,1,2);fill(dock,color,bx+3,by+4,2,1);}
        else fill(dock,color,bx+3,by+1,2,4);
        fill(dock,color,bx+3,by+6,2,1);
    }
}

/* Caption and value rows of the plain graph tile. */
static void graph_tile_text(struct NDDock *dock,const struct NDItem *item,
                            int x,int y,int tw,int th)
{
    const char *caption=!strcmp(item->id,"CPU") ? "CPU" : !strncmp(item->caption,"ART:",4) ? item->label:item->caption;
    if(!strcmp(item->id,"NET") || !strcmp(item->id,"ROADSHOW")){
        caption_text_normal(dock,ND_SUBTLE,"NETWORK",x,y+(th>=60 ? 24 : 17),tw);
        return;
    }
    caption_text_normal(dock, ND_SUBTLE, caption, x, y + (th >= 60 ? 13 : 10), tw);
    centered_text(dock, item->online ? ND_TEXT : ND_SUBTLE,
                  item->value, x, y + (th >= 60 ? 28 : 23), tw);
}

/* One new sample on a plain graph tile: scroll the graph left by a pixel and
 * redraw only the rightmost columns and the text rows.  Anything else (a
 * hover, a press, a moved tile, several samples at once, the RAM layout)
 * falls back to a full draw_tile(), so the result always matches one. */
void draw_graph_step(struct NDDock *dock,int index)
{
    struct NDItem *item=&dock->conf.items[index];
    struct NDGraphView *view=&dock->graph_view[index];
    struct RastPort *rp;
    int x,y,tw,th,bg,zone,col;
    ULONG behind;
    if (!dock->window || index<0 || index>=dock->conf.count) return;
    behind=dock->graph_samples[index]-view->painted;
    if (!behind) return;
    tile_coords(dock,index,&x,&y);
    tile_dimensions(dock,index,&tw,&th);
    bg=tile_interior(dock,index);
    if (dock->background.parts[0].bitmap || behind!=1 || !view->valid || bg!=view->bg ||
        dock->press_item==index || dock->is_drag ||
        x!=view->x || y!=view->y || tw!=view->tw || th!=view->th ||
        view->gw<8 || view->gh<3 || view->gy<=y+3) {
        draw_tile(dock,index);
        return;
    }
    ++dock->diagnostic_tile_paints;
    dock->painting_item=index;
    dock->caption_dirty=1;
    rp=dock->window->RPort;
    /* Text rows and the badge or running dot above the graph. */
    fill(dock,bg,x+3,y+3,tw-6,view->gy-(y+3));
    graph_tile_text(dock,item,x,y,tw,th);
    draw_plugin_running_indicator(dock,item,x,y,tw);
    draw_plugin_state(dock,index,x,y,tw);
    /* Graph: shift left, then redraw the four rightmost columns, which
     * also removes the previous newest-point marker. */
    ScrollRaster(rp,1,0,view->gx,view->gy,
                 view->gx+view->gw-1,view->gy+view->gh-1);
    ++view->phase;
    zone=view->gx+view->gw-4;
    fill(dock,bg,zone,view->gy,4,view->gh);
    SetAPen(rp,pen(dock,ND_BAR_BG));
    for (col=zone;col<view->gx+view->gw;++col)
        if (((col-view->gx+view->phase)&3)==0)
            WritePixel(rp,col,view->gy+view->gh/2);
    graph_series(dock,index,view->gx,view->gy,view->gw,view->gh,zone);
    view->painted=dock->graph_samples[index];
}

static int draw_bank_cached(struct NDDock *dock,int index,const struct NDItem *item,
                            struct NDArtwork *art,const unsigned char *runs,unsigned int count,
                            int size,int x,int y,int tw,int th)
{
    struct NDHoverBank *cache=&dock->hover_banks[index];
    struct RastPort image,mask;
    unsigned int n;
    const unsigned char *bounds;
    if(cache->size!=size){
        WaitBlit();
        if(cache->bitmap)FreeBitMap(cache->bitmap);
        if(cache->mask)FreeBitMap(cache->mask);
        cache->bitmap=NULL;cache->mask=NULL;cache->runs=NULL;cache->size=size;
    }
    if(!cache->bitmap)cache->bitmap=AllocBitMap(size,size,
        dock->screen->RastPort.BitMap->Depth,BMF_CLEAR,dock->screen->RastPort.BitMap);
    if(!cache->mask)cache->mask=AllocBitMap(size,size,1,BMF_CLEAR,NULL);
    if(!cache->bitmap || !cache->mask)return 0;
    if(cache->runs!=runs || cache->online!=item->online){
        bounds=nd_artwork_bounds(art,runs);
        WaitBlit();
        InitRastPort(&image);InitRastPort(&mask);
        image.BitMap=cache->bitmap;mask.BitMap=cache->mask;
        SetRast(&image,0);SetRast(&mask,0);SetAPen(&mask,1);
        for(n=0;n<count;++n){
            const unsigned char *r=runs+n*5;
            int x1,y1,x2,y2;
            ULONG ink=art->mapped[r[4]];
            nd_artwork_scaled_rect(bounds,r,size,&x1,&y1,&x2,&y2);
            if(!dock->conf.colourful_icons && art->data[5]==2 && art->data[56+r[4]]){
                static const int roles[]={ND_ICON_DARK,ND_ICON_CASE,ND_ICON_LIGHT,ND_ICON_ACCENT,ND_ICON_SECONDARY};
                int role=art->data[56+r[4]];
                ink=pen(dock,role==3 && !item->online ? ND_ICON_CASE:roles[role-1]);
            }
            if(x2>x1 && y2>y1){
                SetAPen(&image,ink);RectFill(&image,x1,y1,x2-1,y2-1);
                RectFill(&mask,x1,y1,x2-1,y2-1);
            }
        }
        cache->runs=runs;cache->online=item->online;++cache->serial;
    }
    draw_cached_icon(dock,index,x,y,tw,th,cache->bitmap,cache->mask,size,size);
    return 1;
}

static int draw_plugin_bank(struct NDDock *dock,int index,const struct NDItem *item,int x,int y,int tw,int th)
{
    if(!strcmp(item->id,"MUSIC") && item->media_capabilities)return 0;
    if(item->graph && (!strcmp(item->id,"COMMAND") || !strcmp(item->id,"SCRIPT") || !strcmp(item->id,"MIXER")))return 0;
    if(item->kind==ND_ITEM_PLUGIN && dock->plugin_artwork[index]){
        struct NDArtwork *a=dock->plugin_artwork[index];char art[24];unsigned int state,count,n,flags=0;const unsigned char *runs=NULL;
        if(dock->plugin_art_key[index])runs=nd_artwork_key_frame(a,dock->plugin_art_key[index],dock->plugin_art_state[index],&count,&flags);
        else if(nd_artwork_parse(item->caption,art,&state))runs=nd_artwork_key_frame(a,nd_artwork_key(art),state,&count,&flags);
        if(runs){
            int tiny=dock->conf.item_size==ND_ITEM_SIZE_TINY,size=tw-(a->data[5]==1 || (flags&ND_ARTWORK_LARGE) ? 12:tiny ? 4:16),ix,iy;
            int cached=0,base_size,compact=!tiny && th<64 && (flags&ND_ARTWORK_VALUE);
            static const unsigned char full[4]={0,0,32,32};
            const unsigned char *bounds=NULL;
            int reserve=(flags&ND_ARTWORK_VALUE) ? (tiny || compact ? 15:35):(tiny ? 4:24);
            int maximum=(flags&ND_ARTWORK_HEADING) ? 24:(flags&ND_ARTWORK_VALUE) ? 38:48;
            /* Side docks can fit Normal tiles down to 46 pixels. Keep the
             * selected heading and reading, using a compact two-row footer
             * instead of reserving the tall layout's 50 pixels. */
            if(flags&ND_ARTWORK_HEADING)reserve=tiny ? 15:compact ? 26:50;
            if(size>th-reserve)size=th-reserve;if(size>maximum)size=maximum;if(size<1)return 1;
            base_size=size;
            if(dock->hover_headroom && !(flags&ND_ARTWORK_VALUE) && !dock->painting_shelf)
                cached=draw_bank_cached(dock,index,item,a,runs,count,base_size,x,y,tw,th);
            if (!(flags&ND_ARTWORK_VALUE) && icon_hovered(dock,index)) {
                int width=size,height=size;
                if (wave_icon_size(dock,index,tw,th,&width,&height))
                    size=width;
            }
            ix=x+(tw-size)/2+((flags&ND_ARTWORK_VALUE) ? 0 : wave_icon_shift(dock,index,size));iy=(flags&ND_ARTWORK_VALUE) ? y+3:wave_icon_top(dock,y,th,size,tiny);
            if((!dock->painting_shelf || (flags&ND_ARTWORK_VALUE)) && !cached){bounds=(flags&ND_ARTWORK_VALUE) ? full:nd_artwork_bounds(a,runs);}
            if((!dock->painting_shelf || (flags&ND_ARTWORK_VALUE)) && !cached)
            for(n=0;n<count;++n){const unsigned char *r=runs+n*5;
                int x1,y1,x2,y2;nd_artwork_scaled_rect(bounds,r,size,&x1,&y1,&x2,&y2);
                if(x2>x1 && y2>y1){ULONG ink=dock->painting_mask ? 1UL:a->mapped[r[4]];
                    if(!dock->painting_mask && !dock->conf.colourful_icons && a->data[5]==2 && a->data[56+r[4]]){static const int roles[]={ND_ICON_DARK,ND_ICON_CASE,ND_ICON_LIGHT,ND_ICON_ACCENT,ND_ICON_SECONDARY};int role=a->data[56+r[4]];ink=pen(dock,role==3 && !item->online ? ND_ICON_CASE:roles[role-1]);}
                    SetAPen(dock->window->RPort,ink);RectFill(dock->window->RPort,ix+x1,iy+y1,ix+x2-1,iy+y2-1);}
            }
            if(dock->painting_wave)return 1;
            if((flags&ND_ARTWORK_HEADING) && !tiny)
                label_text(dock,ND_SUBTLE,item->caption,x,y+th-(compact ? 14:35),tw);
            if(flags&ND_ARTWORK_VALUE){char value[32];unsigned long hours,minutes,seconds;strcpy(value,item->value);
                if((flags&ND_ARTWORK_DURATION) && sscanf(value,"%lu:%lu:%lu",&hours,&minutes,&seconds)==3){if(hours)sprintf(value,"%luh%02lum",hours,minutes);else sprintf(value,"%lu:%02lu",minutes,seconds);}
                centered_text_tight(dock,ND_TEXT,value,x,y+th-(tiny || compact ? 2:22),tw);}
            if(!tiny && !compact)caption_text_tight(dock,ND_TEXT,item->label,x,y+th-9,tw);
            draw_plugin_running_indicator(dock,item,x,y,tw);
            return 1;
        }
    }
    return 0;
}

static void draw_tile_content(struct NDDock *dock, int index)
{
    struct NDItem *item = &dock->conf.items[index];
    int x, y, tw = dock->tile_width, th = dock->tile_height;
    int baseline1, baseline2, meter_w;
    char compact_value[32];
    const char *tile_value=item->value;
    if(dock->hover_headroom && !dock->painting_shelf && !dock->painting_wave){
        dock->dirty_tiles[index]=1;
        if(!refresh_shelf_tiles(dock))draw_all(dock);
        return;
    }
    if(item->kind==ND_ITEM_DRAWER && item->volume_show_free && item->show_volume_meter && item->volume_total && item->volume_block_bytes)tile_value=volume_label(item);
    dock->graph_view[index].valid=0;   /* set again by the plain graph layout */
    if (!dock->window || !item_visible(dock,index)) return;
    /* A direct hover/full paint also consumes any queued content update. */
    dock->dirty_tiles[index]=0;
    dock->graph_step[index]=0;
    ++dock->diagnostic_tile_paints;
    dock->painting_item=index;
    dock->caption_dirty=1;
    tile_coords(dock, index, &x, &y);
    tile_dimensions(dock,index,&tw,&th);
    if (item->kind==ND_ITEM_SPACER) {
        int hovered=index==dock->hover;
        int engaged=dock->press_item==index ||
                    (dock->is_drag && dock->drag_to==index);
        int active=hovered || engaged;
        int color=ND_SUBTLE;
        int thickness=item->spacer_line_width;
        /* A spacer is negative space inside one continuous dock skin, not a
         * black tile.  Repaint its slice of the global backdrop first. */
        paint_background(dock,x,y,tw,th);
        if (active) {
            struct RastPort *rp=dock->window->RPort;
            /* Keep empty spacers as visual breathing room.  Hover uses a
             * quiet dotted outline; only an actual edit/drag uses Accent. */
            SetAPen(rp,pen(dock,engaged ? ND_ACCENT : ND_SUBTLE));
            SetDrPt(rp,engaged ? 0xCCCCU : 0x8888U);
            Move(rp,x+2,y+2);Draw(rp,x+tw-3,y+2);
            Draw(rp,x+tw-3,y+th-3);Draw(rp,x+2,y+th-3);
            Draw(rp,x+2,y+2);SetDrPt(rp,0xFFFFU);
        }
        if (item->spacer_line) {
            if (item->spacer_line_color==ND_SPACER_COLOR_ACCENT) color=ND_ACCENT;
            else if (item->spacer_line_color==ND_SPACER_COLOR_SECONDARY)
                color=ND_ACCENT2;
            else if (item->spacer_line_color==ND_SPACER_COLOR_TEXT) color=ND_TEXT;
            if (active) color=ND_ACCENT;
            if (thickness<1 || thickness>8) thickness=2;
            if (vertical(dock) && tw>20)
                fill(dock,color,x+10,y+(th-thickness)/2,tw-20,thickness);
            else if (!vertical(dock) && th>20)
                fill(dock,color,x+(tw-thickness)/2,y+10,thickness,th-20);
        }
        return;
    }
    tile_box(dock,index == dock->hover && !(!dock->edit_mode &&
        !strcmp(item->id,"MUSIC") && item->media_capabilities) ? ND_HOVER:ND_TILE,x,y,tw,th);
    /* Live music controls own their feedback; tile outlines are for editing
     * or whole-tile actions, not button/slider gestures. */
    if ((dock->press_item == index || (dock->is_drag && dock->drag_to == index)) &&
        (dock->edit_mode || strcmp(item->id,"MUSIC") || !item->media_capabilities)) {
        struct RastPort *rp = dock->window->RPort;
        SetAPen(rp, pen(dock, ND_ACCENT));
        SetDrPt(rp, dock->is_drag ? 0xCCCCU : 0xFFFFU);
        Move(rp, x+1, y+1); Draw(rp, x+tw-2, y+1);
        Draw(rp, x+tw-2, y+th-2); Draw(rp, x+1, y+th-2);
        Draw(rp, x+1, y+1); SetDrPt(rp, 0xFFFFU);
    }
    if ((item->kind == ND_ITEM_LAUNCH || item->kind == ND_ITEM_DRAWER ||
         item->kind == ND_ITEM_GROUP) &&
        dock->brushes[index] &&
        draw_brush(dock, index, x, y, tw, th)) {
        if (tile_label_visible(dock,index))
            caption_text_normal(dock, ND_TEXT, volume_label(item), x, y + th - 9, tw);
        draw_volume_meter(dock, item, x, y, tw, th);
        draw_running_indicator(dock,index,x,y,tw);
        return;
    }
    if ((item->kind == ND_ITEM_LAUNCH || item->kind == ND_ITEM_DRAWER ||
         item->kind == ND_ITEM_GROUP) &&
        dock->scaled_icon_bitmaps[index] && dock->scaled_icon_masks[index]) {
        struct BitMap *art,*art_mask;
        int iw,ih;
        (void)tile_icon_art(dock,index,&art,&art_mask,&iw,&ih);
        draw_cached_icon(dock,index,x,y,tw,th,art,art_mask,iw,ih);
        if (tile_label_visible(dock,index))
            caption_text_normal(dock,ND_TEXT,volume_label(item),x,y+th-9,tw);
        draw_volume_meter(dock, item, x, y, tw, th);
        draw_running_indicator(dock,index,x,y,tw);
        return;
    }
    if ((item->kind == ND_ITEM_LAUNCH || item->kind == ND_ITEM_DRAWER ||
         item->kind == ND_ITEM_GROUP) &&
        dock->icons[index]) {
        struct Image *im = (struct Image *)dock->icons[index]->do_Gadget.GadgetRender;
        int tiny=dock->conf.item_size==ND_ITEM_SIZE_TINY;
        int max_width=tw-(tiny ? 2 : 14);
        int max_height=th-(tiny ? 2 : 27);
        /* Never ask icon.library to render into the Workbench screen here.
         * Its native ColorIcon path may temporarily remap/allocate shared
         * pens on every repaint. ColorIcons are decoded into our cached
         * bitmap above; this path is only the pen-free planar fallback. */
        if (im && im->Depth >= 1 && im->Depth <= 4 &&
            im->Width > 0 && im->Height > 0 &&
            im->Width <= max_width && im->Height <= max_height) {
            int iy=nd_icon_art_top(y,th,im->Height,tiny);
            DrawImage(dock->window->RPort, im,
                      x + (tw - im->Width) / 2, iy);
            if (tile_label_visible(dock,index))
                caption_text_normal(dock, ND_TEXT, volume_label(item), x, y + th - 9, tw);
            draw_volume_meter(dock, item, x, y, tw, th);
            draw_running_indicator(dock,index,x,y,tw);
            return;
        }
    }

    if(item->kind==ND_ITEM_PLUGIN && (!strcmp(item->id,"AMINET") || nd_actions_plugin_path(item->command))){
        draw_plugin_launcher_artwork(dock,index,item->label,x,y,tw,th);return;
    }
    if(item->kind==ND_ITEM_PLUGIN&&item->online&&dock->thumbnails[index]){
        int tiny=dock->conf.item_size==ND_ITEM_SIZE_TINY,side=tiny ? 16:32;struct BitMap *image=thumbnail_bitmap(dock,index,tiny);
        if(image){BltBitMapRastPort(image,0,0,dock->window->RPort,x+(tw-side)/2,nd_icon_art_top(y,th,side,tiny),side,side,0xC0);if(!tiny)caption_text_normal(dock,ND_TEXT,item->label,x,y+th-9,tw);(void)draw_plugin_badge(dock,item,x,y,tw);return;}
    }
    if(draw_plugin_bank(dock,index,item,x,y,tw,th))return;
    if(item->kind==ND_ITEM_PLUGIN && nd_shelf_plugin_path(item->command)){draw_plugin_launcher_artwork(dock,index,item->label,x,y,tw,th);(void)draw_plugin_badge(dock,item,x,y,tw);return;}
    if(item->kind==ND_ITEM_PLUGIN && nd_collection_plugin_path(item->command)){
        const char *root=nd_option_get(&dock->conf,item->id,"DRAWER");
        if(root && *root && nd_whd_folder_code(item->value)==ND_WHD_FOLDER_OK &&
           (dock->brushes[index] || dock->icons[index] || dock->scaled_icon_bitmaps[index])){
            draw_plugin_launcher_artwork(dock,index,item->label,x,y,tw,th);
            (void)draw_plugin_badge(dock,item,x,y,tw);
            return;
        }
    }

    if(item->kind==ND_ITEM_PLUGIN && !strcmp(item->id,"SCREENSWITCHER")){
        draw_plugin_launcher_artwork(dock,index,"SCREENS",x,y,tw,th);return;
    }

    if (item->kind==ND_ITEM_PLUGIN && !strcmp(item->id,"DOPUS4")) {
        draw_dopus_tile(dock,index,item,x,y,tw,th);
        return;
    }
    if(item->kind==ND_ITEM_PLUGIN && (!strcmp(item->id,"NET") || !strcmp(item->id,"ROADSHOW")) && strcmp(item->badge,"*") && !metric_kind(dock,item)){
        draw_plugin_launcher_artwork(dock,index,"NETWORK",x,y,tw,th);return;
    }
    if(item->kind==ND_ITEM_PLUGIN && !strcmp(item->id,"BITSKIM") &&
       (!strcmp(item->value,"OFFLINE") || !strcmp(item->value,"RECEIVING OFF") || strstr(item->value," DEVICE"))){
        draw_plugin_launcher_artwork(dock,index,"BITSKIM",x,y,tw,th);
        draw_plugin_running_indicator(dock,item,x,y,tw);return;
    }
    if(item->kind==ND_ITEM_PLUGIN && !strcmp(item->id,"CLIPBOARD")){
        draw_plugin_launcher_artwork(dock,index,"CLIPPY",x,y,tw,th);
        draw_plugin_running_indicator(dock,item,x,y,tw);return;
    }
    if (item->kind==ND_ITEM_PLUGIN && !strcmp(item->id,"RUNNINGAPPS")) {
        draw_running_apps_tile(dock,index,x,y,tw,th);
        return;
    }
    if (item->kind==ND_ITEM_PLUGIN && !strcmp(item->id,"MUSIC") &&
        !item->media_capabilities) {
        draw_music_idle_tile(dock,index,x,y,tw,th);
        draw_plugin_running_indicator(dock,item,x,y,tw);
        return;
    }
    {int kind=metric_kind(dock,item);
     if(kind){draw_metric_tile(dock,index,kind,x,y,tw,th);
        draw_plugin_running_indicator(dock,item,x,y,tw);return;}}
    if (item->kind == ND_ITEM_PLUGIN && !strcmp(item->id,"EYES")) {
        draw_eyes(dock,index,x,y,tw,th);
        draw_plugin_running_indicator(dock,item,x,y,tw);
        return;
    }
    if (item->kind==ND_ITEM_PLUGIN &&
        dock->conf.item_size==ND_ITEM_SIZE_TINY) {
        draw_tiny_plugin(dock,item,x,y,tw,th);
        draw_plugin_running_indicator(dock,item,x,y,tw);
        return;
    }
    if (item->kind == ND_ITEM_PLUGIN && !strcmp(item->id,"CLOCK") &&
        DOCK_OPTS(dock)->clock_analog) {
        draw_analog_clock(dock,x,y,tw,th);
        draw_plugin_running_indicator(dock,item,x,y,tw);
        return;
    }
    if(item->kind==ND_ITEM_PLUGIN && dock->conf.item_size!=ND_ITEM_SIZE_TINY){
        int reserve=dock->window->RPort->TxHeight+8;
        if(th-reserve>=26)th-=reserve;
    }
    if (item->kind==ND_ITEM_PLUGIN && !strcmp(item->id,"RAM")) {
        if (item->graph) draw_ram_graph_tile(dock,index,x,y,tw,th);
        else draw_ram_tile(dock,item,x,y,tw,th);
        draw_plugin_running_indicator(dock,item,x,y,tw);
        return;
    }
    if (item->kind==ND_ITEM_PLUGIN && !strcmp(item->id,"MUSIC")) {
        draw_music_tile(dock,item,x,y,tw,th);
        draw_plugin_running_indicator(dock,item,x,y,tw);
        return;
    }
    if(item->kind==ND_ITEM_PLUGIN && !item->graph &&
        (!strcmp(item->id,"NET") || !strcmp(item->id,"ROADSHOW"))){
        draw_network_meters(dock,index,x,y,tw,th);
        draw_plugin_running_indicator(dock,item,x,y,tw);
        return;
    }
    if (item->kind == ND_ITEM_PLUGIN && item->graph &&
        strcmp(item->id,"DOPUS4") && strcmp(item->id,"ARCHIVE")) {
        graph_tile_text(dock,item,x,y,tw,th);
        draw_graph(dock, index, x, y, tw, th);
        dock->graph_view[index].valid=1;
        draw_plugin_running_indicator(dock,item,x,y,tw);
        return;
    }
    if(item->kind==ND_ITEM_PLUGIN && !strcmp(item->id,"BITSKIM") &&
       item->meter<=100U &&
       (!strncmp(item->value,"SEND ",5) || !strncmp(item->value,"RECV ",5))){
        sprintf(compact_value,"%s %u%%",item->value[0]=='S' ? "SEND":"RECV",
                (unsigned int)item->meter);
        tile_value=compact_value;
    }
    if (item->kind==ND_ITEM_PLUGIN &&
        (!strcmp(item->id,"CPU") || !strcmp(item->id,"BITSKIM"))) {
        /* CPU and RAM use the same heading row as the compact Clock, Music
         * and status tiles; the meter remains anchored at the bottom. */
        baseline1=y+(th>=60 ? 24 : 17);
        baseline2=y+(th>=60 ? 44 : 34);
    } else if (item->kind==ND_ITEM_PLUGIN && item->meter<=100U) {
        /* Meter tiles share RAM's heading/value baselines. */
        baseline1=y+(th>=60 ? 13 : 10);
        baseline2=y+(th>=60 ? 28 : 20);
    } else {
        baseline1 = y + (th >= 60 ? 24 : 17);
        baseline2 = y + (th >= 60 ? 44 : 34);
    }
    /* The footer is the configured tile label; a live plugin heading is
     * content (for example the currently selected sensor), never a caption
     * that label hiding or magnification may suppress. */
    if(item->kind==ND_ITEM_PLUGIN && item->caption[0] &&
       strncmp(item->caption,"ART:",4) && strcmp(item->caption,item->label))
        label_text(dock,ND_SUBTLE,item->caption,x,baseline1,tw);
    caption_text_normal(dock,ND_SUBTLE,item->label,x,baseline1,tw);
    if(item->kind==ND_ITEM_PLUGIN || dock->painting_shelf || tile_label_visible(dock,index))
        centered_text(dock,item->online || item->kind==ND_ITEM_LAUNCH ||
            item->kind==ND_ITEM_DRAWER || item->kind==ND_ITEM_GROUP ? ND_TEXT:ND_SUBTLE,
            tile_value,x,baseline2,tw);
    if (item->kind == ND_ITEM_PLUGIN && item->meter <= 100U) {
        meter_w = tw - 20;
        fill(dock, ND_BAR_BG, x + 10, y + th - 13, meter_w, 4);
        if (item->meter > 0)
            fill(dock, ND_ACCENT, x + 10, y + th - 13,
                 (int)(item->meter * (ULONG)meter_w / 100UL), 4);
    } else if (nd_item_supports_space_meter(item) && item->show_volume_meter &&
               item->meter <= 100U) {
        meter_w = tw - 20;
        fill(dock, ND_BAR_BG, x + 10, y + th - 13, meter_w, 4);
        if (item->meter > 0)
            fill(dock, item->volume_low_notified ? ND_ACCENT2:ND_ACCENT, x + 10, y + th - 13,
                 (int)(item->meter * (ULONG)meter_w / 100UL), 4);
    } else if (item->kind == ND_ITEM_LAUNCH ||
               item->kind == ND_ITEM_DRAWER || item->kind == ND_ITEM_GROUP) {
        fill(dock, ND_ACCENT, x + tw / 2 - 6, y + th - 12, 12, 2);
    }
    draw_running_indicator(dock,index,x,y,tw);
    if (item->kind==ND_ITEM_PLUGIN)
        draw_plugin_running_indicator(dock,item,x,y,tw);
}
static void draw_scroll_control(struct NDDock *dock, int direction)
{
    int slot=direction<0 ? 0 : dock->layout_slots-1;
    int x,y,enabled;
    const char *arrow;
    if (!dock->overflow || slot<0) return;
    enabled=direction<0 ? dock->view_start>0 :
            dock->view_start+dock->visible_count<dock->conf.count;
    arrow=vertical(dock) ? (direction<0 ? "^^" : "vv") :
                           (direction<0 ? "<<" : ">>");
    slot_coords(dock,slot,&x,&y);
    rounded_box(dock,enabled ? ND_TILE : ND_BG,
                x,y,dock->tile_width,dock->tile_height);
    fill(dock,enabled ? ND_ACCENT : ND_SUBTLE,x+6,y+6,3,10);
    centered_text(dock,enabled ? ND_TEXT : ND_SUBTLE,arrow,x,
                  y+dock->tile_height/2+3,dock->tile_width);
    if (direction<0)
        centered_text(dock,ND_SUBTLE,"PREV",x,y+dock->tile_height-9,
                      dock->tile_width);
    else
        centered_text(dock,ND_SUBTLE,"NEXT",x,y+dock->tile_height-9,
                      dock->tile_width);
}
static void draw_insertion_marker(struct NDDock *dock)
{
    int x,y,w,h,index,after;
    if (!dock->edit_mode) return;
    if (dock->is_drag) { index=dock->drag_to;after=dock->drag_after; }
    else if (dock->drop_preview) {
        index=dock->drop_preview_item;after=dock->drop_preview_after;
    } else return;
    if (!item_visible(dock,index)) return;
    tile_coords(dock,index,&x,&y);
    tile_dimensions(dock,index,&w,&h);
    if(dock->hover_headroom && dock->hover_plan_ready){
        const struct NDRect *box=&dock->hover_rects[index];
        x=box->left;y=box->top;w=box->width;h=box->height;
    }
    if(dock->window->RPort->Layer){x-=dock_view_x(dock)-dock->window->RPort->Layer->Scroll_X;y-=dock_view_y(dock)-dock->window->RPort->Layer->Scroll_Y;}
    if (vertical(dock))
        fill(dock,ND_ACCENT,x+8,after ? y+h : y-3,w-16,3);
    else
        fill(dock,ND_ACCENT,after ? x+w : x-3,y+8,3,h-16);
}

static void draw_tab(struct NDDock *dock)
{
    struct RastPort *rp = dock->window->RPort;
    int w = dock_width(dock), h = dock_height(dock);
    int cx = w / 2, cy = h / 2;
    if (w <= 1 || h <= 1) return;
    SetDrPt(rp, 0xFFFFU);
    paint_background(dock, 0, 0, w, h);
    SetAPen(rp, pen(dock, ND_ACCENT));
    Move(rp, 0, 0); Draw(rp, w-1, 0); Draw(rp, w-1, h-1);
    Draw(rp, 0, h-1); Draw(rp, 0, 0);
    if (w > h) fill(dock, ND_ACCENT, cx-11, cy-1, 22, 3);
    else fill(dock, ND_ACCENT, cx-1, cy-11, 3, 22);
}
static int wave_widget(const struct NDItem *item)
{return item->kind==ND_ITEM_PLUGIN;}
void draw_tile(struct NDDock *dock,int index)
{
    int x,y,w,h;
    if(!dock->window || !item_visible(dock,index))return;
    if(dock->hover_headroom && !dock->painting_shelf && !dock->painting_wave){
        dock->dirty_tiles[index]=1;
        if(!refresh_shelf_tiles(dock))draw_all(dock);
        return;
    }
    draw_tile_content(dock,index);
    tile_coords(dock,index,&x,&y);tile_dimensions(dock,index,&w,&h);
    draw_plugin_state(dock,index,x,y,w);
}

static int widget_wave_box(struct NDDock *dock,int index,int *w,int *h)
{
    const struct NDHoverLayoutItem *p=&dock->hover_layout[index];
    if(!dock->hover_plan_ready || (p->kind!=ND_HOVER_WIDGET && !metric_kind(dock,&dock->conf.items[index])))return 0;
    *w=p->width;*h=p->height;return 1;
}
static struct NDHoverWidget *widget_buffer(struct NDDock *dock,int index)
{
    struct NDHoverWidget *cache=&dock->hover_widgets[index];
    int capacity_width,capacity_height;
    ULONG needed=0;
    if(!widget_wave_box(dock,index,&capacity_width,&capacity_height) ||
       capacity_width<1 || capacity_height<1 || capacity_width>ND_HOVER_WIDGET_LIMIT ||
       capacity_height>ND_HOVER_WIDGET_LIMIT)return NULL;
    capacity_width=(capacity_width+31)/32*32;capacity_height=(capacity_height+31)/32*32;
    if(capacity_width<32)capacity_width=32;if(capacity_height<32)capacity_height=32;
    if(cache->bitmap && cache->capacity_width>capacity_width)capacity_width=cache->capacity_width;
    if(cache->bitmap && cache->capacity_height>capacity_height)capacity_height=cache->capacity_height;
    if(capacity_width>ND_HOVER_WIDGET_LIMIT)capacity_width=ND_HOVER_WIDGET_LIMIT;
    if(capacity_height>ND_HOVER_WIDGET_LIMIT)capacity_height=ND_HOVER_WIDGET_LIMIT;
    if(!dock->hover_widget_mask)return NULL;
    if((cache->bitmap || cache->mask) && (cache->capacity_width<capacity_width ||
                        cache->capacity_height<capacity_height)){
        WaitBlit();if(cache->bitmap)FreeBitMap(cache->bitmap);cache->bitmap=NULL;
        if(cache->mask)FreeBitMap(cache->mask);cache->mask=NULL;
    }
    if(!cache->bitmap)needed+=(ULONG)((capacity_width+15)/16)*2UL*(ULONG)capacity_height*dock->screen->RastPort.BitMap->Depth;
    if(!cache->mask)needed+=(ULONG)((capacity_width+15)/16)*2UL*(ULONG)capacity_height;
    if(needed && !hover_cache_reserve(dock,needed,index,-1))return NULL;
    cache->used=++dock->hover_cache_clock;
    if(!cache->bitmap){
        cache->bitmap=AllocBitMap(capacity_width,capacity_height,
            dock->screen->RastPort.BitMap->Depth,BMF_CLEAR,dock->screen->RastPort.BitMap);
        cache->capacity_width=capacity_width;cache->capacity_height=capacity_height;
        cache->serial=0;
    }
    if(!cache->mask){
        cache->mask=AllocBitMap(capacity_width,capacity_height,1,BMF_CLEAR,NULL);
        cache->serial=0;
    }
    return cache->bitmap && cache->mask ? cache:NULL;
}
static void mask_widget_caption(struct NDDock *dock,struct NDHoverWidget *cache,
                                 int index,int x,int y,int tw,int th,int w,int h)
{
    const struct NDCaption *caption=&dock->hover_captions[index];
    struct RastPort mask,*rp=dock->window->RPort;
    int len=(int)strlen(caption->text),extent,left,top,right,bottom;
    if(!caption->width || !len)return;
    /* Clear the caption band, including abbreviations and ellipses. */
    extent=caption->width;
    left=caption->x-x-1;
    top=caption->baseline-rp->TxBaseline-y;
    right=((left+extent+2)*w+tw-1)/tw-1;
    bottom=((top+rp->TxHeight)*h+th-1)/th-1;
    left=left*w/tw;top=top*h/th;
    if(left<0)left=0;if(top<0)top=0;
    if(right>=w)right=w-1;if(bottom>=h)bottom=h-1;
    if(right<left || bottom<top)return;
    InitRastPort(&mask);mask.BitMap=cache->mask;SetAPen(&mask,0);
    RectFill(&mask,left,top,right,bottom);
}
static void widget_content_bounds(struct NDHoverWidget *cache,int width,int height)
{
    int x,y,left=width,top=height,right=-1,bottom=-1;
    const unsigned char *bits=(const unsigned char*)cache->mask->Planes[0];
    int bytes=(width+7)/8;
    WaitBlit();
    /* Scan whole bytes; only the first and last set byte of a row are
     * examined bit by bit (the mask is mostly empty or mostly solid). */
    for(y=0;y<height;++y){
        const unsigned char *row=bits+(ULONG)y*cache->mask->BytesPerRow;
        int first=0,last=bytes-1;
        unsigned char tail=(unsigned char)(width&7 ? 0xff<<(8-(width&7)):0xff);
        while(first<bytes && !(row[first]&(first==bytes-1 ? tail:0xff)))++first;
        if(first==bytes)continue;
        while(!(row[last]&(last==bytes-1 ? tail:0xff)))--last;
        for(x=first*8;!(row[x/8]&(128>>(x&7)));++x);
        if(x<left)left=x;
        for(x=last*8+7;x>=width || !(row[x/8]&(128>>(x&7)));--x);
        if(x>right)right=x;
        if(y<top)top=y;bottom=y;
    }
    cache->content_left=left;cache->content_top=top;
    cache->content_width=right>=left ? right-left+1:0;
    cache->content_height=bottom>=top ? bottom-top+1:0;
}
static int scale_planar_widget(struct BitScaleArgs *scale)
{
    struct BitMap *src=scale->bsa_SrcBitMap,*dst=scale->bsa_DestBitMap;
    if(!(GetBitMapAttr(src,BMA_FLAGS)&BMF_STANDARD) ||
       !(GetBitMapAttr(dst,BMA_FLAGS)&BMF_STANDARD) || src->Depth!=dst->Depth)return 0;
    WaitBlit();
    if(!nd_planar_scale((unsigned char *const *)src->Planes,src->BytesPerRow,src->Rows,
       (unsigned char *const *)dst->Planes,dst->BytesPerRow,dst->Rows,src->Depth,
       scale->bsa_SrcX,scale->bsa_SrcY,scale->bsa_SrcWidth,scale->bsa_SrcHeight,
       scale->bsa_XDestFactor,scale->bsa_YDestFactor))return 0;
    scale->bsa_DestWidth=scale->bsa_XDestFactor;
    scale->bsa_DestHeight=scale->bsa_YDestFactor;
    return 1;
}
static void draw_widget_wave(struct NDDock *dock,int index,int x,int y,int tw,int th)
{
    struct BitScaleArgs scale;
    struct NDHoverWidget *cache;
    int w=tw,h=th;
    if(!dock->hover_shelf_valid || !widget_wave_box(dock,index,&w,&h))return;
    (void)metric_source_box(dock,index,&x,&y,&tw,&th);
    cache=widget_buffer(dock,index);
    if(!cache){
        /* A failed optional cache must not hide the live widget entirely. */
        int fw=tw<w ? tw:w,fh=th<h ? th:h;
        struct NDRect *box=&dock->hover_rects[index];
        box->left+=(w-fw)/2;box->top+=(h-fh)/2;box->width=fw;box->height=fh;
        if(dock->hover_widget_mask)BltMaskBitMapRastPort(dock->hover_shelf,x,y,dock->window->RPort,
            box->left,box->top,fw,fh,0xe0,dock->hover_widget_mask->Planes[0]);
        return;
    }
    if(cache->width!=w || cache->height!=h || cache->serial!=dock->hover_shelf_serial){
        WaitBlit();
        memset(&scale,0,sizeof(scale));
        scale.bsa_SrcX=x;scale.bsa_SrcY=y;
        scale.bsa_SrcWidth=scale.bsa_XSrcFactor=tw;
        scale.bsa_SrcHeight=scale.bsa_YSrcFactor=th;
        scale.bsa_XDestFactor=w;scale.bsa_YDestFactor=h;
        scale.bsa_SrcBitMap=dock->hover_shelf;scale.bsa_DestBitMap=cache->bitmap;
        if(!scale_planar_widget(&scale))BitMapScale(&scale);
        if(scale.bsa_DestWidth!=w || scale.bsa_DestHeight!=h){cache->serial=0;return;}
        memset(&scale,0,sizeof(scale));
        scale.bsa_SrcX=x;scale.bsa_SrcY=y;
        scale.bsa_SrcWidth=scale.bsa_XSrcFactor=tw;
        scale.bsa_SrcHeight=scale.bsa_YSrcFactor=th;
        scale.bsa_XDestFactor=w;scale.bsa_YDestFactor=h;
        scale.bsa_SrcBitMap=dock->hover_widget_mask;scale.bsa_DestBitMap=cache->mask;
        if(!scale_planar_widget(&scale))BitMapScale(&scale);
        if(scale.bsa_DestWidth!=w || scale.bsa_DestHeight!=h){cache->serial=0;return;}
        mask_widget_caption(dock,cache,index,x,y,tw,th,w,h);
        widget_content_bounds(cache,w,h);
        cache->width=w;cache->height=h;cache->serial=dock->hover_shelf_serial;
    }
    BltMaskBitMapRastPort(cache->bitmap,0,0,dock->window->RPort,
        dock->hover_rects[index].left,dock->hover_rects[index].top,w,h,0xe0,cache->mask->Planes[0]);
}

static int hover_art_size(struct NDDock *dock,int index,int tw,int th,int *w,int *h)
{
    struct NDArtwork *a=dock->plugin_artwork[index];
    struct BitMap *bitmap,*mask;
    const struct NDItem *item=&dock->conf.items[index];
    if(!strcmp(item->id,"MUSIC") && item->media_capabilities)return 0;
    if(a){
        char art[24];unsigned int state,count,flags=0;const unsigned char *runs=NULL;
        int tiny=dock->conf.item_size==ND_ITEM_SIZE_TINY,reserve,maximum,size;
        if(dock->plugin_art_key[index])runs=nd_artwork_key_frame(a,dock->plugin_art_key[index],dock->plugin_art_state[index],&count,&flags);
        else if(nd_artwork_parse(item->caption,art,&state))runs=nd_artwork_key_frame(a,nd_artwork_key(art),state,&count,&flags);
        if(runs){
            if(flags&ND_ARTWORK_VALUE)return 0;
            reserve=tiny ? 4:24;maximum=48;
            size=tw-(a->data[5]==1 || (flags&ND_ARTWORK_LARGE) ? 12:tiny ? 4:16);
            if(size>th-reserve)size=th-reserve;if(size>maximum)size=maximum;
            if(size<1)return 0;*w=*h=size;return 1;
        }
    }
    if(dock->brush_bitmaps[index] && dock->brush_masks[index]){
        *w=dock->scaled_icon_width[index];*h=dock->scaled_icon_height[index];return 1;
    }
    return tile_icon_art(dock,index,&bitmap,&mask,w,h);
}
/* prepare_hover_layout() runs on every pointer move; measuring every label
 * with TextLength() each time dominated it with many tiles.  The width is
 * remembered per tile for its text and font. */
static int hover_label_width(struct NDDock *dock,int index,const char *label)
{
    struct NDLabelWidth *c=&dock->hover_label_width[index];
    const void *font=dock->window->RPort->Font;
    size_t n=strlen(label);
    if(n>=sizeof(c->text))n=sizeof(c->text)-1;
    if(c->font!=font || strncmp(c->text,label,n) || c->text[n]){
        c->width=TextLength(dock->window->RPort,(STRPTR)label,(int)strlen(label));
        memcpy(c->text,label,n);c->text[n]=0;c->font=font;
    }
    return c->width;
}
static void prepare_hover_layout(struct NDDock *dock)
{
    int i,v=vertical(dock),axis=v ? dock->height:dock->width;
    int active=dock->hover>=0 && !dock->is_drag;
    int focus_row=0,row_count=v ? 1:dock->layout_rows,row,extra_cross=0;
    struct NDHoverLayoutItem row_items[ND_MAX_ITEMS];
    int indices[ND_MAX_ITEMS];
    int label_height=dock->conf.item_size==ND_ITEM_SIZE_TINY ? 0:dock->window->RPort->TxHeight+3;
    int base_start=v ? dock->hover_pad_y:dock->hover_pad_x;
    int base_length=axis-2*dock->hover_axis_padding,start=base_start,end=base_start+base_length;
    int animating=0,pointer=dock->hover_pointer;
    if((dock->grip_drag || dock->music_press_action || dock->music_hover_action) && dock->hover_plan_ready)return;
    if(active && !v){int x,y;tile_coords(dock,dock->hover,&x,&y);
        focus_row=(y-dock->hover_pad_y-ND_BORDER)/(dock->tile_height+dock->gap);}
    dock->hover_plan_ready=0;
    for(i=0;i<dock->conf.count;++i){
        struct NDHoverLayoutItem *p=&dock->hover_layout[i];
        const struct NDItem *item=&dock->conf.items[i];
        int x,y,tw,th,distance,mw,mh,label_width;
        tile_coords(dock,i,&x,&y);tile_dimensions(dock,i,&tw,&th);
        memset(p,0,sizeof(*p));p->vertical=v;p->anchor=v ? y+th/2:x+tw/2;
        p->row=v ? 0:(y-dock->hover_pad_y-ND_BORDER)/(dock->tile_height+dock->gap);
        p->width=tw;p->height=th;
        if(metric_kind(dock,item)){
            int cx=x,cy=y;
            if(metric_source_box(dock,i,&cx,&cy,&p->width,&p->height))p->kind=ND_HOVER_ART;
        }else if(hover_art_size(dock,i,tw,th,&p->width,&p->height))p->kind=ND_HOVER_ART;
        else if(wave_widget(item))p->kind=ND_HOVER_WIDGET;
        p->base_width=p->width;p->base_height=p->height;p->centre=p->anchor;
        if(p->kind==ND_HOVER_ART && label_height){
            const char *label=item->kind==ND_ITEM_PLUGIN ? item->label:volume_label(&dock->conf.items[i]);
            label_width=hover_label_width(dock,i,label);
            if(label_width>tw-6)label_width=tw-6;
            p->label=v ? label_height:label_width;
        }
        if(active && p->row==focus_row && p->kind!=ND_HOVER_FIXED &&
           (dock->conf.hover_animation || i==dock->hover)){
            /* Without animation the hovered item uses the full configured
             * strength immediately, independent of pointer distance. */
            distance=dock->conf.hover_animation ? p->anchor-(dock->hover_pointer*4+2):0;
            mw=v ? tw+dock->hover_headroom-6:ND_HOVER_WIDGET_LIMIT;
            mh=v ? ND_HOVER_WIDGET_LIMIT:th+dock->hover_headroom-label_height-6;
            if(p->kind==ND_HOVER_ART){if(mw>ND_HOVER_ICON_LIMIT)mw=ND_HOVER_ICON_LIMIT;if(mh>ND_HOVER_ICON_LIMIT)mh=ND_HOVER_ICON_LIMIT;}
            nd_hover_size(distance,2*((v ? dock->tile_height:dock->tile_width)+dock->gap),
                dock->conf.hover_magnification,mw,mh,&p->width,&p->height);
        }
        /* Animated magnification: show the eased size, stepping toward this
         * target once per frame (draw_hover() sets hover_anim_advance). */
        if(i<ND_MAX_ITEMS && dock->conf.hover_animation && dock->hover_anim_valid &&
           p->kind!=ND_HOVER_FIXED){
            if(dock->hover_anim_advance){
                dock->hover_anim_w[i]=(short)nd_hover_ease(dock->hover_anim_w[i],p->width);
                dock->hover_anim_h[i]=(short)nd_hover_ease(dock->hover_anim_h[i],p->height);
            }
            if(dock->hover_anim_w[i]!=p->width || dock->hover_anim_h[i]!=p->height)animating=1;
            p->width=dock->hover_anim_w[i];p->height=dock->hover_anim_h[i];
        }else if(i<ND_MAX_ITEMS){dock->hover_anim_w[i]=(short)p->width;dock->hover_anim_h[i]=(short)p->height;}
        /* Artwork can replace a larger live widget between timer ticks.
         * Eased dimensions must obey the new renderer's storage limit too. */
        if(p->kind==ND_HOVER_ART){
            if(p->width>ND_HOVER_ICON_LIMIT)p->width=ND_HOVER_ICON_LIMIT;
            if(p->height>ND_HOVER_ICON_LIMIT)p->height=ND_HOVER_ICON_LIMIT;
            dock->hover_anim_w[i]=(short)p->width;dock->hover_anim_h[i]=(short)p->height;
        }
    }
    dock->hover_anim_valid=dock->conf.hover_animation!=0;
    dock->hover_animating=animating;
    /* While tiles are still shrinking after the pointer left, keep packing
     * them around the last focus so neighbours stay separated. */
    if(active){dock->hover_anim_row=focus_row;dock->hover_anim_pointer=dock->hover_pointer;}
    else if(animating){focus_row=dock->hover_anim_row;pointer=dock->hover_anim_pointer;active=1;}
    if(active)for(row=0;row<row_count;++row){
        int n=0;
        for(i=0;i<dock->conf.count;++i)if(dock->hover_layout[i].row==row){
            indices[n]=i;row_items[n]=dock->hover_layout[i];++n;
        }
        if(row==focus_row){
            if(dock->conf.hover_animation)
                nd_hover_layout_at(row_items,n,pointer*4+2,dock->gap,2+(dock->conf.drag_bar ? ND_DRAG_BAR_SIZE:0),axis-2);
            else {
                int focus=0;
                for(i=0;i<n;++i)if(indices[i]==dock->hover){focus=i;break;}
                nd_hover_layout(row_items,n,focus,dock->gap,2+(dock->conf.drag_bar ? ND_DRAG_BAR_SIZE:0),axis-2);
            }
            for(i=0;i<n;++i)dock->hover_layout[indices[i]]=row_items[i];
        }
    }
    for(i=0;i<dock->conf.count;++i){
        struct NDHoverLayoutItem *p=&dock->hover_layout[i];struct NDRect *box=&dock->hover_rects[i];
        int x,y,tw,th,normal_top;
        tile_coords(dock,i,&x,&y);tile_dimensions(dock,i,&tw,&th);
        box->width=p->width;box->height=p->height;
        if(v){
            box->top=p->centre-(p->height+(p->kind==ND_HOVER_ART ? p->label:0))/2;
            if(dock->conf.side==ND_SIDE_LEFT)box->left=x+(p->kind==ND_HOVER_ART ? (tw-p->base_width)/2:0);
            else box->left=x+tw-(p->kind==ND_HOVER_ART ? (tw-p->base_width)/2:0)-p->width;
        }else{
            box->left=p->centre-p->width/2;
            if(p->kind==ND_HOVER_WIDGET)box->top=dock->conf.side==ND_SIDE_BOTTOM ? y+th-p->height:y;
            else{
                normal_top=nd_icon_art_top(y,th,p->base_height,dock->conf.item_size==ND_ITEM_SIZE_TINY);
                box->top=dock->conf.side==ND_SIDE_BOTTOM ? y+th-(label_height ? 24:4)-p->height:normal_top;
            }
        }
        if(p->kind==ND_HOVER_FIXED){box->left=x;box->top=y;}
    }
    if(active && dock->conf.count){
        for(row=0;row<row_count;++row){
            int first=-1,last=-1;
            for(i=0;i<dock->conf.count;++i)if(dock->hover_layout[i].row==row){
                if(first<0 || dock->hover_layout[i].anchor<dock->hover_layout[first].anchor)first=i;
                if(last<0 || dock->hover_layout[i].anchor>dock->hover_layout[last].anchor)last=i;
            }
            if(first>=0){
                struct NDHoverLayoutItem *f=&dock->hover_layout[first],*l=&dock->hover_layout[last];
                int ef=v ? f->height-f->base_height:f->width-f->base_width;
                int el=v ? l->height-l->base_height:l->width-l->base_width;
                int left=base_start+f->centre-f->anchor-ef/2;
                int right=base_start+base_length+l->centre-l->anchor+(el+1)/2;
                if(left<start)start=left;if(right>end)end=right;
            }
        }
        if(start<0)start=0;if(end>axis)end=axis;
    }
    if(active && !v && row_count>1){
        int original_top=dock->hover_pad_y+ND_BORDER+focus_row*(dock->tile_height+dock->gap);
        int original_bottom=original_top+dock->tile_height;
        for(i=0;i<dock->conf.count;++i)if(dock->hover_layout[i].row==focus_row){
            const struct NDRect *box=&dock->hover_rects[i];
            int extra=dock->conf.side==ND_SIDE_BOTTOM ? original_top-box->top:
                box->top+box->height+(dock->hover_layout[i].kind==ND_HOVER_ART ? label_height:0)-original_bottom;
            if(extra>extra_cross)extra_cross=extra;
        }
        if(extra_cross>dock->hover_headroom)extra_cross=dock->hover_headroom;
        for(i=0;i<dock->conf.count;++i){
            struct NDHoverLayoutItem *p=&dock->hover_layout[i];
            if(dock->conf.side==ND_SIDE_BOTTOM && p->row<focus_row)p->cross_shift=-extra_cross;
            else if(dock->conf.side==ND_SIDE_TOP && p->row>focus_row)p->cross_shift=extra_cross;
            dock->hover_rects[i].top+=p->cross_shift;
        }
    }
    dock->hover_shelf_rect.left=v ? dock->hover_pad_x:start;
    dock->hover_shelf_rect.top=v ? start:dock->hover_pad_y-(dock->conf.side==ND_SIDE_BOTTOM ? extra_cross:0);
    dock->hover_shelf_rect.width=v ? dock->width-dock->hover_headroom:end-start;
    dock->hover_shelf_rect.height=v ? end-start:dock->height-dock->hover_headroom+extra_cross;
    dock->hover_plan_ready=1;
}
static void draw_wave_label(struct NDDock *dock,int index)
{
    const struct NDItem *item=&dock->conf.items[index];
    const struct NDRect *box=&dock->hover_rects[index];
    int x,y,w,h,baseline,centre=box->left+box->width/2;
    tile_coords(dock,index,&x,&y);tile_dimensions(dock,index,&w,&h);
    baseline=vertical(dock) ? box->top+box->height+dock->window->RPort->TxHeight+2:
        y+h-9+dock->hover_layout[index].cross_shift;
    if(!vertical(dock) && dock->conf.side==ND_SIDE_TOP)
        baseline+=box->height-dock->hover_layout[index].base_height;
    if(vertical(dock) && (!dock->conf.hover_magnification || dock->hover<0))baseline=y+h-9;
    if(tile_label_visible(dock,index) && !floating_caption(dock,index))
        label_text(dock,ND_TEXT,item->kind==ND_ITEM_PLUGIN ? item->label:volume_label(&dock->conf.items[index]),centre-(w-6)/2,baseline,w-6);
    if(item->kind==ND_ITEM_PLUGIN){
        draw_plugin_running_indicator(dock,item,centre-w/2,box->top,w);
        draw_plugin_state(dock,index,centre-w/2,box->top,w);
        if(!strcmp(item->id,"DOPUS4") && dopus_application_running(item))
            fill(dock,ND_ACCENT2,box->left+box->width-9,box->top+5,4,4);
    }
    else{
        draw_volume_meter(dock,item,centre-w/2,baseline-h+9,w,h);
        draw_running_indicator(dock,index,centre-w/2,baseline-h+9,w);
    }
}

/* Popup anchors follow opaque artwork pixels, not the reserved canvas or
 * a widget's empty margins. Bounds are recorded only when its cache changes. */
void visible_tile_rect(struct NDDock *dock,int index,struct NDRect *rect)
{
    int x,y,w,h;
    tile_coords(dock,index,&x,&y);tile_dimensions(dock,index,&w,&h);
    rect->left=x;rect->top=y;rect->width=w;rect->height=h;
    if(dock->hover_headroom && dock->hover_plan_ready){
        const struct NDHoverWidget *cache=&dock->hover_widgets[index];
        *rect=dock->hover_rects[index];
        if(cache->serial==dock->hover_shelf_serial && cache->content_width>0){
            rect->left+=cache->content_left;rect->top+=cache->content_top;
            rect->width=cache->content_width;rect->height=cache->content_height;
        }
    }else if(metric_source_box(dock,index,&x,&y,&w,&h) || hover_art_size(dock,index,w,h,&w,&h)){
        /* Metric coordinates are already cropped; other icons use their
         * normal common artwork box. */
        if(!metric_kind(dock,&dock->conf.items[index])){
            int tx,ty,tw,th;tile_coords(dock,index,&tx,&ty);tile_dimensions(dock,index,&tw,&th);
            x=tx+(tw-w)/2;y=nd_icon_art_top(ty,th,h,dock->conf.item_size==ND_ITEM_SIZE_TINY);
        }
        rect->left=x;rect->top=y;rect->width=w;rect->height=h;
    }
}
void close_caption_popup(struct NDDock *dock)
{
    if(dock->caption_popup){CloseWindow(dock->caption_popup);dock->caption_popup=NULL;}
    dock->caption_index=-1;dock->caption_text[0]=0;
}
void update_caption_popup(struct NDDock *dock)
{
    int index=dock->hover,x,y,w,h,len,tx,ty,tw,th;
    const char *name;struct RastPort *rp;
    if(!dock->window || !dock->screen || dock->hidden || dock->covered || dock->box_pending ||
       dock->is_drag || dock->popup || dock->calendar || dock->folder_popup || dock->about ||
       index<0 || index>=dock->conf.count || !floating_caption(dock,index)){
        close_caption_popup(dock);dock->caption_dirty=0;return;
    }
    if(!dock->caption_dirty && dock->caption_popup)return;
    dock->caption_dirty=0;
    name=dock->conf.items[index].label;
    len=(int)strlen(name);rp=dock->window->RPort;
    while(len && TextLength(rp,(STRPTR)name,len)>dock->screen->Width-16)--len;
    if(!len){close_caption_popup(dock);return;}
    w=TextLength(rp,(STRPTR)name,len)+12;h=rp->TxHeight+10;
    tile_coords(dock,index,&tx,&ty);tile_dimensions(dock,index,&tw,&th);
    if(dock->hover_headroom && dock->hover_plan_ready){
        struct NDRect box;visible_tile_rect(dock,index,&box);
        tx=box.left;ty=box.top;tw=box.width;th=box.height;
    }
    x=dock_left(dock)+tx+(tw-w)/2;y=dock_top(dock)+ty-h-4;
    if(y<dock->screen->BarHeight)y=dock_top(dock)+ty+th+4;
    if(x<0)x=0;if(x+w>dock->screen->Width)x=dock->screen->Width-w;
    if(y+h>dock->screen->Height)y=dock->screen->Height-h;
    if(dock->caption_popup && (!strncmp(dock->caption_text,name,(size_t)len) &&
       dock->caption_text[len]==0) && dock->caption_popup->Width==w && dock->caption_popup->Height==h){
        if(x!=dock->caption_x || y!=dock->caption_y){
            close_tooltip(dock);dock->hover_ticks=0;
            ChangeWindowBox(dock->caption_popup,x,y,w,h);
            dock->caption_x=x;dock->caption_y=y;
        }
        return;
    }
    close_caption_popup(dock);close_tooltip(dock);dock->hover_ticks=0;
    dock->caption_popup=OpenWindowTags(NULL,WA_PubScreen,(ULONG)dock->screen,
        WA_Left,x,WA_Top,y,WA_Width,w,WA_Height,h,WA_Borderless,TRUE,WA_Activate,FALSE,
        WA_Flags,WFLG_SMART_REFRESH,TAG_DONE);
    if(!dock->caption_popup)return;
    dock->caption_index=index;dock->caption_x=x;dock->caption_y=y;
    memcpy(dock->caption_text,name,(size_t)len);dock->caption_text[len]=0;
    rp=dock->caption_popup->RPort;
    SetFont(rp,dock->window->RPort->Font);
    SetSoftStyle(rp,dock->conf.font_mode==ND_FONT_BOLD ? FSF_BOLD:FS_NORMAL,0xff);
    SetAPen(rp,pen(dock,ND_BG));RectFill(rp,0,0,w-1,h-1);
    SetAPen(rp,pen(dock,ND_ACCENT));Move(rp,0,0);Draw(rp,w-1,0);Draw(rp,w-1,h-1);Draw(rp,0,h-1);Draw(rp,0,0);
    SetDrMd(rp,JAM1);SetAPen(rp,pen(dock,ND_TEXT));Move(rp,6,5+rp->TxBaseline);Text(rp,(STRPTR)name,len);
}

static void paint_hover_wave(struct NDDock *dock)
{
    int i,pass;
    dock->painting_wave=1;
    for(pass=0;pass<2;++pass)for(i=0;i<dock->conf.count;++i){
        struct BitMap *bitmap,*mask;
        int x,y,w,h,iw,ih;
        const struct NDItem *item=&dock->conf.items[i];
        /* The largest tile belongs in front of its neighbours. */
        if((i==dock->hover)!=(pass==1) || !item_visible(dock,i))continue;
        if(dock->wave_span_active && !dock->wave_draw[i])continue;
        tile_coords(dock,i,&x,&y);tile_dimensions(dock,i,&w,&h);
        if(metric_kind(dock,item)){
            draw_widget_wave(dock,i,x,y,w,h);draw_wave_label(dock,i);continue;
        }
        if(dock->hover_layout[i].kind==ND_HOVER_WIDGET){
            const struct NDRect *box=&dock->hover_rects[i];
            draw_widget_wave(dock,i,x,y,w,h);
            if(tile_label_visible(dock,i) && !floating_caption(dock,i))
                label_text(dock,ND_TEXT,item->label,box->left,box->top+box->height-9,box->width);
            draw_plugin_state(dock,i,box->left,box->top,box->width);
            continue;
        }
        if(dock->hover_layout[i].kind==ND_HOVER_FIXED){
            const struct NDRect *box=&dock->hover_rects[i];
            if(dock->hover_shelf_valid)
                BltBitMapRastPort(dock->hover_shelf,x,y,dock->window->RPort,box->left,box->top,w,h,0xc0);
            draw_fixed_caption(dock,i,box->left,box->top,x,y);
            x=box->left;y=box->top;
        }
        if(draw_plugin_bank(dock,i,item,x,y,w,h)){
            if(dock->hover_layout[i].kind==ND_HOVER_ART)draw_wave_label(dock,i);
            continue;
        }
        if(dock->brush_bitmaps[i] && dock->brush_masks[i])
            draw_cached_icon(dock,i,x,y,w,h,dock->brush_bitmaps[i],
                dock->brush_masks[i],dock->scaled_icon_width[i],dock->scaled_icon_height[i]);
        else if(tile_icon_art(dock,i,&bitmap,&mask,&iw,&ih))
            draw_cached_icon(dock,i,x,y,w,h,bitmap,mask,iw,ih);
        if(dock->hover_layout[i].kind==ND_HOVER_ART)draw_wave_label(dock,i);
    }
    draw_grip(dock);
    dock->painting_wave=0;
    draw_insertion_marker(dock);
}
/* The area paint_hover_wave() can touch for tile INDEX.  Labels, running
 * dots, meters and plugin states are centred on the artwork box but span the
 * full tile width (draw_wave_label()); vertical docks draw the label below
 * the box.  The normal tile area and a small margin are included as well. */
static void hover_tile_extent(struct NDDock *dock,int index,struct NDRect *ext)
{
    const struct NDRect *box=&dock->hover_rects[index];
    int x,y,w,h,left,top,right,bottom,centre=box->left+box->width/2;
    tile_coords(dock,index,&x,&y);tile_dimensions(dock,index,&w,&h);
    left=box->left;top=box->top;right=box->left+box->width;bottom=box->top+box->height;
    if(centre-w/2<left)left=centre-w/2;
    if(centre-w/2+w>right)right=centre-w/2+w;
    if(x<left)left=x;
    if(y<top)top=y;
    if(x+w>right)right=x+w;
    if(y+h>bottom)bottom=y+h;
    /* Vertical: the label baseline is TxHeight+2 below the box, with its
     * descenders and the volume meter (baseline+3..+4) underneath. */
    if(vertical(dock) && box->top+box->height+dock->window->RPort->TxHeight+10>bottom)
        bottom=box->top+box->height+dock->window->RPort->TxHeight+10;
    ext->left=left-ND_HOVER_EXT_MARGIN;ext->top=top-ND_HOVER_EXT_MARGIN;
    ext->width=right-left+2*ND_HOVER_EXT_MARGIN;ext->height=bottom-top+2*ND_HOVER_EXT_MARGIN;
}
/* Everything besides tile rectangles that changes how the frame composes. */
static void hover_frame_key(const struct NDDock *dock,long *key,int vx,int vy)
{
    /* Which tile is hovered changes only that tile and the previous one (they
     * are marked changed); whether any tile is hovered changes every label. */
    key[0]=(long)dock->hover_shelf_serial;key[1]=dock->hover>=0;
    key[2]=dock->music_hover_action;key[3]=dock->music_press_action;
    key[4]=dock->press_item;key[5]=dock->is_drag ? 1:0;key[6]=dock->edit_mode;
    key[7]=dock->conf.count;key[8]=dock->overflow ? dock->view_start:-1;
    key[9]=vx;key[10]=vy;key[11]=((long)dock->width<<16)^(long)dock->height;
}
/* Only the low-memory direct-paint fallback uses a temporary translation.
 * Keep the layer locked and restore zero before Intuition can move/resize it. */
static int begin_canvas_paint(struct NDDock *dock)
{
    struct Layer *layer=dock->window->RPort->Layer;
    int x=dock_view_x(dock),y=dock_view_y(dock);
    if(!LayersBase||!layer||(!x&&!y))return 0;
    LockLayer(0,layer);ScrollLayer(0,layer,x,y);return 1;
}
static void end_canvas_paint(struct NDDock *dock)
{
    struct Layer *layer=dock->window->RPort->Layer;
    ScrollLayer(0,layer,-dock_view_x(dock),-dock_view_y(dock));UnlockLayer(layer);
}
void draw_hover(struct NDDock *dock)
{
    ULONG frame_sec=0,frame_micro=0,end_sec=0,end_micro=0,cost;
    ND_TRACE_START(render_start);
    if(!dock->window || dock->hidden || dock->box_pending)return;
    if(!dock->hover_shelf_valid){draw_all(dock);return;}
    CurrentTime(&frame_sec,&frame_micro);
    /* One blit restores the shelf, labels and exposed desktop. Mouse motion
     * never re-renders plugin values/graphs or the repeated background strips. */
    if(!dock->hover_box_paint)dock->hover_pointer=nd_hover_axis(vertical(dock) ? dock->screen->MouseY-dock_top(dock):
        dock->screen->MouseX-dock_left(dock),dock->hover_pointer);
    dock->hover_anim_advance=!dock->hover_box_paint;prepare_hover_layout(dock);dock->hover_anim_advance=0;
    if(!sync_dock_viewport(dock))return;
    {
        struct Window *real=dock->window,target;
        struct RastPort rp;
        long key[12];
        int i,lo=0,hi=0,partial=0,direct=0,v=vertical(dock);
        int vx=dock_view_x(dock),vy=dock_view_y(dock);
        if(dock->hover_frame && (dock->hover_frame_width!=dock->width || dock->hover_frame_height!=dock->height)){
            WaitBlit();FreeBitMap(dock->hover_frame);dock->hover_frame=NULL;
        }
        if(!dock->hover_frame)dock->hover_prev_valid=0;
        if(!dock->hover_frame){
            dock->hover_frame=AllocBitMap(dock->width,dock->height,
                dock->screen->RastPort.BitMap->Depth,BMF_CLEAR,dock->screen->RastPort.BitMap);
            dock->hover_frame_width=dock->width;dock->hover_frame_height=dock->height;
        }
        hover_frame_key(dock,key,vx,vy);
        for(i=0;i<dock->conf.count && i<ND_MAX_ITEMS;++i){
            hover_tile_extent(dock,i,&dock->hover_ext[i]);
            dock->hover_visible[i]=(unsigned char)(item_visible(dock,i)!=0);
        }
        /* The newly hovered tile is drawn last and loses its label to the
         * floating caption; the previous one returns to normal order. */
        if(dock->hover_prev_valid && dock->hover_prev_index!=dock->hover){
            if(dock->hover_prev_index>=0 && dock->hover_prev_index<ND_MAX_ITEMS)
                dock->hover_tile_changed[dock->hover_prev_index]=1;
            if(dock->hover>=0 && dock->hover<ND_MAX_ITEMS)dock->hover_tile_changed[dock->hover]=1;
        }
        /* A retained frame lets an unchanged tile keep its pixels: only the
         * span touched by a moved, resized or repainted tile - including its
         * label and badges - is recomposed and presented (nd_hover_dirty.h). */
        if(dock->hover_frame && dock->hover_prev_valid && !memcmp(key,dock->hover_prev_key,sizeof(key))){
            if(!nd_hover_dirty_span(dock->hover_prev_rects,dock->hover_rects,
                   dock->hover_prev_ext,dock->hover_ext,dock->hover_tile_changed,dock->hover_visible,
                   dock->conf.count<ND_MAX_ITEMS ? dock->conf.count:ND_MAX_ITEMS,v,&dock->hover_prev_shelf,&dock->hover_shelf_rect,
                   v ? dock->height:dock->width,&lo,&hi)){
                ++dock->diagnostic_hover_skipped;
                dock->caption_dirty=1;
                ND_TRACE_END(NDT_HOVER,render_start);
                return;
            }
            partial=1;
        }
        if(dock->hover_frame){
            WaitBlit();rp=*real->RPort;rp.Layer=NULL;rp.BitMap=dock->hover_frame;
            target=*real;target.RPort=&rp;dock->window=&target;
        }
        if(!dock->hover_frame)direct=begin_canvas_paint(dock);
        if(partial){
            if(v)paint_background(dock,0,lo,dock->width,hi-lo);
            else paint_background(dock,lo,0,hi-lo,dock->height);
            ++dock->diagnostic_hover_partial;
        }else{
            paint_background(dock,0,0,dock->width,dock->height);
            ++dock->diagnostic_hover_full;
        }
        /* Tiles wholly outside the span keep their pixels: skip them. */
        dock->wave_span_active=partial;dock->wave_span_lo=lo;dock->wave_span_hi=hi;
        if(partial)(void)nd_hover_draw_set(dock->hover_ext,dock->hover_visible,
                                           dock->conf.count<ND_MAX_ITEMS ? dock->conf.count:ND_MAX_ITEMS,
                                           v,lo,hi,dock->hover,dock->wave_draw);
        paint_hover_wave(dock);
        dock->wave_span_active=0;
        dock->window=real;
        if(direct)end_canvas_paint(dock);
        if(dock->hover_frame){
            int x0=vx,y0=vy,x1=vx+real->Width,y1=vy+real->Height;
            if(partial){
                /* The leading grip is drawn independently of the tile span.
                 * Present its cleared former positions as well, including
                 * after a reload or a frame clipped by a box transition. */
                if(v){if(!dock->conf.drag_bar && lo>y0)y0=lo;if(hi<y1)y1=hi;}
                else{if(!dock->conf.drag_bar && lo>x0)x0=lo;if(hi<x1)x1=hi;}
            }
            if(x1>x0 && y1>y0)
                BltBitMapRastPort(dock->hover_frame,x0,y0,real->RPort,x0-vx,y0-vy,x1-x0,y1-y0,0xc0);
            memcpy(dock->hover_prev_key,key,sizeof(key));
            memcpy(dock->hover_prev_rects,dock->hover_rects,sizeof(dock->hover_prev_rects));
            memcpy(dock->hover_prev_ext,dock->hover_ext,sizeof(dock->hover_prev_ext));
            dock->hover_prev_index=dock->hover;
            dock->hover_prev_shelf=dock->hover_shelf_rect;
            memset(dock->hover_tile_changed,0,sizeof(dock->hover_tile_changed));
            dock->hover_prev_valid=1;
        }else dock->hover_prev_valid=0;
    }
    dock->caption_dirty=1;
    CurrentTime(&end_sec,&end_micro);
    if((end_sec>frame_sec || (end_sec==frame_sec && end_micro>=frame_micro)) && end_sec-frame_sec<60UL){
        cost=(end_sec-frame_sec)*1000000UL+end_micro-frame_micro;
        dock->frame_average_us=nd_frame_average(dock->frame_average_us,cost);
        if(cost>dock->frame_peak_us)dock->frame_peak_us=cost;
        ++dock->frame_samples;
    }
    if(dock->hover_animating)kick_animation_timer(dock);
    ND_TRACE_END(NDT_HOVER,render_start);
}

/* Magnification keeps every tile in an off-screen shelf that draw_hover()
 * scales from.  A content update (clock tick, meter, graph step) repaints
 * only the changed tiles into the shelf and their widget mask, and drops
 * only their scaled widget caches, instead of a full draw_all() that
 * re-renders every tile and rebuilds every cache.  Returns 0 when a full
 * paint is needed (no valid shelf, size change, edit/drag decorations). */
static int refresh_shelf_tiles(struct NDDock *dock)
{
    struct Window *real=dock->window,target;
    struct RastPort port;
    unsigned char repainted[ND_MAX_ITEMS];
    int i,any=0;
    if(!real || dock->hidden || dock->box_pending || !dock->hover_shelf_valid ||
       !dock->hover_shelf || dock->hover_shelf_width!=dock->width ||
       dock->hover_shelf_height!=dock->height || dock->edit_mode || dock->is_drag ||
       dock->painting_shelf || dock->painting_wave || dock->painting_mask)return 0;
    memset(repainted,0,sizeof(repainted));
    WaitBlit();
    port=*real->RPort;port.Layer=NULL;port.BitMap=dock->hover_shelf;
    target=*real;target.RPort=&port;dock->window=&target;
    dock->painting_shelf=1;
    for(i=0;i<dock->conf.count && i<ND_MAX_ITEMS;++i){
        if(!dock->dirty_tiles[i] && !dock->graph_step[i])continue;
        dock->dirty_tiles[i]=0;dock->graph_step[i]=0;
        if(!item_visible(dock,i))continue;
        draw_tile(dock,i);repainted[i]=1;any=1;
    }
    if(any && dock->hover_widget_mask){
        WaitBlit();port.BitMap=dock->hover_widget_mask;dock->painting_mask=1;
        for(i=0;i<dock->conf.count && i<ND_MAX_ITEMS;++i){
            int x,y,w,h;
            if(!repainted[i] || (dock->hover_layout[i].kind!=ND_HOVER_WIDGET &&
               !metric_kind(dock,&dock->conf.items[i])))continue;
            tile_coords(dock,i,&x,&y);tile_dimensions(dock,i,&w,&h);
            SetAPen(&port,0);RectFill(&port,x,y,x+w-1,y+h-1);
            draw_tile(dock,i);
        }
        dock->painting_mask=0;
    }
    dock->painting_shelf=0;dock->window=real;
    for(i=0;i<dock->conf.count && i<ND_MAX_ITEMS;++i)
        if(repainted[i]){dock->hover_widgets[i].serial=0;dock->hover_tile_changed[i]=1;}   /* rebuild this widget only */
    if(any)draw_hover(dock);
    return 1;
}
void draw_all(struct NDDock *dock)
{
    int i,first,last,direct=0;
    struct Window *real_window,target;
    struct RastPort buffer_port;
    ND_TRACE_START(render_start);
    if (!dock->window || dock->box_pending) return;
    ++dock->diagnostic_full_paints;
    dock->caption_dirty=1;
    if (dock->hidden) { draw_tab(dock); return; }
    real_window=dock->window;
    if(dock->hover_headroom){
        dock->hover_prev_valid=0;   /* the next hover frame is composed in full */
        memset(dock->hover_captions,0,sizeof(dock->hover_captions));
        prepare_hover_layout(dock);
        if(dock->hover_shelf && (dock->hover_shelf_width!=dock->width ||
                                dock->hover_shelf_height!=dock->height)){
            WaitBlit();FreeBitMap(dock->hover_shelf);dock->hover_shelf=NULL;
            if(dock->hover_widget_mask)FreeBitMap(dock->hover_widget_mask);
            dock->hover_widget_mask=NULL;
        }
        if(!dock->hover_shelf)
            dock->hover_shelf=AllocBitMap(dock->width,dock->height,
                dock->screen->RastPort.BitMap->Depth,BMF_CLEAR,dock->screen->RastPort.BitMap);
        dock->hover_shelf_valid=0;
        if(dock->hover_shelf){
            WaitBlit();
            dock->hover_shelf_width=dock->width;dock->hover_shelf_height=dock->height;
            buffer_port=*real_window->RPort;buffer_port.Layer=NULL;
            buffer_port.BitMap=dock->hover_shelf;
            target=*real_window;target.RPort=&buffer_port;dock->window=&target;
        }
    }
    if(dock->hover_headroom&&!dock->hover_shelf)direct=begin_canvas_paint(dock);
    /* Intuition windows are rectangular and OS 3.x has no alpha window
     * shape.  Always paint the complete layer: leaving the short-row sides
     * unfilled exposes Intuition's white backfill rather than Workbench. */
    dock->painting_shelf=dock->hover_headroom!=0;
    paint_background(dock,0,0,dock->width,dock->height);
    if (!dock->hover_headroom && dock->theme_spec.bevel && !dock->background.parts[0].bitmap &&
        dock->width>3 && dock->height>3) {
        struct RastPort *rp=dock->window->RPort;
        SetAPen(rp,pen(dock,ND_HOVER));
        Move(rp,0,dock->height-2);Draw(rp,0,0);Draw(rp,dock->width-2,0);
        SetAPen(rp,pen(dock,ND_SUBTLE));
        Move(rp,dock->width-1,1);Draw(rp,dock->width-1,dock->height-1);
        Draw(rp,1,dock->height-1);
    }
    if (dock->edit_mode) {
        struct RastPort *rp=dock->window->RPort;
        /* Editing must be discoverable without turning the entire dock into
         * a bright selection rectangle. */
        SetAPen(rp,pen(dock,ND_SUBTLE));
        SetDrPt(rp,0xCCCCU);
        Move(rp,0,0);Draw(rp,dock->width-1,0);
        Draw(rp,dock->width-1,dock->height-1);
        Draw(rp,0,dock->height-1);Draw(rp,0,0);
        SetDrPt(rp,0xFFFFU);
    }
    first=dock->overflow ? dock->view_start : 0;
    last=dock->overflow ? first+dock->visible_count : dock->conf.count;
    if (last>dock->conf.count) last=dock->conf.count;
    dock->painting_shelf=dock->hover_headroom!=0;
    draw_grip(dock);
    for (i=first;i<last;++i) draw_tile(dock,i);
    dock->painting_shelf=0;
    if(dock->hover_headroom){
        if(dock->hover_shelf){
            /* Render the same live widget shapes in one bit. This avoids
             * reading/colour-keying RTG pixels and keeps the enlarged
             * background transparent, including text and analogue clocks. */
            if(!dock->hover_widget_mask)
                dock->hover_widget_mask=AllocBitMap(dock->width,dock->height,1,BMF_CLEAR,NULL);
            if(dock->hover_widget_mask){
                WaitBlit();buffer_port.BitMap=dock->hover_widget_mask;
                SetRast(&buffer_port,0);dock->painting_mask=1;dock->painting_shelf=1;
                for(i=first;i<last;++i)if(dock->hover_layout[i].kind==ND_HOVER_WIDGET || metric_kind(dock,&dock->conf.items[i]))draw_tile(dock,i);
                dock->painting_mask=0;dock->painting_shelf=0;
            }
        }
        dock->window=real_window;
        if(direct){end_canvas_paint(dock);direct=0;}
        dock->hover_shelf_valid=dock->hover_shelf!=NULL;
        if(dock->hover_shelf_valid){++dock->hover_shelf_serial;draw_hover(dock);}
        else{direct=begin_canvas_paint(dock);paint_hover_wave(dock);if(direct)end_canvas_paint(dock);}
    }
    if (dock->overflow) {
        draw_scroll_control(dock,-1);
        draw_scroll_control(dock,1);
    }
    draw_insertion_marker(dock);
    memset(dock->dirty_tiles,0,sizeof(dock->dirty_tiles));
    memset(dock->graph_step,0,sizeof(dock->graph_step));
    ND_TRACE_END(NDT_PAINT,render_start);
}
/* Hover and drag feedback normally changes only a few tiles. Repainting the
 * entire dock for each boundary crossing is conspicuous on baseline 68k
 * machines, especially when graph plugins and planar icons are visible. */
void redraw_changed_tiles(struct NDDock *dock, int a, int b, int c)
{
    if(dock->hover_headroom){
        /* Repaint only these tiles into the shelf (refresh_shelf_tiles()). */
        if(a>=0 && a<ND_MAX_ITEMS)dock->dirty_tiles[a]=1;
        if(b>=0 && b<ND_MAX_ITEMS)dock->dirty_tiles[b]=1;
        if(c>=0 && c<ND_MAX_ITEMS)dock->dirty_tiles[c]=1;
        if(!refresh_shelf_tiles(dock))draw_all(dock);
        return;
    }
    if (item_visible(dock,a)) draw_tile(dock,a);
    if (item_visible(dock,b) && b != a) draw_tile(dock,b);
    if (item_visible(dock,c) && c != a && c != b)
        draw_tile(dock, c);
}
void mark_tile_dirty(struct NDDock *dock, int index)
{
    if (index >= 0 && index < ND_MAX_ITEMS)
        dock->dirty_tiles[index] = 1;
}
static void apply_volume_meters(struct NDDock *dock,int query)
{
 int i;
 if(dock->hidden || dock->covered || dock->box_pending)return;
 if(query)nd_volume_meters_begin();
 for(i=0;i<dock->conf.count;++i){
  struct NDItem *item=&dock->conf.items[i];struct NDVolumeSpace space;UWORD next;int changed=0,low;
  if(!nd_item_supports_space_meter(item) || (dock->window && !item_visible(dock,i)))continue;
  if(!item->show_volume_meter){if(item->meter!=ND_NO_METER){item->meter=ND_NO_METER;mark_tile_dirty(dock,i);}continue;}
  if(query)nd_volume_meters_add(item->command);
  /* A failed/slow query must not erase a previously valid reading or re-arm
   * its low-space alert. Cached results are keyed by path, never tile index. */
  if(!nd_volume_meters_cached(item->command,&space)){
   if(item->meter!=ND_NO_METER || item->volume_total){item->meter=ND_NO_METER;item->volume_total=item->volume_used=item->volume_block_bytes=0;item->volume_low_notified=0;mark_tile_dirty(dock,i);}
   continue;
  }
  next=(UWORD)nd_used_percent(space.used,space.total);low=nd_volume_low(&space,item->volume_warning);
  if(low && !item->volume_low_notified){char summary[100];nd_volume_summary(&space,summary);dock_alert(dock,item->label,"Low disk space",summary,ND_ALERT_WARNING);}
  changed=item->volume_total!=space.total || item->volume_used!=space.used || item->volume_block_bytes!=space.block_bytes || item->volume_low_notified!=low || item->meter!=next;
  item->volume_total=space.total;item->volume_used=space.used;item->volume_block_bytes=space.block_bytes;item->volume_low_notified=low;item->meter=next;
  if(changed)mark_tile_dirty(dock,i);
 }
 if(query){char program[512];strcpy(program,dock->directory);if(AddPart(program,"SupportLog",sizeof(program)))(void)nd_volume_meters_submit(program);}
}
void refresh_volume_meters(struct NDDock *dock)
{
 (void)nd_volume_meters_poll();apply_volume_meters(dock,1);
}
void process_volume_meters(struct NDDock *dock)
{
 if(nd_volume_meters_poll())apply_volume_meters(dock,0);
}
void flush_dirty_tiles(struct NDDock *dock)
{
    int i;
    /* Hidden or covered: keep the marks; the next visible flush paints. */
    if (!dock->window || dock->hidden || dock->box_pending || dock->covered)
        return;
    if(dock->hover_headroom){
        for(i=0;i<dock->conf.count && i<ND_MAX_ITEMS;++i)
            if(item_visible(dock,i) && (dock->dirty_tiles[i] || dock->graph_step[i])){
                if(!refresh_shelf_tiles(dock))draw_all(dock);
                return;
            }
        return;
    }
    for (i = 0; i < dock->conf.count && i < ND_MAX_ITEMS; ++i) {
        int full=dock->dirty_tiles[i],step=dock->graph_step[i];
        dock->dirty_tiles[i]=0;
        dock->graph_step[i]=0;
        if (!item_visible(dock,i)) continue;
        if (full) draw_tile(dock,i);
        else if (step) draw_graph_step(dock,i);
    }
}
