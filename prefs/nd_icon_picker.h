#ifndef ND_ICON_PICKER_H
#define ND_ICON_PICKER_H
#include <graphics/gfxbase.h>
#include <graphics/rastport.h>
#include <graphics/view.h>
#include <proto/graphics.h>
#include <dos/dosextens.h>
#include "nd_ilbm.h"
#include "nd_collection_icons.h"
#include "nd_hass_art.h"
#include "nd_artwork.h"
#include "nd_plugin_discovery.h"
struct GfxBase *GfxBase=NULL;
#define ND_ICON_PICKER_CHOICES ND_COLLECTION_ICON_COUNT
struct NDIconPicker {
    struct NDPrefsLayout layout;
    unsigned short pixels[ND_COLLECTION_ICON_COUNT][1024];
    LONG pens[ND_COLLECTION_ICON_COUNT*ND_ICON_MAX_COLORS];
    int pen_count,rows,page,selected;
    char title[96];
    struct DrawInfo *draw;
};
static void icon_picker_load(struct PluginPrefs *p,struct NDIconPicker *pick)
{
    struct NDILBM *image;char root[300],path[340];int i,x,y,k;ULONG map[16];
    struct Process *self=(struct Process *)FindTask(NULL);APTR old=self->pr_WindowPtr;
    self->pr_WindowPtr=(APTR)-1L;
    memset(pick->pixels,0xff,sizeof(pick->pixels));
    if(nd_plugin_install_drawer(root,sizeof(root))){*FilePart(root)=0;AddPart(root,"Icons",sizeof(root));}
    else strcpy(root,"PROGDIR:Icons");
    image=AllocVec(sizeof(*image),MEMF_PUBLIC|MEMF_CLEAR);
    if(image)for(i=0;i<ND_COLLECTION_ICON_COUNT;++i){
        strcpy(path,root);
        if(!AddPart(path,(STRPTR)FilePart((STRPTR)nd_collection_icons[i].path),sizeof(path)) || !nd_ilbm_load(path,image))continue;
        for(k=0;k<image->colors;++k){
            ULONG red=(ULONG)image->palette[k][0]*0x01010101UL;
            ULONG green=(ULONG)image->palette[k][1]*0x01010101UL;
            ULONG blue=(ULONG)image->palette[k][2]*0x01010101UL;
            LONG pen=ObtainBestPen(p->screen->ViewPort.ColorMap,red,green,blue,TAG_DONE);
            if(pen>=0){pick->pens[pick->pen_count++]=pen;map[k]=(ULONG)pen;}
            else map[k]=FindColor(p->screen->ViewPort.ColorMap,red,green,blue,-1);
        }
        for(y=0;y<32;++y)for(x=0;x<32;++x){
            k=image->pixels[(y*image->height/32)*ND_ICON_MAX_DIM+x*image->width/32];
            if(k!=image->transparent && k<image->colors)pick->pixels[i][y*32+x]=(unsigned short)map[k];
        }
    }
    if(image)FreeVec(image);self->pr_WindowPtr=old;
}
static const char *icon_picker_name(int index)
{return nd_collection_icons[index].name;}
static void icon_picker_paint(struct Window *win,struct NDIconPicker *pick)
{
    struct RastPort *rp=win->RPort;int slot,index,x,y,xx,yy,a,b,w,h,start,col,pen;
    char caption[40];const struct NDPrefsLayout *lay=&pick->layout;
    SetDrMd(rp,JAM1);SetAPen(rp,pick->draw->dri_Pens[BACKGROUNDPEN]);
    RectFill(rp,6,nd_prefs_layout_y(lay,12),lay->window_width-7,nd_prefs_layout_y(lay,16+pick->rows*54)-1);
    for(slot=0;slot<pick->rows*5;++slot){
        index=pick->page*pick->rows*5+slot;if(index>=ND_ICON_PICKER_CHOICES)break;
        x=nd_prefs_layout_x(lay,10+(slot%5)*96);y=nd_prefs_layout_y(lay,16+(slot/5)*54);
        w=nd_prefs_layout_w(lay,92);h=nd_prefs_layout_y(lay,50);
        SetAPen(rp,pick->draw->dri_Pens[index==pick->selected ? FILLPEN:BACKGROUNDPEN]);RectFill(rp,x,y,x+w-1,y+h-1);
        if(index<ND_COLLECTION_ICON_COUNT){
            for(yy=0;yy<32;++yy)for(xx=0;xx<32;){
                pen=pick->pixels[index][yy*32+xx];start=xx++;
                while(xx<32 && pick->pixels[index][yy*32+xx]==pen)++xx;
                if(pen==65535)continue;
                a=x+nd_prefs_layout_x(lay,30+start);b=y+nd_prefs_layout_y(lay,2+yy);
                SetAPen(rp,(ULONG)pen);RectFill(rp,a,b,x+nd_prefs_layout_x(lay,30+xx)-1,y+nd_prefs_layout_y(lay,3+yy)-1);
            }
        }
        strncpy(caption,icon_picker_name(index),sizeof(caption)-1);caption[sizeof(caption)-1]=0;
        col=(int)strlen(caption);
        while(col>0 && TextLength(rp,caption,col)>w-4)--col;
        if(col<(int)strlen(caption) && col>=3){caption[col-1]='.';caption[col-2]='.';caption[col-3]='.';}
        SetAPen(rp,pick->draw->dri_Pens[index==pick->selected ? FILLTEXTPEN:TEXTPEN]);
        Move(rp,x+(w-TextLength(rp,caption,col))/2,y+nd_prefs_layout_y(lay,45));Text(rp,caption,col);
    }
}
static void choose_collection_icon(struct PluginPrefs *p,int row)
{
    struct NDIconPicker *pick;struct Gadget *gadgets=NULL,*last,*g,*enabled[128],*previous=NULL,*next=NULL;
    struct Window *win=NULL;struct NewGadget ng;struct IntuiMessage *im;
    int at=visible_start(p)+row,i,j,done=0,accepted=0,enabled_count=0,height,fy,fx,slot;
    ULONG old_flags=0,seconds=0,micros=0;
    pick=AllocVec(sizeof(*pick),MEMF_PUBLIC|MEMF_REVERSE|MEMF_CLEAR);
    if(!pick){DisplayBeep(p->screen);return;}
    GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
    if(!GfxBase)goto finish;
    pick->draw=GetScreenDrawInfo(p->screen);if(!pick->draw)goto finish;
    fy=p->screen->RastPort.Font ? p->screen->RastPort.Font->tf_YSize:8;
    fx=p->screen->RastPort.Font ? p->screen->RastPort.Font->tf_XSize:8;
    height=(p->screen->Height-p->screen->BarHeight)*8/(fy<8 ? 8:fy)-12;
    pick->rows=(height-60)/54;if(pick->rows>4)pick->rows=4;if(pick->rows<1)goto finish;
    height=60+pick->rows*54;
    nd_prefs_layout_init(&pick->layout,fx,fy,p->screen->Width,p->screen->Height,p->screen->BarHeight,500,height);
    if(!nd_prefs_layout_fits(&pick->layout,8,4) || !CreateContext(&gadgets))goto finish;
    for(i=0;i<ND_ICON_PICKER_CHOICES;++i)if(!strcmp(icon_picker_name(i),p->edits[at]))pick->selected=i;
    pick->page=pick->selected/(pick->rows*5);icon_picker_load(p,pick);
    last=gadgets;
    for(i=0;i<4;++i){
        static const char *const labels[]={"Previous","Next","Choose","Cancel"};
        if(i<2 && pick->rows*5>=ND_ICON_PICKER_CHOICES)continue;
        network_picker_field(p,&pick->layout,&ng,10+i*122,height-30,112,16,i+1);
        ng.ng_GadgetText=(STRPTR)labels[i];last=CreateGadget(BUTTON_KIND,last,&ng,
            GA_Disabled,(ULONG)(i==0 ? pick->page==0:i==1 ? (pick->page+1)*pick->rows*5>=ND_ICON_PICKER_CHOICES:0),TAG_DONE);
        if(!last)goto finish;if(i==0)previous=last;if(i==1)next=last;
    }
    if(pick->rows*5<ND_ICON_PICKER_CHOICES)
        sprintf(pick->title,"Choose tile icon - page %d of %d",pick->page+1,(ND_ICON_PICKER_CHOICES+pick->rows*5-1)/(pick->rows*5));
    else strcpy(pick->title,"Choose tile icon - double-click to choose");
    win=OpenWindowTags(NULL,WA_CustomScreen,(ULONG)p->screen,
        WA_Left,nd_prefs_layout_left(&pick->layout),WA_Top,nd_prefs_layout_top(&pick->layout),
        WA_Width,pick->layout.window_width,WA_Height,pick->layout.window_height,
        WA_Title,(ULONG)pick->title,WA_Gadgets,(ULONG)gadgets,
        WA_DragBar,TRUE,WA_DepthGadget,TRUE,WA_CloseGadget,TRUE,WA_Activate,TRUE,
        WA_Flags,WFLG_SMART_REFRESH|WFLG_ACTIVATE,
        WA_IDCMP,BUTTONIDCMP|IDCMP_MOUSEBUTTONS|IDCMP_RAWKEY|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW,TAG_DONE);
    if(!win)goto finish;
    old_flags=p->win->IDCMPFlags;
    for(g=p->gadgets;g && enabled_count<128;g=g->NextGadget)if(!(g->Flags&GFLG_DISABLED)){enabled[enabled_count++]=g;OffGadget(g,p->win,NULL);}
    ModifyIDCMP(p->win,IDCMP_REFRESHWINDOW);GT_RefreshWindow(win,NULL);icon_picker_paint(win,pick);
    while(!done){
        Wait((1UL<<win->UserPort->mp_SigBit)|(1UL<<p->win->UserPort->mp_SigBit));
        while((im=GT_GetIMsg(win->UserPort))!=NULL){
            ULONG kind=im->Class,secs=im->Seconds,mics=im->Micros;
            UWORD code=im->Code,id=kind==IDCMP_GADGETUP && im->IAddress ? ((struct Gadget *)im->IAddress)->GadgetID:0;
            int mx=im->MouseX,my=im->MouseY,redraw=0;GT_ReplyIMsg(im);
            if(kind==IDCMP_CLOSEWINDOW)done=1;
            else if(kind==IDCMP_REFRESHWINDOW){GT_BeginRefresh(win);icon_picker_paint(win,pick);GT_EndRefresh(win,TRUE);}
            else if(kind==IDCMP_MOUSEBUTTONS && code==SELECTDOWN){
                int bx=mx*8/pick->layout.font_x,by=my*8/pick->layout.font_y;
                slot=(by-16)/54*5+(bx-10)/96;
                if(bx>=10 && bx<490 && by>=16 && by<16+pick->rows*54 && slot<pick->rows*5){
                    int selected=pick->page*pick->rows*5+slot;
                    if(selected<ND_ICON_PICKER_CHOICES){
                        if(selected==pick->selected && DoubleClick(seconds,micros,secs,mics))accepted=done=1;
                        pick->selected=selected;seconds=secs;micros=mics;redraw=1;
                    }
                }
            }else if(kind==IDCMP_GADGETUP){
                if(id==1 && pick->page>0){--pick->page;redraw=1;}
                if(id==2 && (pick->page+1)*pick->rows*5<ND_ICON_PICKER_CHOICES){++pick->page;redraw=1;}
                if(id==3)accepted=done=1;if(id==4)done=1;
            }else if(kind==IDCMP_RAWKEY){
                if(code==0x45)done=1;else if(code==0x44)accepted=done=1;
                else{int move=code==0x4f ? -1:code==0x4e ? 1:code==0x4c ? -5:code==0x4d ? 5:0;
                    if(move){pick->selected+=move;if(pick->selected<0)pick->selected=0;if(pick->selected>=ND_ICON_PICKER_CHOICES)pick->selected=ND_ICON_PICKER_CHOICES-1;pick->page=pick->selected/(pick->rows*5);redraw=1;}}
            }
            if(redraw){
                if(previous){
                    sprintf(pick->title,"Choose tile icon - page %d of %d",pick->page+1,(ND_ICON_PICKER_CHOICES+pick->rows*5-1)/(pick->rows*5));
                    SetWindowTitles(win,(STRPTR)pick->title,(STRPTR)-1);
                }
                if(previous)GT_SetGadgetAttrs(previous,win,NULL,GA_Disabled,(ULONG)(pick->page==0),TAG_DONE);
                if(next)GT_SetGadgetAttrs(next,win,NULL,GA_Disabled,(ULONG)((pick->page+1)*pick->rows*5>=ND_ICON_PICKER_CHOICES),TAG_DONE);
                icon_picker_paint(win,pick);
            }
        }
        while((im=GT_GetIMsg(p->win->UserPort))!=NULL){ULONG kind=im->Class;GT_ReplyIMsg(im);if(kind==IDCMP_REFRESHWINDOW){GT_BeginRefresh(p->win);GT_EndRefresh(p->win,TRUE);}}
    }
    CloseWindow(win);win=NULL;ModifyIDCMP(p->win,old_flags);
    for(j=0;j<enabled_count;++j)OnGadget(enabled[j],p->win,NULL);
    RefreshGList(p->gadgets,p->win,NULL,-1);
    if(accepted){strcpy(p->edits[at],icon_picker_name(pick->selected));GT_SetGadgetAttrs(p->fields[row],p->win,NULL,GTST_String,(ULONG)p->edits[at],TAG_DONE);}
    ActivateWindow(p->win);
finish:
    if(!done){SetWindowTitles(p->win,(STRPTR)"Could not open icon library",(STRPTR)-1);DisplayBeep(p->screen);}
    if(win)CloseWindow(win);if(gadgets)FreeGadgets(gadgets);
    for(i=0;i<pick->pen_count;++i)ReleasePen(p->screen->ViewPort.ColorMap,(ULONG)pick->pens[i]);
    if(pick->draw)FreeScreenDrawInfo(p->screen,pick->draw);
    if(GfxBase){CloseLibrary((struct Library *)GfxBase);GfxBase=NULL;}
    FreeVec(pick);
}
#include <proto/icon.h>
#include <workbench/icon.h>
#include <workbench/workbench.h>
struct Library *IconBase=NULL;
static int preview_planar_pixel(const struct Image *im,int x,int y)
{
    int plane,source=0,pixel=im->PlaneOnOff,words=(im->Width+15)/16;
    for(plane=0;plane<8;++plane)if(im->PlanePick&(1<<plane)){
        if(source<im->Depth){
            if(im->ImageData[source*words*im->Height+y*words+x/16]&(1U<<(15-x%16)))pixel|=1<<plane;
            else pixel&=~(1<<plane);
        }
        ++source;
    }
    return pixel;
}
static void collection_preview_load(struct PluginPrefs *p)
{
    int option=network_option_index(p,"ICON"),file=network_option_index(p,"ICONFILE");
    int i,x,y,k,w=0,h=0,fw,fh,dx,dy;char path[340],root[300];ULONG mapped[256];
    struct NDILBM *image=NULL;struct DiskObject *icon=NULL;struct Image *planar=NULL;
    UBYTE *pixels=NULL,*mask=NULL;struct ColorRegister *palette=NULL;
    LONG colors=0,transparent=-1,iw=0,ih=0;struct TagItem tags[8];
    struct Process *self=(struct Process *)FindTask(NULL);APTR old=self->pr_WindowPtr;
    memset(p->icon_preview,0xff,sizeof(p->icon_preview));p->icon_preview_kind=0;
    if(hass_settings(p)){
        const char *kind=nd_hass_auto_icon(hass_edit(p,"ENTITY"),hass_edit(p,"DEVICE_CLASS"),hass_edit(p,"ENTITY_NAME"));
        struct NDArtwork bank;const unsigned char *runs;unsigned int count,n;int item_index;
        memset(&bank,0,sizeof(bank));if(nd_plugin_install_drawer(root,sizeof(root)))*FilePart(root)=0;else strcpy(root,"PROGDIR:");
        for(item_index=0;item_index<p->config->count;++item_index)if(!strcmp(p->config->items[item_index].id,p->id))break;
        if(item_index>=p->config->count || !nd_artwork_path(root,p->config->items[item_index].command,path,sizeof(path)) || !nd_artwork_load(&bank,path))return;
        GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);if(!GfxBase){nd_artwork_close(&bank);return;}
        for(k=0;k<16;++k)mapped[k]=FindColor(p->screen->ViewPort.ColorMap,
            (ULONG)bank.data[8+k*3]*0x01010101UL,(ULONG)bank.data[9+k*3]*0x01010101UL,(ULONG)bank.data[10+k*3]*0x01010101UL,-1);
        runs=nd_artwork_frame(&bank,kind,ND_HASS_ICON_OFF,&count);
        if(runs){for(n=0;n<count;++n){const unsigned char *r=runs+n*5;
            for(y=r[1];y<r[1]+r[3];++y)for(x=r[0];x<r[0]+r[2];++x)p->icon_preview[y*32+x]=(unsigned short)mapped[r[4]];
        }p->icon_preview_kind=1;}
        nd_artwork_close(&bank);goto done;
    }
    if(option<0)return;
    {const char *choice=p->edits[option];int entity_option;
     if(!strcmp(nd_option_schema_id(p->config,p->id),"HASS") && !strcmp(choice,"Auto")){
         entity_option=network_option_index(p,"ENTITY");
         choice=nd_hass_default_icon(entity_option>=0 ? p->edits[entity_option]:"");
     }
     i=nd_collection_icon_index(choice);
    }
    self->pr_WindowPtr=(APTR)-1L;
    if(nd_plugin_install_drawer(root,sizeof(root)))*FilePart(root)=0;
    else strcpy(root,"PROGDIR:");
    p->icon_preview_custom=nd_collection_art_path(root,i>=0 ? nd_collection_icons[i].name:p->edits[option],file>=0 ? p->edits[file]:"",path,sizeof(path))==2;
    GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);if(!GfxBase)goto done;
    image=AllocVec(sizeof(*image),MEMF_PUBLIC|MEMF_CLEAR);
    if(image && nd_ilbm_load(path,image)){
        w=image->width;h=image->height;colors=image->colors;transparent=image->transparent;
        for(k=0;k<colors;++k)mapped[k]=FindColor(p->screen->ViewPort.ColorMap,
            (ULONG)image->palette[k][0]*0x01010101UL,(ULONG)image->palette[k][1]*0x01010101UL,(ULONG)image->palette[k][2]*0x01010101UL,-1);
    }else{
        if(image){FreeVec(image);image=NULL;}
        i=(int)strlen(path);if(i>5 && path[i-5]=='.' && (path[i-4]=='i' || path[i-4]=='I') &&
            (path[i-3]=='n' || path[i-3]=='N') && (path[i-2]=='f' || path[i-2]=='F') && (path[i-1]=='o' || path[i-1]=='O'))path[i-5]=0;
        IconBase=OpenLibrary("icon.library",37);if(!IconBase)goto done;
        if(IconBase->lib_Version>=44){
            struct TagItem load[2];load[0].ti_Tag=ICONGETA_GetPaletteMappedIcon;load[0].ti_Data=FALSE;load[1].ti_Tag=TAG_DONE;load[1].ti_Data=0;
            icon=GetIconTagList(path,load);
        }else icon=GetDiskObject(path);
        if(!icon)goto done;
        if(IconBase->lib_Version>=44){
            tags[0].ti_Tag=ICONCTRLA_GetWidth;tags[0].ti_Data=(ULONG)&iw;
            tags[1].ti_Tag=ICONCTRLA_GetHeight;tags[1].ti_Data=(ULONG)&ih;
            tags[2].ti_Tag=ICONCTRLA_GetImageData1;tags[2].ti_Data=(ULONG)&pixels;
            tags[3].ti_Tag=ICONCTRLA_GetPalette1;tags[3].ti_Data=(ULONG)&palette;
            tags[4].ti_Tag=ICONCTRLA_GetPaletteSize1;tags[4].ti_Data=(ULONG)&colors;
            tags[5].ti_Tag=ICONCTRLA_GetTransparentColor1;tags[5].ti_Data=(ULONG)&transparent;
            tags[6].ti_Tag=ICONCTRLA_GetImageMask1;tags[6].ti_Data=(ULONG)&mask;
            tags[7].ti_Tag=TAG_DONE;tags[7].ti_Data=0;IconControlA(icon,tags);
        }
        if(pixels && palette && colors>0 && colors<=256 && iw>0 && ih>0 && iw<=1024 && ih<=1024){
            w=(int)iw;h=(int)ih;
            for(k=0;k<colors;++k)mapped[k]=FindColor(p->screen->ViewPort.ColorMap,
                (ULONG)palette[k].red*0x01010101UL,(ULONG)palette[k].green*0x01010101UL,(ULONG)palette[k].blue*0x01010101UL,-1);
        }else{
            pixels=NULL;mask=NULL;transparent=-1;planar=(struct Image *)icon->do_Gadget.GadgetRender;
            if(!planar || !planar->ImageData || planar->Depth<1 || planar->Depth>8 || planar->Width<=0 || planar->Height<=0 || planar->Width>1024 || planar->Height>1024)goto done;
            w=planar->Width;h=planar->Height;
            for(k=0;k<256;++k)mapped[k]=(ULONG)(k&((p->screen->RastPort.BitMap->Depth>=8) ? 255:((1<<p->screen->RastPort.BitMap->Depth)-1)));
        }
    }
    fw=32;fh=h*32/w;if(fh>32){fh=32;fw=w*32/h;}if(fw<1)fw=1;if(fh<1)fh=1;
    dx=(32-fw)/2;dy=(32-fh)/2;
    for(y=0;y<fh;++y)for(x=0;x<fw;++x){
        int sx=x*w/fw,sy=y*h/fh;
        if(image)k=image->pixels[sy*ND_ICON_MAX_DIM+sx];
        else if(pixels)k=pixels[sy*w+sx];else k=preview_planar_pixel(planar,sx,sy);
        if((pixels && k>=colors) || k==transparent || (mask && !(mask[sy*((w+15)/16)*2+sx/8]&(1<<(7-sx%8)))))continue;
        p->icon_preview[(dy+y)*32+dx+x]=(unsigned short)mapped[k];
    }
    p->icon_preview_kind=1;
