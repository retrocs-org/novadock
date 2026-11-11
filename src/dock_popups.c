/* NovaDock: Popup windows: calendar, About and drawer contents. */
#include "dock_internal.h"

/* Defined below, private to this file. */
static int calendar_leap(int year);
static int calendar_month_days(int year,int month);
static LONG calendar_days_before(int year,int month);
static void calendar_date_from_days(LONG days,int *year,int *month,int *day);
static void calendar_text(struct NDDock *dock,const char *s,int x,int y,int w,int color);
static void draw_calendar(struct NDDock *dock);
static void about_text(struct NDDock *dock,const char *s,int baseline, int color);
static void draw_about(struct NDDock *dock);
static void free_folder_icons(struct NDDock *dock);
static void close_folder_window(struct NDDock *dock);
static void draw_icon_image(struct RastPort *rp,const struct Image *image, int x,int y,int w,int h);
static void draw_folder_glyph(struct NDDock *dock,struct RastPort *rp, int drawer,int x,int y,int w,int h);
static void folder_text(struct RastPort *rp,const char *text,int x,int y, int width,int centred);
static void draw_folder_cell(struct NDDock *dock,int index);
static int folder_can_go_up(struct NDDock *dock);
static void draw_folder_popup(struct NDDock *dock);
static int read_folder(struct NDDock *dock);
static int open_folder_window(struct NDDock *dock);
static void navigate_folder(struct NDDock *dock,const char *path, const char *focus);
static void folder_go_up(struct NDDock *dock);
static void open_folder_file(const char *path);
static void activate_folder_entry(struct NDDock *dock,int index,int in_workbench);
static void set_folder_hot(struct NDDock *dock,int index);
static void folder_type_ahead(struct NDDock *dock,char ch,ULONG seconds);