done:
    if(image)FreeVec(image);if(icon)FreeDiskObject(icon);
    if(IconBase){CloseLibrary(IconBase);IconBase=NULL;}
    if(GfxBase){CloseLibrary((struct Library *)GfxBase);GfxBase=NULL;}
    self->pr_WindowPtr=old;
}
static void collection_preview_paint(struct PluginPrefs *p)
{
    struct DrawInfo *draw;struct RastPort *rp;int x,y,size,xx,yy,a,b,pen,at,row;
    if(!p->win)return;
    if(p->focused){x=nd_prefs_layout_x(&p->layout,492);y=nd_prefs_layout_y(&p->layout,32);size=nd_prefs_layout_w(&p->layout,64);}
    else{
        at=network_option_index(p,"ICON");row=at-visible_start(p);if(row<0 || row>=page_rows(p))return;
        x=nd_prefs_layout_x(&p->layout,300);y=nd_prefs_layout_y(&p->layout,16+row*24);size=nd_prefs_layout_w(&p->layout,16);
    }
    GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);if(!GfxBase)return;
    draw=GetScreenDrawInfo(p->screen);if(!draw)goto done;
    rp=p->win->RPort;SetDrMd(rp,JAM1);
    if(!p->focused){
        const char *label=p->definitions[at].label;int len=(int)strlen(label);
        SetAPen(rp,draw->dri_Pens[TEXTPEN]);Move(rp,x-TextLength(rp,(STRPTR)label,len)-6,y+rp->TxBaseline);Text(rp,(STRPTR)label,len);
    }
    SetAPen(rp,draw->dri_Pens[BACKGROUNDPEN]);RectFill(rp,x-2,y-2,x+size+1,y+size+1);
    if(p->icon_preview_kind==1){
        for(yy=0;yy<size;++yy)for(xx=0;xx<size;){
            pen=p->icon_preview[(yy*32/size)*32+xx*32/size];a=xx++;
            while(xx<size && p->icon_preview[(yy*32/size)*32+xx*32/size]==pen)++xx;
            if(pen!=65535){SetAPen(rp,(ULONG)pen);RectFill(rp,x+a,y+yy,x+xx-1,y+yy);}
        }
    }else{
        const char *text=p->icon_preview_kind==2 ? "Aa":"?";
        SetAPen(rp,draw->dri_Pens[TEXTPEN]);Move(rp,x+(size-TextLength(rp,(STRPTR)text,(LONG)strlen(text)))/2,y+size/2+rp->TxBaseline/2);Text(rp,(STRPTR)text,(LONG)strlen(text));
    }
    if(p->focused){
        const char *caption=p->icon_preview_custom ? "Custom":"Preview";
        SetAPen(rp,draw->dri_Pens[TEXTPEN]);b=(int)strlen(caption);Move(rp,x+(size-TextLength(rp,(STRPTR)caption,b))/2,y+size+rp->TxHeight+3);Text(rp,(STRPTR)caption,b);
    }
    FreeScreenDrawInfo(p->screen,draw);
done:
    CloseLibrary((struct Library *)GfxBase);GfxBase=NULL;
}
#endif