#include "nd_calendar.h"
static void calendar_text(struct NDDock *dock,const char *s,int x,int y,int w,int color)
{
    struct RastPort *rp=dock->calendar->RPort;
    int len=(int)strlen(s),extent;
    SetAPen(rp,pen(dock,color));SetDrMd(rp,JAM1);
    extent=TextLength(rp,(STRPTR)s,len);
    Move(rp,x+(w-extent)/2,y);Text(rp,(STRPTR)s,len);
}
static void draw_calendar(struct NDDock *dock)
{
    static const char *months[12]={"January","February","March","April","May","June",
        "July","August","September","October","November","December"};
    static const char *week[7]={"Su","Mo","Tu","We","Th","Fr","Sa"};
    struct Window *w=dock->calendar;
    struct RastPort *rp;
    struct DateStamp stamp;
    int bx,by,iw,ih,col,row,day,first,count,today_y,today_m,today_d;
    char title[32],number[4];
    if (!w) return;
    rp=w->RPort;bx=w->BorderLeft;by=w->BorderTop;
    iw=w->Width-w->BorderLeft-w->BorderRight;
    ih=w->Height-w->BorderTop-w->BorderBottom;
    SetAPen(rp,pen(dock,ND_BG));RectFill(rp,bx,by,bx+iw-1,by+ih-1);
    SetAPen(rp,pen(dock,ND_TILE));RectFill(rp,bx+4,by+4,bx+25,by+20);
    RectFill(rp,bx+iw-26,by+4,bx+iw-5,by+20);
    calendar_text(dock,"<",bx+4,by+16,22,ND_TEXT);
    calendar_text(dock,">",bx+iw-26,by+16,22,ND_TEXT);
    sprintf(title,"%s %d",nd_tr(months[dock->calendar_month]),dock->calendar_year);
    calendar_text(dock,title,bx+28,by+17,iw-56,ND_TEXT);
    for (col=0;col<7;++col)
        calendar_text(dock,nd_tr(week[col]),bx+5+col*(iw-10)/7,by+36,(iw-10)/7,ND_SUBTLE);
    first=(int)(calendar_days_before(dock->calendar_year,dock->calendar_month)%7);
    if (first<0) first+=7;
    count=calendar_month_days(dock->calendar_year,dock->calendar_month);
    DateStamp(&stamp);
    calendar_date_from_days(stamp.ds_Days,&today_y,&today_m,&today_d);
    for (day=1;day<=count;++day) {
        int pos=first+day-1;
        col=pos%7;row=pos/7;
        if (dock->calendar_year==today_y && dock->calendar_month==today_m && day==today_d) {
            SetAPen(rp,pen(dock,ND_ACCENT));
            RectFill(rp,bx+7+col*(iw-10)/7,by+42+row*17,
                     bx+3+(col+1)*(iw-10)/7,by+57+row*17);
        }
        sprintf(number,"%d",day);
        calendar_text(dock,number,bx+5+col*(iw-10)/7,by+55+row*17,
                      (iw-10)/7,ND_TEXT);
    }
}
void close_calendar(struct NDDock *dock)
{
    struct IntuiMessage *m;
    if (!dock->calendar) return;
    while ((m=(struct IntuiMessage *)GetMsg(dock->calendar->UserPort))!=NULL)
        ReplyMsg((struct Message *)m);
    CloseWindow(dock->calendar);dock->calendar=NULL;
}
void open_calendar(struct NDDock *dock,int index)
{
    struct DateStamp stamp;
    int day,x,y,px,py,width=236,height=151;
    /* Treat the calendar as a popup: clicking Clock again closes the existing
     * instance instead of immediately rebuilding it in the same position. */
    if (dock->calendar) {close_calendar(dock);return;}
    if (!dock->screen || !item_visible(dock,index)) return;
    close_folder_popup(dock);
    DateStamp(&stamp);
    calendar_date_from_days(stamp.ds_Days,&dock->calendar_year,&dock->calendar_month,&day);
    tile_coords(dock,index,&x,&y);
    px=dock->window->LeftEdge+x;
    py=dock->window->TopEdge+y-height-4;
    if (vertical(dock)) {
        px=dock->conf.side==ND_SIDE_LEFT ? dock->window->LeftEdge+dock->width+4 :
           dock->window->LeftEdge-width-4;
        py=dock->window->TopEdge+y;
    }
    if (px+width>dock->screen->Width) px=dock->screen->Width-width;
    if (px<0) px=0;
    if (py<dock->screen->BarHeight) py=dock->window->TopEdge+y+dock->tile_height+4;
    if (py+height>dock->screen->Height) py=dock->screen->Height-height;
    dock->calendar=OpenWindowTags(NULL,WA_PubScreen,(ULONG)dock->screen,
        WA_Left,px,WA_Top,py,WA_Width,width,WA_Height,height,
        WA_Title,(ULONG)nd_tr("NovaDock Calendar"),WA_DragBar,TRUE,
        WA_CloseGadget,TRUE,WA_DepthGadget,TRUE,WA_Activate,TRUE,
        WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_MOUSEBUTTONS|
                 IDCMP_RAWKEY|IDCMP_INACTIVEWINDOW,
        WA_Flags,WFLG_SMART_REFRESH|WFLG_ACTIVATE,TAG_DONE);
    if (dock->calendar) draw_calendar(dock);
}
void calendar_events(struct NDDock *dock)
{
    struct IntuiMessage *m;
    ULONG cls,code;WORD mx,my;
    while (dock->calendar &&
           (m=(struct IntuiMessage *)GetMsg(dock->calendar->UserPort))!=NULL) {
        cls=m->Class;code=m->Code;mx=m->MouseX;my=m->MouseY;
        ReplyMsg((struct Message *)m);
        if (cls==IDCMP_REFRESHWINDOW) {
            BeginRefresh(dock->calendar);draw_calendar(dock);
            EndRefresh(dock->calendar,TRUE);
        } else if (cls==IDCMP_CLOSEWINDOW || cls==IDCMP_INACTIVEWINDOW ||
                   cls==IDCMP_RAWKEY) close_calendar(dock);
        else if (cls==IDCMP_MOUSEBUTTONS && code==SELECTDOWN) {
            int bx=dock->calendar->BorderLeft,by=dock->calendar->BorderTop;
            int iw=dock->calendar->Width-bx-dock->calendar->BorderRight;
            if (my>=by+4 && my<=by+20 && mx>=bx+4 && mx<=bx+25) {
                if (dock->calendar_year==1900 && dock->calendar_month==0) continue;
                if (--dock->calendar_month<0) {dock->calendar_month=11;--dock->calendar_year;}
                draw_calendar(dock);
            } else if (my>=by+4 && my<=by+20 && mx>=bx+iw-26 && mx<=bx+iw-5) {
                if (dock->calendar_year==9999 && dock->calendar_month==11) continue;
                if (++dock->calendar_month>11) {dock->calendar_month=0;++dock->calendar_year;}
                draw_calendar(dock);
            }
        }
    }
}
static void about_text(struct NDDock *dock,const char *s,int baseline,
                       int color)
{
    struct Window *w=dock->about;
    struct RastPort *rp;
    int inner,len,extent,x;
    if (!w) return;
    rp=w->RPort;len=(int)strlen(s);
    inner=w->Width-w->BorderLeft-w->BorderRight;
    extent=TextLength(rp,(STRPTR)s,len);
    x=w->BorderLeft+(inner-extent)/2;
    if (x<w->BorderLeft+4) x=w->BorderLeft+4;
    SetAPen(rp,pen(dock,color));SetDrMd(rp,JAM1);
    Move(rp,x,w->BorderTop+baseline);Text(rp,(STRPTR)s,len);
}
static void draw_about(struct NDDock *dock)
{
    struct Window *w=dock->about;
    struct RastPort *rp;
    int x,y,iw,ih;
    if (!w) return;
    rp=w->RPort;x=w->BorderLeft;y=w->BorderTop;
    iw=w->Width-w->BorderLeft-w->BorderRight;
    ih=w->Height-w->BorderTop-w->BorderBottom;
    SetAPen(rp,pen(dock,ND_BG));RectFill(rp,x,y,x+iw-1,y+ih-1);
    about_text(dock,ND_ABOUT_NAME " " ND_ABOUT_VERSION,18,ND_TEXT);
    about_text(dock,ND_ABOUT_DESCRIPTION_1,42,ND_TEXT);
    about_text(dock,ND_ABOUT_DESCRIPTION_2,57,ND_TEXT);
    about_text(dock,"License: " ND_ABOUT_LICENSE,81,ND_SUBTLE);
    about_text(dock,"Website: " ND_ABOUT_WEBSITE,103,ND_ACCENT);
    about_text(dock,"GitHub: " ND_ABOUT_GITHUB,120,ND_ACCENT);
}
void close_about(struct NDDock *dock)
{
    struct IntuiMessage *m;
    if (!dock->about) return;
    while ((m=(struct IntuiMessage *)GetMsg(dock->about->UserPort))!=NULL)
        ReplyMsg((struct Message *)m);
    CloseWindow(dock->about);dock->about=NULL;
}
void open_about(struct NDDock *dock)
{
    int width=440,height=162,x,y;
    if (dock->about) {
        WindowToFront(dock->about);ActivateWindow(dock->about);return;
    }
    if (!dock->screen) return;
    close_tooltip(dock);close_calendar(dock);close_folder_popup(dock);
    x=(dock->screen->Width-width)/2;
    y=(dock->screen->Height-height)/2;
    if (x<0) x=0;
    if (y<dock->screen->BarHeight) y=dock->screen->BarHeight;
    dock->about=OpenWindowTags(NULL,WA_PubScreen,(ULONG)dock->screen,
        WA_Left,x,WA_Top,y,WA_Width,width,WA_Height,height,
        WA_Title,(ULONG)nd_tr("About NovaDock"),WA_DragBar,TRUE,
        WA_CloseGadget,TRUE,WA_DepthGadget,TRUE,WA_Activate,TRUE,
        WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_RAWKEY,
        WA_Flags,WFLG_SMART_REFRESH|WFLG_ACTIVATE,TAG_DONE);
    if (dock->about) draw_about(dock);
}
void about_events(struct NDDock *dock)
{
    struct IntuiMessage *m;ULONG cls;
    while (dock->about &&
           (m=(struct IntuiMessage *)GetMsg(dock->about->UserPort))!=NULL) {
        cls=m->Class;ReplyMsg((struct Message *)m);
        if (cls==IDCMP_REFRESHWINDOW) {
            BeginRefresh(dock->about);draw_about(dock);
            EndRefresh(dock->about,TRUE);
        } else if (cls==IDCMP_CLOSEWINDOW || cls==IDCMP_RAWKEY)
            close_about(dock);
    }
}
static void free_folder_icons(struct NDDock *dock)
{
    int i;
    memset(dock->folder_icon_tried,0,sizeof(dock->folder_icon_tried));
    if (!dock->folder_icons) return;
    for (i=0;i<ND_FOLDER_MAX;++i)
        if (dock->folder_icons[i]) {
            FreeDiskObject(dock->folder_icons[i]);
            dock->folder_icons[i]=NULL;
        }
}
static void close_folder_window(struct NDDock *dock)
{
    struct IntuiMessage *m;
    if (!dock->folder_popup) return;
    while ((m=(struct IntuiMessage *)GetMsg(dock->folder_popup->UserPort))!=NULL)
        ReplyMsg((struct Message *)m);
    CloseWindow(dock->folder_popup);dock->folder_popup=NULL;
}
void close_folder_popup(struct NDDock *dock)
{
    close_folder_window(dock);
    free_folder_icons(dock);
    if (dock->folder_icons) { FreeVec(dock->folder_icons);dock->folder_icons=NULL; }
    if (dock->folder_entries) { FreeVec(dock->folder_entries);dock->folder_entries=NULL; }
    dock->folder_count=0;dock->folder_hot=-1;dock->folder_offset=0;
}
/* Classic planar icon image, scaled down to the box if needed.  Pen 0 is
 * treated as transparent so icons sit on the theme's tile colour; runs of
 * equal pens are filled in one call to keep 68000 drawing quick. */
static void draw_icon_image(struct RastPort *rp,const struct Image *image,
                            int x,int y,int w,int h)
{
    int iw,ih,dw,dh,dx,dy,sx,sy,run_start,run_pen,pen;
    if (!image || !image->ImageData || image->Width<=0 || image->Height<=0) return;
    iw=image->Width;ih=image->Height;
    if (iw<=w && ih<=h) {dw=iw;dh=ih;}
    else if ((long)iw*h>(long)ih*w) {dw=w;dh=(int)((long)ih*w/iw);}
    else {dh=h;dw=(int)((long)iw*h/ih);}
    if (dw<1) dw=1;
    if (dh<1) dh=1;
    x+=(w-dw)/2;y+=(h-dh)/2;
    for (dy=0;dy<dh;++dy) {
        sy=(int)((long)dy*ih/dh);
        run_start=0;run_pen=-1;
        for (dx=0;dx<=dw;++dx) {
            pen=-1;
            if (dx<dw) {
                sx=(int)((long)dx*iw/dw);
                pen=planar_icon_pixel(image,sx,sy);
                if (pen==0) pen=-1;
            }
            if (pen!=run_pen) {
                if (run_pen>0) {
                    SetAPen(rp,(ULONG)run_pen);
                    RectFill(rp,x+run_start,y+dy,x+dx-1,y+dy);
                }
                run_pen=pen;run_start=dx;
            }
        }
    }
}
static void draw_folder_glyph(struct NDDock *dock,struct RastPort *rp,
                              int drawer,int x,int y,int w,int h)
{
    int gw=w<24 ? w : 24,gh=h<18 ? h : 18,gx=x+(w-gw)/2,gy=y+(h-gh)/2;
    if (drawer) {
        SetAPen(rp,pen(dock,ND_ACCENT));
        RectFill(rp,gx,gy+3,gx+gw-1,gy+gh-1);
        RectFill(rp,gx,gy,gx+gw/2,gy+3);
    } else {
        SetAPen(rp,pen(dock,ND_TEXT));
        RectFill(rp,gx+3,gy,gx+gw-4,gy+gh-1);
        SetAPen(rp,pen(dock,ND_TILE));
        RectFill(rp,gx+5,gy+2,gx+gw-6,gy+gh-3);
        SetAPen(rp,pen(dock,ND_SUBTLE));
        RectFill(rp,gx+7,gy+5,gx+gw-8,gy+5);
        RectFill(rp,gx+7,gy+9,gx+gw-8,gy+9);
    }
}
static void folder_text(struct RastPort *rp,const char *text,int x,int y,
                        int width,int centred)
{
    struct TextExtent extent;
    int length=(int)strlen(text),fit;
    if (width<8) return;
    fit=(int)TextFit(rp,(STRPTR)text,(ULONG)length,&extent,NULL,1,
                     (ULONG)width,32767);
    if (fit<=0) return;
    if (centred) x+=(width-(int)TextLength(rp,(STRPTR)text,fit))/2;
    Move(rp,x,y);
    Text(rp,(STRPTR)text,fit);
}
static void draw_folder_cell(struct NDDock *dock,int index)
{
    const struct NDFolderLayout *l=&dock->folder_layout;
    struct RastPort *rp;
    struct NDFolderEntry *entry;
    struct DiskObject *icon;
    int row,column,x,y,baseline;
    if (!dock->folder_popup || index<0 || index>=dock->folder_count) return;
    row=index/l->columns-dock->folder_offset;
    column=index%l->columns;
    if (row<0 || row>=l->visible_rows) return;
    rp=dock->folder_popup->RPort;
    entry=&dock->folder_entries[index];
    icon=dock->folder_icons ? dock->folder_icons[index] : NULL;
    x=4+column*l->cell_width;
    y=l->header_height+4+row*l->cell_height;
    SetDrMd(rp,JAM1);
    SetAPen(rp,pen(dock,index==dock->folder_hot ? ND_HOVER : ND_BG));
    RectFill(rp,x,y,x+l->cell_width-1,y+l->cell_height-1);
    baseline=rp->TxBaseline;
    if (l->view==ND_FOLDER_VIEW_ICONS) {
        if (icon && (icon->do_Gadget.Flags&GFLG_GADGIMAGE) &&
            icon->do_Gadget.GadgetRender)
            draw_icon_image(rp,(const struct Image *)icon->do_Gadget.GadgetRender,
                            x+4,y+2,l->icon_width,l->icon_height);
        else draw_folder_glyph(dock,rp,entry->drawer,x+4,y+2,
                               l->icon_width,l->icon_height);
        SetAPen(rp,pen(dock,entry->drawer ? ND_ACCENT : ND_TEXT));
        folder_text(rp,entry->name,x+2,y+l->icon_height+4+baseline,
                    l->cell_width-4,1);
    } else {
        char size[16];
        int size_width=0;
        draw_folder_glyph(dock,rp,entry->drawer,x+2,y+1,14,l->cell_height-2);
        if (!entry->drawer) {
            if (entry->size>=1048576UL)
                sprintf(size,"%luM",(entry->size+524288UL)/1048576UL);
            else sprintf(size,"%luK",(entry->size+1023UL)/1024UL);
            size_width=(int)TextLength(rp,(STRPTR)size,strlen(size));
            SetAPen(rp,pen(dock,ND_SUBTLE));
            Move(rp,x+l->cell_width-4-size_width,y+2+baseline);
            Text(rp,(STRPTR)size,strlen(size));
        }
        SetAPen(rp,pen(dock,entry->drawer ? ND_ACCENT : ND_TEXT));
        folder_text(rp,entry->name,x+20,y+2+baseline,
                    l->cell_width-28-size_width,0);
    }
}
static int folder_can_go_up(struct NDDock *dock)
{
    char path[ND_COMMAND_LEN];
    strcpy(path,dock->folder_path);
    return nd_folder_parent(path,dock->folder_root);
}
/* Same order as ND_FOLDER_SORT_*; listed here so they are translatable. */
static const char *folder_sort_labels[] = { "Name", "Date", "Size" };
static void draw_folder_popup(struct NDDock *dock)
{
    const struct NDFolderLayout *l=&dock->folder_layout;
    struct RastPort *rp;
    const char *title,*sort;
    char label[ND_COMMAND_LEN+4];
    int i,first,last,w,h,sort_width;
    if (!dock->folder_popup) return;
    rp=dock->folder_popup->RPort;SetDrMd(rp,JAM1);
    w=dock->folder_popup->Width;h=dock->folder_popup->Height;
    SetAPen(rp,pen(dock,ND_BG));RectFill(rp,0,0,w-1,h-1);
    SetAPen(rp,pen(dock,ND_TILE));RectFill(rp,0,0,w-1,l->header_height-1);
    SetAPen(rp,pen(dock,ND_SUBTLE));
    Move(rp,0,h-1);Draw(rp,0,0);Draw(rp,w-1,0);Draw(rp,w-1,h-1);Draw(rp,0,h-1);
    /* Header: [<]  Title  Sort  [view] */
    title=FilePart((STRPTR)dock->folder_path);
    if (!title || !*title) title=dock->folder_path;
    sprintf(label,"%s%s",title,dock->folder_more ? " +" : "");
    if (folder_can_go_up(dock)) {
        SetAPen(rp,pen(dock,ND_ACCENT));
        Move(rp,8,3+rp->TxBaseline);Text(rp,(STRPTR)"<",1);
    }
    sort=nd_tr(folder_sort_labels[dock->folder_sort%ND_FOLDER_SORT_COUNT]);
    sort_width=(int)TextLength(rp,(STRPTR)sort,strlen(sort));
    SetAPen(rp,pen(dock,ND_SUBTLE));
    Move(rp,w-24-sort_width-6,3+rp->TxBaseline);Text(rp,(STRPTR)sort,strlen(sort));
    SetAPen(rp,pen(dock,ND_ACCENT));
    Move(rp,w-17,3+rp->TxBaseline);
    Text(rp,l->view==ND_FOLDER_VIEW_ICONS ? (STRPTR)"=" : (STRPTR)"#",1);
    SetAPen(rp,pen(dock,ND_TEXT));
    folder_text(rp,label,30,3+rp->TxBaseline,w-30-24-sort_width-12,0);
    if (!dock->folder_count) {
        SetAPen(rp,pen(dock,ND_SUBTLE));
        folder_text(rp,nd_tr("(empty)"),8,l->header_height+6+rp->TxBaseline,
                    w-16,0);
        return;
    }
    first=dock->folder_offset*l->columns;
    last=first+l->visible_rows*l->columns;
    if (last>dock->folder_count) last=dock->folder_count;
    for (i=first;i<last;++i) draw_folder_cell(dock,i);
    if (l->total_rows>l->visible_rows) {
        /* Thin scroll indicator on the right edge. */
        int track=h-l->header_height-6,top,size;
        size=track*l->visible_rows/l->total_rows;
        if (size<6) size=6;
        top=(track-size)*dock->folder_offset/(l->total_rows-l->visible_rows);
        SetAPen(rp,pen(dock,ND_ACCENT));
        RectFill(rp,w-3,l->header_height+3+top,w-2,l->header_height+3+top+size);
    }
}
void folder_icons_tick(struct NDDock *dock)
{
    char path[ND_COMMAND_LEN];
    APTR requesters;
    int i,first,last,budget=2;
    if (!dock->folder_popup || dock->folder_view!=ND_FOLDER_VIEW_ICONS ||
        !IconBase || !dock->folder_icons) return;
    first=dock->folder_offset*dock->folder_layout.columns;
    last=first+dock->folder_layout.visible_rows*dock->folder_layout.columns;
    if (last>dock->folder_count) last=dock->folder_count;
    requesters=quiet_requesters();
    for (i=first;i<last && budget>0;++i) {
        if (dock->folder_icon_tried[i]) continue;
        dock->folder_icon_tried[i]=1;--budget;
        if (nd_folder_join(path,sizeof(path),dock->folder_path,dock->folder_entries[i].name))
            dock->folder_icons[i]=GetDiskObject((STRPTR)path);
        if (dock->folder_icons[i]) draw_folder_cell(dock,i);
    }
    restore_requesters(requesters);
}
/* Read folder_path; returns the entry count, or -1 if it cannot be read. */
static int read_folder(struct NDDock *dock)
{
    struct FileInfoBlock *fib;
    APTR requesters;
    BPTR lock;
    int count=0;
    if (!dock->folder_entries)
        dock->folder_entries=(struct NDFolderEntry *)AllocVec(
            sizeof(struct NDFolderEntry)*ND_FOLDER_MAX,MEMF_PUBLIC|MEMF_CLEAR);
    if (!dock->folder_icons)
        dock->folder_icons=(struct DiskObject **)AllocVec(
            sizeof(struct DiskObject *)*ND_FOLDER_MAX,MEMF_PUBLIC|MEMF_CLEAR);
    if (!dock->folder_entries || !dock->folder_icons) return -1;
    free_folder_icons(dock);
    dock->folder_count=0;dock->folder_more=0;
    requesters=quiet_requesters();
    lock=Lock((STRPTR)dock->folder_path,ACCESS_READ);
    restore_requesters(requesters);
    if (!lock) return -1;
    fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
    if (!fib) { UnLock(lock);return -1; }
    if (Examine(lock,fib) && fib->fib_DirEntryType>0)
        while (ExNext(lock,fib)) {
            struct NDFolderEntry *entry;
            if (!nd_folder_visible_name(fib->fib_FileName)) continue;
            if (count>=ND_FOLDER_MAX) { dock->folder_more=1;break; }
            entry=&dock->folder_entries[count];
            strncpy(entry->name,fib->fib_FileName,ND_FOLDER_NAME_LEN-1);
            entry->name[ND_FOLDER_NAME_LEN-1]='\0';
            entry->drawer=fib->fib_DirEntryType>0;
            entry->size=(unsigned long)fib->fib_Size;
            entry->days=fib->fib_Date.ds_Days;
            entry->minutes=fib->fib_Date.ds_Minute;
            entry->ticks=fib->fib_Date.ds_Tick;
            ++count;
        }
    FreeDosObject(DOS_FIB,fib);UnLock(lock);
    nd_folder_sort(dock->folder_entries,count,dock->folder_sort);
    dock->folder_count=count;
    return count;
}
/* (Re)open the popup window sized for the current entries and view. */
static int open_folder_window(struct NDDock *dock)
{
    struct NDFolderLayout *l=&dock->folder_layout;
    int x,y,px,py,width,height;
    if (!dock->screen || !dock->window) return 0;
    close_folder_window(dock);
    nd_folder_layout(dock->folder_view,dock->folder_count,
                     dock->screen->Height*2/3,l);
    width=l->width;height=l->height;
    tile_coords(dock,dock->folder_tile,&x,&y);
    px=dock->window->LeftEdge+x;
    py=dock->window->TopEdge+y-height-3;
    if (vertical(dock)) {
        px=dock->conf.side==ND_SIDE_LEFT ? dock->window->LeftEdge+dock->width+3 :
           dock->window->LeftEdge-width-3;
        py=dock->window->TopEdge+y;
    }
    if (px+width>dock->screen->Width) px=dock->screen->Width-width;
    if (px<0) px=0;
    if (py<dock->screen->BarHeight) py=dock->window->TopEdge+y+dock->tile_height+3;
    if (py+height>dock->screen->Height) py=dock->screen->Height-height;
    if (py<0) py=0;
    dock->folder_offset=nd_folder_scroll_to(l,dock->folder_hot,dock->folder_offset);
    dock->folder_popup=OpenWindowTags(NULL,WA_PubScreen,(ULONG)dock->screen,
        WA_Left,px,WA_Top,py,WA_Width,width,WA_Height,height,
        WA_Borderless,TRUE,WA_Activate,TRUE,WA_RMBTrap,TRUE,
        WA_IDCMP,IDCMP_MOUSEBUTTONS|IDCMP_MOUSEMOVE|IDCMP_REFRESHWINDOW|
                 IDCMP_INACTIVEWINDOW|IDCMP_RAWKEY|IDCMP_VANILLAKEY,
        WA_Flags,WFLG_REPORTMOUSE|WFLG_SIMPLE_REFRESH|WFLG_ACTIVATE,TAG_DONE);
    if (!dock->folder_popup) return 0;
    if (dock->dock_font) SetFont(dock->folder_popup->RPort,dock->dock_font);
    draw_folder_popup(dock);
    return 1;
}
void open_folder_popup(struct NDDock *dock,int index)
{
    const struct NDItem *item;
    int count;
    if (index<0 || index>=dock->conf.count || !dock->screen) return;
    item=&dock->conf.items[index];
    if (item->kind!=ND_ITEM_DRAWER && item->kind!=ND_ITEM_GROUP) return;
    close_folder_popup(dock);close_calendar(dock);
    dock->folder_tile=index;
    strncpy(dock->folder_root,item->command,ND_COMMAND_LEN-1);
    dock->folder_root[ND_COMMAND_LEN-1]='\0';
    strcpy(dock->folder_path,dock->folder_root);
    count=read_folder(dock);
    if (count<0) { close_folder_popup(dock);
        show_feedback(dock,"Cannot read drawer contents",1);return; }
    if (!count) { close_folder_popup(dock);
        show_feedback(dock,"Drawer is empty",0);return; }
    dock->folder_hot=0;dock->folder_offset=0;dock->folder_typed[0]='\0';
    if (!open_folder_window(dock)) close_folder_popup(dock);
}
/* Enter PATH (a sub-drawer or the parent); FOCUS selects an entry by name. */
static void navigate_folder(struct NDDock *dock,const char *path,
                            const char *focus)
{
    char previous[ND_COMMAND_LEN];
    int i;
    strcpy(previous,dock->folder_path);
    strncpy(dock->folder_path,path,ND_COMMAND_LEN-1);
    dock->folder_path[ND_COMMAND_LEN-1]='\0';
    if (read_folder(dock)<0) {
        strcpy(dock->folder_path,previous);
        (void)read_folder(dock);
        DisplayBeep(dock->screen);
    }
    dock->folder_hot=dock->folder_count ? 0 : -1;
    dock->folder_offset=0;dock->folder_typed[0]='\0';
    if (focus)
        for (i=0;i<dock->folder_count;++i)
            if (!strcmp(dock->folder_entries[i].name,focus)) {dock->folder_hot=i;break;}
    if (!open_folder_window(dock)) close_folder_popup(dock);
}
static void folder_go_up(struct NDDock *dock)
{
    char path[ND_COMMAND_LEN],child[ND_FOLDER_NAME_LEN];
    const char *name;
    strcpy(path,dock->folder_path);
    if (!nd_folder_parent(path,dock->folder_root)) return;
    name=FilePart((STRPTR)dock->folder_path);
    strncpy(child,name ? name : "",sizeof(child)-1);child[sizeof(child)-1]='\0';
    navigate_folder(dock,path,child);
}
static void open_folder_file(const char *path)
{
    char quoted[ND_COMMAND_LEN*2+4];
    BPTR path_lock=Lock((STRPTR)path,ACCESS_READ);
    if (path_lock) {
        UnLock(path_lock);
        if (WorkbenchBase && WorkbenchBase->lib_Version>=44 &&
            OpenWorkbenchObject((STRPTR)path,TAG_DONE)) return;
    }
    if (nd_quote_path(quoted,sizeof(quoted),path)) launch_command(quoted);
}
/* Drawers open inside the popup; Shift (or Alt) opens them in Workbench.
 * Files open with Workbench and close the popup. */
static void activate_folder_entry(struct NDDock *dock,int index,int in_workbench)
{
    char path[ND_COMMAND_LEN];
    int drawer;
    if (index<0 || index>=dock->folder_count ||
        !nd_folder_join(path,sizeof(path),dock->folder_path,
                        dock->folder_entries[index].name)) return;
    drawer=dock->folder_entries[index].drawer;
    if (drawer && !in_workbench) {
        navigate_folder(dock,path,NULL);
        return;
    }
    close_folder_popup(dock);
    if (drawer) open_drawer(path); else open_folder_file(path);
}
static void set_folder_hot(struct NDDock *dock,int index)
{
    int old=dock->folder_hot,offset;
    if (index==old || index<0 || index>=dock->folder_count) return;
    dock->folder_hot=index;
    offset=nd_folder_scroll_to(&dock->folder_layout,index,dock->folder_offset);
    if (offset!=dock->folder_offset) {
        dock->folder_offset=offset;draw_folder_popup(dock);
    } else { draw_folder_cell(dock,old);draw_folder_cell(dock,index); }
}
static void folder_type_ahead(struct NDDock *dock,char ch,ULONG seconds)
{
    size_t n=strlen(dock->folder_typed);
    int start,found;
    /* A pause of more than a second starts a new search. */
    if (seconds>dock->folder_typed_seconds+1) n=0;
    dock->folder_typed_seconds=seconds;
    if (n+1>=sizeof(dock->folder_typed)) n=0;
    dock->folder_typed[n]=ch;dock->folder_typed[n+1]='\0';
    /* Typing the same single letter again steps to the next match. */
    start=dock->folder_hot<0 ? 0 : dock->folder_hot+(n==0 ? 1 : 0);
    found=nd_folder_find_prefix(dock->folder_entries,dock->folder_count,
                                dock->folder_typed,start);
    if (found<0 && n>0) {
        dock->folder_typed[0]=ch;dock->folder_typed[1]='\0';
        found=nd_folder_find_prefix(dock->folder_entries,dock->folder_count,
                                    dock->folder_typed,dock->folder_hot+1);
    }
    if (found>=0) set_folder_hot(dock,found); else DisplayBeep(dock->screen);
}
void folder_popup_events(struct NDDock *dock)
{
    struct IntuiMessage *m;
    ULONG cls,code,seconds;UWORD qualifier;WORD mx,my;int hit,old_view;
    while (dock->folder_popup &&
           (m=(struct IntuiMessage *)GetMsg(dock->folder_popup->UserPort))!=NULL) {
        cls=m->Class;code=m->Code;mx=m->MouseX;my=m->MouseY;
        qualifier=m->Qualifier;seconds=m->Seconds;
        ReplyMsg((struct Message *)m);
        if (cls==IDCMP_REFRESHWINDOW) {
            BeginRefresh(dock->folder_popup);draw_folder_popup(dock);
            EndRefresh(dock->folder_popup,TRUE);
            continue;
        }
        if (cls==IDCMP_INACTIVEWINDOW ||
            (cls==IDCMP_MOUSEBUTTONS && code==MENUDOWN)) {
            close_folder_popup(dock);continue;
        }
        hit=nd_folder_hit(&dock->folder_layout,dock->folder_count,
                          dock->folder_offset,folder_can_go_up(dock),mx,my);
        if (cls==IDCMP_MOUSEMOVE) {
            if (hit>=0) set_folder_hot(dock,hit);
        } else if (cls==IDCMP_MOUSEBUTTONS && code==SELECTDOWN) {
            if (hit>=0)
                activate_folder_entry(dock,hit,(qualifier&(IEQUALIFIER_LSHIFT|
                    IEQUALIFIER_RSHIFT|IEQUALIFIER_LALT|IEQUALIFIER_RALT))!=0);
            else if (hit==ND_FOLDER_HIT_UP) folder_go_up(dock);
            else if (hit==ND_FOLDER_HIT_TITLE) {
                char path[ND_COMMAND_LEN];
                strcpy(path,dock->folder_path);
                close_folder_popup(dock);
                open_drawer(path);
            } else if (hit==ND_FOLDER_HIT_SORT) {
                char focus[ND_FOLDER_NAME_LEN];
                int i;
                focus[0]='\0';
                if (dock->folder_hot>=0)
                    strcpy(focus,dock->folder_entries[dock->folder_hot].name);
                dock->folder_sort=(dock->folder_sort+1)%ND_FOLDER_SORT_COUNT;
                free_folder_icons(dock);
                nd_folder_sort(dock->folder_entries,dock->folder_count,
                               dock->folder_sort);
                for (i=0;i<dock->folder_count;++i)
                    if (!strcmp(dock->folder_entries[i].name,focus)) dock->folder_hot=i;
                dock->folder_offset=nd_folder_scroll_to(&dock->folder_layout,
                    dock->folder_hot,0);
                draw_folder_popup(dock);
                persist_folder_choice(dock);
            } else if (hit==ND_FOLDER_HIT_VIEW) {
                old_view=dock->folder_view;
                dock->folder_view=old_view==ND_FOLDER_VIEW_ICONS ?
                    ND_FOLDER_VIEW_LIST : ND_FOLDER_VIEW_ICONS;
                if (!open_folder_window(dock)) close_folder_popup(dock);
                persist_folder_choice(dock);
            }
        } else if (cls==IDCMP_VANILLAKEY) {
            if (code==27) close_folder_popup(dock);
            else if (code==13) activate_folder_entry(dock,dock->folder_hot,
                    (qualifier&(IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT))!=0);
            else if (code==8) folder_go_up(dock);
            else if (code>32 && code!=127) folder_type_ahead(dock,(char)code,seconds);
        } else if (cls==IDCMP_RAWKEY && !(code&IECODE_UP_PREFIX)) {
            int list=dock->folder_layout.view==ND_FOLDER_VIEW_LIST;
            if (code==0x4C) set_folder_hot(dock,nd_folder_move(&dock->folder_layout,
                    dock->folder_count,dock->folder_hot,0,-1));
            else if (code==0x4D) set_folder_hot(dock,nd_folder_move(&dock->folder_layout,
                    dock->folder_count,dock->folder_hot,0,1));
            else if (code==0x4F) {
                if (list) folder_go_up(dock);
                else set_folder_hot(dock,nd_folder_move(&dock->folder_layout,
                         dock->folder_count,dock->folder_hot,-1,0));
            } else if (code==0x4E) {
                if (list) {
                    if (dock->folder_hot>=0 &&
                        dock->folder_entries[dock->folder_hot].drawer)
                        activate_folder_entry(dock,dock->folder_hot,0);
                } else set_folder_hot(dock,nd_folder_move(&dock->folder_layout,
                         dock->folder_count,dock->folder_hot,1,0));
            } else if (code==0x7A || code==0x7B) {
                /* NewMouse wheel up/down: scroll one row. */
                int offset=dock->folder_offset+(code==0x7A ? -1 : 1);
                int max=dock->folder_layout.total_rows-dock->folder_layout.visible_rows;
                if (offset<0) offset=0;
                if (offset>max) offset=max;
                if (offset!=dock->folder_offset) {
                    dock->folder_offset=offset;draw_folder_popup(dock);
                }
            } else if (code==0x45) close_folder_popup(dock);
        }
    }
}
