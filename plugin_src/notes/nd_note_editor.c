/* Native selection formatting with a Markdown-backed document and source view. */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <devices/inputevent.h>
#include <devices/clipboard.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <proto/keymap.h>
#include <proto/diskfont.h>
#include <stdio.h>
#include <string.h>
#include "nd_note_editor.h"
#include "nd_markdown.h"
#include "../clipboard/nd_clipboard.h"
#if defined(ND_PLUGIN_RESOURCES)
#include "nd_plugin_resources.h"
#endif
extern struct IntuitionBase *IntuitionBase;
extern struct GfxBase *GfxBase;
extern struct Library *GadToolsBase,*KeymapBase,*DiskfontBase;
struct NoteClipboard {
    struct MsgPort *port;struct IOClipReq *io;
    unsigned char raw[16384];char text[ND_NOTE_TEXT];
    unsigned long size;int opened,pending,state,error,paste;struct NDNoteText selection;
};
struct NDNoteEditor {
    struct Screen *screen;struct Window *window,*context;struct Gadget *gadgets,*title,*scroll,*mode,*controls[5];int controls_value[5];
    APTR visual;struct DrawInfo *di;struct TextFont *fonts[3];struct NDNote *note;
    struct NDMarkdown *view,*rollback;struct NDMDStyle typing;
    struct NDNoteText edit;struct NoteClipboard *clip;
    unsigned short start[2048],length[2048],heights[2048],baselines[2048],widths[3][4][256];
    unsigned int lines,top,visible,body_y,body_h;int focus,markdown,typing_set,dragging,layout_dirty,painted,pending_paint,source_dirty;
    unsigned int painted_cursor,painted_anchor,painted_top;
    LONG inks[6],paper;unsigned char owned[7];char notice[128];
};

static void clipboard_submit(struct NoteClipboard *c,int state,UWORD command,void *data,ULONG length)
{
    c->state=state;c->pending=1;c->io->io_Command=command;c->io->io_Data=data;
    c->io->io_Length=length;c->io->io_Error=0;c->io->io_Actual=0;SendIO((struct IORequest *)c->io);
}
static void clipboard_close(struct NoteClipboard *c)
{
    if(!c)return;
    if(c->pending){
        WaitIO((struct IORequest *)c->io);
        if((c->state==1 || c->state==2) && c->io->io_Actual){
            do {c->io->io_Command=CMD_READ;c->io->io_Data=NULL;c->io->io_Length=0x7fffffffUL;c->io->io_Error=0;
                DoIO((struct IORequest *)c->io);}while(!c->io->io_Error && c->io->io_Actual);
        }else if(c->state==3 && !c->io->io_Error){c->io->io_Command=CMD_UPDATE;DoIO((struct IORequest *)c->io);}
    }
    if(c->opened)CloseDevice((struct IORequest *)c->io);
    if(c->io)DeleteIORequest((struct IORequest *)c->io);if(c->port)DeleteMsgPort(c->port);FreeVec(c);
}
void nd_note_editor_notice(struct NDNoteEditor *e,const char *message)
{if(e){strncpy(e->notice,message,sizeof(e->notice)-1);e->notice[sizeof(e->notice)-1]=0;SetWindowTitles(e->window,e->notice,(STRPTR)-1);}}
static const char *document(struct NDNoteEditor *e)
{return e->markdown ? e->note->text:e->view->text;}
static struct NDMDStyle style_at(struct NDNoteEditor *e,unsigned int pos)
{
    struct NDMDStyle f;memset(&f,0,sizeof(f));
    if(!e->markdown)f=e->view->format[pos];
    return f;
}
static unsigned int face_for(struct NDMDStyle f)
{return f.face<3 ? f.face:0;}
static unsigned int effect_for(struct NDMDStyle f)
{return f.style | (f.heading ? 1:0) | (f.heading==4 ? 2:0);}
static void set_type(struct NDNoteEditor *e,struct NDMDStyle f)
{
    struct RastPort *rp=e->window->RPort;
    SetFont(rp,e->fonts[face_for(f)]);
    SetSoftStyle(rp,(effect_for(f)&1 ? FSF_BOLD:0)|(effect_for(f)&2 ? FSF_ITALIC:0),AskSoftStyle(rp));
}
static unsigned int glyph_width(struct NDNoteEditor *e,unsigned int pos)
{
    struct NDMDStyle f=style_at(e,pos);unsigned int face=face_for(f),style=effect_for(f)&3;
    unsigned char c=(unsigned char)document(e)[pos];unsigned short *cached=&e->widths[face][style][c];
    if(!*cached){set_type(e,f);*cached=(unsigned short)TextLength(e->window->RPort,(STRPTR)&c,1);if(!*cached)*cached=1;}
    return *cached;
}
static unsigned int cursor_line(struct NDNoteEditor *e)
{unsigned int i;for(i=0;i+1<e->lines;++i)if(e->edit.cursor<e->start[i+1])return i;return e->lines ? e->lines-1:0;}
static void layout(struct NDNoteEditor *e)
{
    unsigned int pos=0,start=0,width=0,height=10,base=7,face,n;
    struct NDMDStyle f;const char *text;
    if(!e->layout_dirty)return;e->layout_dirty=0;n=(unsigned int)strlen(document(e));
    text=document(e);e->lines=0;
    while(pos<n && e->lines<2047){
        f=style_at(e,pos);face=face_for(f);
        if(text[pos]=='\n' || (width && width+glyph_width(e,pos)>(unsigned int)e->window->Width-58)){
            e->start[e->lines]=start;e->length[e->lines]=pos-start;e->heights[e->lines]=height;e->baselines[e->lines++]=base;
            if(text[pos]=='\n')++pos;start=pos;width=0;height=10;base=7;continue;
        }
        width+=glyph_width(e,pos);if(e->fonts[face]->tf_YSize+2>height)height=e->fonts[face]->tf_YSize+2;
        if(e->fonts[face]->tf_Baseline>base)base=e->fonts[face]->tf_Baseline;++pos;
    }
    e->start[e->lines]=start;e->length[e->lines]=pos-start;e->heights[e->lines]=height;e->baselines[e->lines++]=base;
    if(e->top>=e->lines)e->top=e->lines-1;
}
static unsigned int page_lines(struct NDNoteEditor *e,unsigned int top)
{unsigned int h=0,n=0;while(top+n<e->lines && h+e->heights[top+n]<=e->body_h){h+=e->heights[top+n];++n;}return n ? n:1;}
static unsigned int pixel_offset(struct NDNoteEditor *e,unsigned int start,unsigned int count)
{unsigned int w=0;while(count--)w+=glyph_width(e,start++);return w;}
static void sync_controls(struct NDNoteEditor *e)
{
    unsigned int a=e->edit.cursor<e->edit.anchor ? e->edit.cursor:e->edit.anchor;
    unsigned int b=e->edit.cursor>e->edit.anchor ? e->edit.cursor:e->edit.anchor,i;
    struct NDMDStyle f=e->typing_set ? e->typing:style_at(e,a);int values[5];
    if(!e->markdown && b>a)for(i=a;i<b;++i)f.style&=style_at(e,i).style;
    values[0]=f.face;values[1]=f.heading;values[2]=f.style&1;values[3]=(f.style&2)!=0;values[4]=f.color;
    for(i=0;i<5;++i)if(e->controls[i] && e->controls_value[i]!=values[i]){
        GT_SetGadgetAttrs(e->controls[i],e->window,NULL,i==2 || i==3 ? GTCB_Checked:GTCY_Active,values[i],TAG_DONE);
        e->controls_value[i]=values[i];
    }
}
static void paint(struct NDNoteEditor *e,int follow,int incremental)
{
    struct RastPort *rp=e->window->RPort;struct NDMDStyle f,next;
    const char *text=document(e);unsigned int row,i,p,end,a,b,run,width;
    int x,y,selected,next_selected,repaint;unsigned int old_a,old_b;
    sync_controls(e);layout(e);row=cursor_line(e);e->visible=page_lines(e,e->top);
    if(follow){if(row<e->top)e->top=row;while(row>=e->top+page_lines(e,e->top))++e->top;}
    e->visible=page_lines(e,e->top);
    GT_SetGadgetAttrs(e->scroll,e->window,NULL,GTSC_Total,e->lines,GTSC_Visible,e->visible,GTSC_Top,e->top,TAG_DONE);
    incremental=incremental && e->painted && e->painted_top==e->top;
    old_a=e->painted_cursor<e->painted_anchor ? e->painted_cursor:e->painted_anchor;
    old_b=e->painted_cursor>e->painted_anchor ? e->painted_cursor:e->painted_anchor;
    SetDrMd(rp,JAM1);SetAPen(rp,e->paper);
    if(!incremental)RectFill(rp,12,e->body_y,e->window->Width-34,e->body_y+e->body_h-1);
    a=e->edit.cursor<e->edit.anchor ? e->edit.cursor:e->edit.anchor;b=e->edit.cursor>e->edit.anchor ? e->edit.cursor:e->edit.anchor;
    y=e->body_y;
    for(i=e->top;i<e->lines && i<e->top+e->visible;++i){
        x=16;p=e->start[i];end=p+e->length[i];repaint=1;
        if(incremental){
            unsigned int before_a=old_a>p ? old_a:p,before_b=old_b<end ? old_b:end;
            unsigned int after_a=a>p ? a:p,after_b=b<end ? b:end;
            if(before_a>=before_b)before_a=before_b=0;
            if(after_a>=after_b)after_a=after_b=0;
            repaint=before_a!=after_a || before_b!=after_b ||
                (e->painted_cursor>=p && e->painted_cursor<=end) || (e->edit.cursor>=p && e->edit.cursor<=end);
        }
        if(!repaint){y+=e->heights[i];continue;}
        if(incremental){SetAPen(rp,e->paper);RectFill(rp,12,y,e->window->Width-34,y+e->heights[i]-1);}
        while(p<end){
            f=style_at(e,p);selected=p>=a && p<b;run=p+1;
            while(run<end){next=style_at(e,run);next_selected=run>=a && run<b;
                if(memcmp(&f,&next,sizeof(f)) || selected!=next_selected)break;++run;}
            width=pixel_offset(e,p,run-p);set_type(e,f);
            if(selected){SetAPen(rp,e->di->dri_Pens[FILLPEN]);RectFill(rp,x,y,x+width-1,y+e->heights[i]-1);}
            SetAPen(rp,selected ? e->di->dri_Pens[FILLTEXTPEN]:e->inks[f.color<6 ? f.color:0]);
            Move(rp,x,y+e->baselines[i]);Text(rp,(STRPTR)(text+p),run-p);x+=width;p=run;
        }
        if(i==row && e->focus){
            p=e->edit.cursor-e->start[i];if(p>e->length[i])p=e->length[i];width=pixel_offset(e,e->start[i],p);
            SetAPen(rp,e->inks[0]);Move(rp,16+width,y);Draw(rp,16+width,y+e->heights[i]-1);
        }
        y+=e->heights[i];
    }
    e->painted=1;e->painted_cursor=e->edit.cursor;e->painted_anchor=e->edit.anchor;e->painted_top=e->top;
}
static void draw(struct NDNoteEditor *e,int follow){e->pending_paint=0;paint(e,follow,0);}
static void draw_selection(struct NDNoteEditor *e){e->pending_paint=0;paint(e,1,1);}
static int commit_view(struct NDNoteEditor *e)
{
    char *raw=AllocVec(ND_NOTE_TEXT,MEMF_PUBLIC);char metadata[ND_NOTE_FORMAT];int ok=0;
    if(raw && nd_md_formats_write(e->view,metadata,sizeof(metadata)) && nd_md_write(e->view,raw)){
        strcpy(e->note->text,raw);strcpy(e->note->format,metadata);e->layout_dirty=1;
        e->note->style=0;e->note->size=8;strcpy(e->note->font,"topaz.font");ok=1;
    }else {memcpy(e->view,e->rollback,sizeof(*e->view));nd_note_editor_notice(e,"Note or styling limit reached; previous text preserved");}
    if(raw)FreeVec(raw);return ok;
}
static int reconcile_source(struct NDNoteEditor *e)
{
    char metadata[ND_NOTE_FORMAT];if(!e->source_dirty)return 1;
    nd_md_parse(e->note->text,e->rollback,0,0);nd_md_reconcile(e->view,e->rollback);
    if(!nd_md_formats_write(e->rollback,metadata,sizeof(metadata))){nd_note_editor_notice(e,"Styling metadata is full; remove some formatting before saving");return 0;}
    memcpy(e->view,e->rollback,sizeof(*e->view));strcpy(e->note->format,metadata);e->source_dirty=0;return 1;
}

static int edit_insert(struct NDNoteEditor *e,struct NDNoteText *edit,const char *text)
{
    struct NDNoteText before=*edit;struct NDMDStyle f;
    if(e->markdown){int ok=nd_note_insert(e->note->text,edit,text);if(ok){e->layout_dirty=1;e->source_dirty=1;}return ok;}
    memcpy(e->rollback,e->view,sizeof(*e->view));
    f=e->typing_set ? e->typing:style_at(e,edit->cursor<edit->anchor ? edit->cursor:edit->anchor);
    if(!nd_md_insert(e->view,edit,text,f) || !commit_view(e)){*edit=before;return 0;}
    return 1;
}
static void edit_backspace(struct NDNoteEditor *e,int forward)
{
    struct NDNoteText before=e->edit;
    if(e->markdown){nd_note_backspace(e->note->text,&e->edit,forward);e->layout_dirty=1;e->source_dirty=1;return;}
    memcpy(e->rollback,e->view,sizeof(*e->view));nd_md_backspace(e->view,&e->edit,forward);
    if(!commit_view(e))e->edit=before;
}
static void apply_format(struct NDNoteEditor *e,int action,unsigned int value)
{
    unsigned int raw_a,raw_b;
    if(e->clip && e->clip->pending)return;
    if(e->markdown){
        raw_a=e->edit.cursor;raw_b=e->edit.anchor;if(!reconcile_source(e))return;
        e->edit.cursor=nd_md_position(e->view,raw_a);e->edit.anchor=nd_md_position(e->view,raw_b);
        e->layout_dirty=1;e->painted=0;e->markdown=0;GT_SetGadgetAttrs(e->mode,e->window,NULL,GTCY_Active,0,TAG_DONE);
    }
    memcpy(e->rollback,e->view,sizeof(*e->view));
    if(e->edit.cursor==e->edit.anchor && action!=5 && action!=6){
        if(!e->typing_set)e->typing=style_at(e,e->edit.cursor);e->typing_set=1;
        if(action==1)e->typing.style^=1;else if(action==2)e->typing.style^=2;
        else if(action==3)e->typing.face=value;else if(action==4)e->typing.color=value;
    }else{nd_md_format(e->view,&e->edit,action,value);(void)commit_view(e);e->typing_set=0;}
    e->focus=1;draw(e,1);
}
static void toggle_mode(struct NDNoteEditor *e,int source)
{
    if(e->clip && e->clip->pending){GT_SetGadgetAttrs(e->mode,e->window,NULL,GTCY_Active,e->markdown,TAG_DONE);return;}
    if(source && !e->markdown){
        e->edit.cursor=e->view->source[e->edit.cursor];e->edit.anchor=e->view->source[e->edit.anchor];
    }else if(!source && e->markdown){
        if(!reconcile_source(e))return;e->edit.cursor=nd_md_position(e->view,e->edit.cursor);e->edit.anchor=nd_md_position(e->view,e->edit.anchor);
    }
    e->layout_dirty=1;e->painted=0;e->markdown=source;e->top=0;e->typing_set=0;e->focus=1;draw(e,1);
}
static void close_context(struct NDNoteEditor *e)
{if(e->context){CloseWindow(e->context);e->context=NULL;}}
static void open_context(struct NDNoteEditor *e,int mx,int my)
{
    static const char *labels[]={"Cut","Copy","Paste","Delete selected text","Remove formatting"};
    struct RastPort *rp;int x=e->window->LeftEdge+mx,y=e->window->TopEdge+my,i;
    close_context(e);if(x+190>e->screen->Width)x=e->screen->Width-190;
    if(y+96>e->screen->Height)y=e->screen->Height-96;
    e->context=OpenWindowTags(NULL,WA_PubScreen,(ULONG)e->screen,WA_Left,x,WA_Top,y,
        WA_Width,190,WA_Height,96,WA_Borderless,TRUE,WA_Activate,TRUE,WA_RMBTrap,TRUE,
        WA_IDCMP,IDCMP_MOUSEBUTTONS|IDCMP_INACTIVEWINDOW|IDCMP_RAWKEY,TAG_DONE);
    if(!e->context)return;rp=e->context->RPort;SetFont(rp,e->fonts[0]);SetDrMd(rp,JAM1);
    SetAPen(rp,e->di->dri_Pens[BACKGROUNDPEN]);RectFill(rp,0,0,189,95);
    SetAPen(rp,e->di->dri_Pens[TEXTPEN]);Move(rp,0,0);Draw(rp,189,0);Draw(rp,189,95);Draw(rp,0,95);Draw(rp,0,0);
    for(i=0;i<5;++i){Move(rp,8,14+i*18);Text(rp,(STRPTR)labels[i],strlen(labels[i]));}
}

struct NDNoteEditor *nd_note_editor_open(struct NDNote *note)
{
    static STRPTR fonts[]={"Topaz 8","Topaz 9","Courier 13",NULL};
    static STRPTR paragraphs[]={"Normal","H1","H2","H3","H4",NULL};
    static STRPTR colors[]={"Black","Red","Blue","Green","Purple","Grey",NULL};
    static STRPTR modes[]={"Formatted","Markdown",NULL};
    static const char *buttons[]={"Save","Save/close","Delete","Discard"};
    static const unsigned char rgb[7][3]={{0,0,0},{178,34,34},{22,77,170},{20,107,56},{120,42,144},{102,102,102},{255,255,255}};
    struct NDNoteEditor *e;struct Gadget *last;struct NewGadget ng;
    struct TextAttr attr={(STRPTR)"topaz.font",8,0,0};int w,h,top,i;LONG obtained;
    e=(struct NDNoteEditor *)AllocVec(sizeof(*e),MEMF_PUBLIC|MEMF_CLEAR);if(!e)return NULL;
    e->view=AllocVec(sizeof(*e->view),MEMF_PUBLIC|MEMF_CLEAR);e->rollback=AllocVec(sizeof(*e->rollback),MEMF_PUBLIC);
    if(!e->view || !e->rollback)goto fail;
    for(i=0;i<5;++i)e->controls_value[i]=-1;
    e->layout_dirty=1;e->note=note;e->focus=1;e->screen=LockPubScreen("Workbench");if(!e->screen)goto fail;
    w=e->screen->Width-20;if(w>620)w=620;h=e->screen->Height-e->screen->BarHeight-20;if(h>400)h=400;
    if(w<400 || h<200)goto fail;
    e->visual=GetVisualInfo(e->screen,TAG_DONE);e->di=GetScreenDrawInfo(e->screen);
    if(!e->visual || !e->di)goto fail;
    for(i=0;i<7;++i){
        obtained=ObtainBestPen(e->screen->ViewPort.ColorMap,rgb[i][0]*0x01010101UL,rgb[i][1]*0x01010101UL,rgb[i][2]*0x01010101UL,TAG_DONE);
        e->owned[i]=obtained>=0;if(i<6)e->inks[i]=obtained>=0 ? obtained:e->di->dri_Pens[TEXTPEN];
        else e->paper=obtained>=0 ? obtained:e->di->dri_Pens[BACKGROUNDPEN];
    }
    for(i=0;i<3;++i){
        attr.ta_Name=i==2 ? "courier.font":"topaz.font";attr.ta_YSize=i==2 ? 13:i==1 ? 9:8;
        e->fonts[i]=DiskfontBase ? OpenDiskFont(&attr):OpenFont(&attr);
        if(!e->fonts[i]){attr.ta_Name="topaz.font";attr.ta_YSize=i ? 9:8;e->fonts[i]=OpenFont(&attr);}
        if(!e->fonts[i])goto fail;
    }
    attr.ta_Name="topaz.font";attr.ta_YSize=8;
    if(!strncmp(note->original,"NOVADOCK-NOTE 1\n",16)){
        unsigned int c;memset(e->view,0,sizeof(*e->view));strcpy(e->view->text,note->text);
        for(c=0;c<=strlen(e->view->text);++c){e->view->format[c].style=note->style;
            e->view->format[c].face=!strcmp(note->font,"courier.font") ? 2:note->size==9 ? 1:0;}
        if(!nd_md_formats_write(e->view,note->format,sizeof(note->format)) || !nd_md_write(e->view,note->text))goto fail;note->style=0;note->size=8;strcpy(note->font,"topaz.font");
    }
    nd_md_parse(note->text,e->view,!strcmp(note->font,"courier.font") ? 2:note->size==9 ? 1:0,note->style);
    (void)nd_md_formats_read(e->view,note->format);
    if(note->style || strcmp(note->font,"topaz.font") || note->size!=8 || strstr(note->text,"<span style=\"")){
        if(!nd_md_formats_write(e->view,note->format,sizeof(note->format)) || !nd_md_write(e->view,note->text))goto fail;note->style=0;note->size=8;strcpy(note->font,"topaz.font");
    }
    last=CreateContext(&e->gadgets);if(!last)goto fail;
    memset(&ng,0,sizeof(ng));ng.ng_VisualInfo=e->visual;ng.ng_TextAttr=&attr;
    top=e->screen->WBorTop+e->screen->Font->ta_YSize+8;
    ng.ng_LeftEdge=64;ng.ng_TopEdge=top;ng.ng_Width=w-80;ng.ng_Height=18;ng.ng_Flags=PLACETEXT_LEFT;
    ng.ng_GadgetText="Title";ng.ng_GadgetID=1;
    e->title=last=CreateGadget(STRING_KIND,last,&ng,GTST_String,(ULONG)note->title,GTST_MaxChars,79,TAG_DONE);if(!last)goto fail;
    ng.ng_TopEdge+=24;ng.ng_LeftEdge=52;ng.ng_Width=116;ng.ng_GadgetText="Font";ng.ng_GadgetID=2;
    e->controls[0]=last=CreateGadget(CYCLE_KIND,last,&ng,GTCY_Labels,(ULONG)fonts,GTCY_Active,0,TAG_DONE);if(!last)goto fail;
    ng.ng_LeftEdge=220;ng.ng_Width=100;ng.ng_GadgetText="Style";ng.ng_GadgetID=6;
    e->controls[1]=last=CreateGadget(CYCLE_KIND,last,&ng,GTCY_Labels,(ULONG)paragraphs,GTCY_Active,0,TAG_DONE);if(!last)goto fail;
    ng.ng_TopEdge+=24;ng.ng_LeftEdge=12;ng.ng_Width=26;ng.ng_GadgetID=3;ng.ng_GadgetText="B";ng.ng_Flags=PLACETEXT_RIGHT;
    e->controls[2]=last=CreateGadget(CHECKBOX_KIND,last,&ng,GTCB_Checked,FALSE,TAG_DONE);if(!last)goto fail;
    ng.ng_LeftEdge=64;ng.ng_GadgetID=4;ng.ng_GadgetText="I";
    e->controls[3]=last=CreateGadget(CHECKBOX_KIND,last,&ng,GTCB_Checked,FALSE,TAG_DONE);if(!last)goto fail;
    ng.ng_LeftEdge=164;ng.ng_Width=100;ng.ng_GadgetID=7;ng.ng_GadgetText="Colour";ng.ng_Flags=PLACETEXT_LEFT;
    e->controls[4]=last=CreateGadget(CYCLE_KIND,last,&ng,GTCY_Labels,(ULONG)colors,GTCY_Active,0,TAG_DONE);if(!last)goto fail;
    ng.ng_LeftEdge=324;ng.ng_Width=w-340;if(ng.ng_Width<120){ng.ng_LeftEdge=12;ng.ng_TopEdge+=24;ng.ng_Width=w-40;}
    ng.ng_GadgetID=8;ng.ng_GadgetText="View";
    e->mode=last=CreateGadget(CYCLE_KIND,last,&ng,GTCY_Labels,(ULONG)modes,GTCY_Active,0,TAG_DONE);if(!last)goto fail;
    e->body_y=ng.ng_TopEdge+26;e->body_h=h-e->body_y-42;
    ng.ng_LeftEdge=w-28;ng.ng_TopEdge=e->body_y;ng.ng_Width=16;ng.ng_Height=e->body_h;
    ng.ng_GadgetText="";ng.ng_GadgetID=5;ng.ng_Flags=0;
    e->scroll=last=CreateGadget(SCROLLER_KIND,last,&ng,GTSC_Total,1,GTSC_Visible,1,GTSC_Top,0,PGA_Freedom,FREEVERT,TAG_DONE);if(!last)goto fail;
    for(i=0;i<4;++i){ng.ng_LeftEdge=12+i*((w-24)/4);ng.ng_TopEdge=h-32;ng.ng_Width=(w-24)/4-6;ng.ng_Height=20;
        ng.ng_GadgetID=i==2 ? 103:i==3 ? 102:100+i;ng.ng_GadgetText=(STRPTR)buttons[i];ng.ng_Flags=PLACETEXT_IN;
        last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);if(!last)goto fail;}
    strcpy(e->notice,"NovaDock Note - select text to format; right-click for editing");
    e->window=OpenWindowTags(NULL,WA_Title,(ULONG)e->notice,WA_PubScreen,(ULONG)e->screen,
        WA_Left,(e->screen->Width-w)/2,WA_Top,e->screen->BarHeight+8,WA_Width,w,WA_Height,h,
        WA_Flags,WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_ACTIVATE|WFLG_SIMPLE_REFRESH,
        WA_RMBTrap,TRUE,WA_ReportMouse,TRUE,
        WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_GADGETUP|IDCMP_MOUSEBUTTONS|IDCMP_MOUSEMOVE|IDCMP_RAWKEY|IDCMP_REFRESHWINDOW,
        WA_Gadgets,(ULONG)e->gadgets,TAG_DONE);
    if(!e->window)goto fail;GT_RefreshWindow(e->window,NULL);draw(e,1);return e;
fail:nd_note_editor_close(e);return NULL;
}

unsigned long nd_note_editor_signals(const struct NDNoteEditor *e)
{
    unsigned long signals=0;if(!e)return 0;
    if(e->window)signals|=1UL<<e->window->UserPort->mp_SigBit;
    if(e->context)signals|=1UL<<e->context->UserPort->mp_SigBit;
    if(e->clip && e->clip->pending)signals|=1UL<<e->clip->port->mp_SigBit;
    return signals;
}
void nd_note_editor_raise(struct NDNoteEditor *e)
{if(e){WindowToFront(e->window);ActivateWindow(e->window);}}
void nd_note_editor_close(struct NDNoteEditor *e)
{
    unsigned int i;
    if(!e)return;clipboard_close(e->clip);close_context(e);
    if(e->window)CloseWindow(e->window);
    for(i=0;i<3;++i)if(e->fonts[i])CloseFont(e->fonts[i]);
    for(i=0;i<7;++i)if(e->owned[i])ReleasePen(e->screen->ViewPort.ColorMap,i<6 ? e->inks[i]:e->paper);
    if(e->view)FreeVec(e->view);if(e->rollback)FreeVec(e->rollback);
    if(e->gadgets)FreeGadgets(e->gadgets);if(e->visual)FreeVisualInfo(e->visual);
    if(e->di)FreeScreenDrawInfo(e->screen,e->di);if(e->screen)UnlockPubScreen(NULL,e->screen);FreeVec(e);
}
static void copy_title(struct NDNoteEditor *e)
{strncpy(e->note->title,((struct StringInfo *)e->title->SpecialInfo)->Buffer,79);e->note->title[79]=0;}
int nd_note_editor_prepare_save(struct NDNoteEditor *e)
{if(!e)return 1;if(e->clip && e->clip->pending){nd_note_editor_notice(e,"Finish the clipboard operation before switching notes");return 0;}copy_title(e);return !e->markdown || reconcile_source(e);}
static int clipboard_start(struct NDNoteEditor *e,int paste,int cut)
{
    struct NoteClipboard *c=e->clip;unsigned int a,b;unsigned long size;
    if(!c){c=(struct NoteClipboard *)AllocVec(sizeof(*c),MEMF_PUBLIC|MEMF_CLEAR);if(!c)return 0;e->clip=c;
        c->port=CreateMsgPort();if(c->port)c->io=(struct IOClipReq *)CreateIORequest(c->port,sizeof(*c->io));
        if(c->io && !OpenDevice("clipboard.device",0,(struct IORequest *)c->io,0))c->opened=1;}
    if(!c->opened || c->pending)return 0;
    c->paste=paste;c->selection=e->edit;c->error=0;c->io->io_ClipID=0;c->io->io_Offset=0;
    if(paste)clipboard_submit(c,1,CMD_READ,c->raw,sizeof(c->raw));
    else{
        a=e->edit.cursor<e->edit.anchor ? e->edit.cursor:e->edit.anchor;
        b=e->edit.cursor>e->edit.anchor ? e->edit.cursor:e->edit.anchor;if(a==b)return 1;
        memcpy(c->text,document(e)+a,b-a);c->text[b-a]=0;
        size=nd_clip_encode(c->text,c->raw,sizeof(c->raw));if(!size)return 0;
        clipboard_submit(c,3,CMD_WRITE,c->raw,size);
        /* Cut only after UPDATE confirms the clipboard owns the text. */
        if(cut)c->paste=2;
    }
    return 1;
}
static void clipboard_service(struct NDNoteEditor *e)
{
    struct NoteClipboard *c=e->clip;int state;
    if(!c || !c->pending || !CheckIO((struct IORequest *)c->io))return;
    WaitIO((struct IORequest *)c->io);c->pending=0;state=c->state;if(c->io->io_Error)c->error=1;
    if(state==1){c->size=c->io->io_Actual;if(c->size){clipboard_submit(c,2,CMD_READ,NULL,0x7fffffffUL);return;}}
    else if(state==2 && c->io->io_Actual && !c->error){clipboard_submit(c,2,CMD_READ,NULL,0x7fffffffUL);return;}
    else if(state==3 && !c->error){clipboard_submit(c,4,CMD_UPDATE,NULL,0);return;}
    if(c->error){nd_note_editor_notice(e,"Clipboard operation failed; note preserved");return;}
    if(c->paste==1){
        int decoded=nd_clip_decode(c->raw,c->size,c->text,sizeof(c->text));
        if(decoded==1){char *from=c->text,*to=c->text;while(*from){
            if(*from=='\r'){*to++='\n';++from;if(*from=='\n')++from;}
            else {*to++=*from=='\t' ? ' ':*from;++from;}}*to=0;}
        if(decoded!=1 || !edit_insert(e,&c->selection,c->text))
            nd_note_editor_notice(e,"Clipboard text cannot fit (8 KB / 1024 lines); note preserved");
    }else if(c->paste==2)(void)edit_insert(e,&c->selection,"");
    if(c->paste)e->edit=c->selection;
    draw(e,1);
}
static void place_cursor(struct NDNoteEditor *e,int mx,int my,int extend)
{
    unsigned int row=e->top,column=0,width=0,old_cursor=e->edit.cursor,old_anchor=e->edit.anchor,old_top=e->top;int y=e->body_y;
    if(my<(int)e->body_y && e->top){--e->top;row=e->top;}
    else if(my>=(int)(e->body_y+e->body_h) && e->top+e->visible<e->lines){++e->top;row=e->top;}
    while(row+1<e->lines && y+e->heights[row]<=my){y+=e->heights[row];++row;}
    while(column<e->length[row]){unsigned int w=glyph_width(e,e->start[row]+column);
        if((int)(width+w/2)>=mx-16)break;width+=w;++column;}
    e->edit.cursor=e->start[row]+column;if(!extend)e->edit.anchor=e->edit.cursor;
    e->typing_set=0;e->focus=1;if(old_cursor!=e->edit.cursor || old_anchor!=e->edit.anchor || old_top!=e->top)e->pending_paint=1;
}
static void context_action(struct NDNoteEditor *e,int action)
{
    if(e->clip && e->clip->pending)return;
    if(action==2){if(!clipboard_start(e,1,0))nd_note_editor_notice(e,"Clipboard unavailable");}
    else if(action==0 || action==1){if(!clipboard_start(e,0,action==0))nd_note_editor_notice(e,"Select text before copying or cutting");}
    else if(action==3){(void)edit_insert(e,&e->edit,"");draw(e,1);}
    else if(action==4)apply_format(e,6,0);
}
static void service_context(struct NDNoteEditor *e)
{
    struct IntuiMessage *m;ULONG cls;UWORD code;int x,y,action;
    while(e->context && (m=(struct IntuiMessage *)GetMsg(e->context->UserPort))!=NULL){
        cls=m->Class;code=m->Code;x=m->MouseX;y=m->MouseY;ReplyMsg((struct Message *)m);
        if(cls==IDCMP_MOUSEBUTTONS && code==SELECTDOWN){
            action=(y-3)/18;if(y<3 || y>=93 || x<0 || x>=190)action=-1;
            close_context(e);if(action>=0 && action<5)context_action(e,action);
        }else if(cls==IDCMP_INACTIVEWINDOW || (cls==IDCMP_MOUSEBUTTONS && code==MENUDOWN) ||
                 (cls==IDCMP_RAWKEY && code==0x45))close_context(e);
    }
}
int nd_note_editor_events(struct NDNoteEditor *e)
{
    struct IntuiMessage *m;struct InputEvent ie;ULONG cls;UWORD code,qual,id;
    int mx,my,changed,mutated,row,column,target,n,result=0;char typed[16];unsigned int cursor;
    if(!e)return 0;clipboard_service(e);service_context(e);
    while((m=GT_GetIMsg(e->window->UserPort))!=NULL){
        cls=m->Class;code=m->Code;qual=m->Qualifier;mx=m->MouseX;my=m->MouseY;
        id=cls==IDCMP_GADGETUP ? ((struct Gadget *)m->IAddress)->GadgetID:0;
        memset(&ie,0,sizeof(ie));ie.ie_Class=IECLASS_RAWKEY;ie.ie_Code=code;ie.ie_Qualifier=qual;
        if(cls==IDCMP_RAWKEY && m->IAddress)ie.ie_EventAddress=*(APTR *)m->IAddress;
        n=cls==IDCMP_RAWKEY && !(code&0x80) ? (int)MapRawKey(&ie,typed,sizeof(typed)-1,NULL):0;
        if(n<0 || n>=sizeof(typed))n=0;typed[n]=0;GT_ReplyIMsg(m);changed=mutated=0;
        if(cls==IDCMP_CLOSEWINDOW)result=2;
        else if(cls==IDCMP_REFRESHWINDOW){GT_BeginRefresh(e->window);draw(e,0);GT_EndRefresh(e->window,TRUE);}
        else if(cls==IDCMP_MOUSEBUTTONS){
            if(code==MENUDOWN && my>=(int)e->body_y && my<(int)(e->body_y+e->body_h))open_context(e,mx,my);
            else if(code==SELECTUP)e->dragging=0;
            else if(code==SELECTDOWN){close_context(e);
                if(my>=(int)e->body_y && my<(int)(e->body_y+e->body_h) && mx>=12 && mx<e->window->Width-32){
                    e->dragging=1;place_cursor(e,mx,my,(qual&(IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT))!=0);
                }else if(my<(int)e->body_y)e->focus=0;
            }
        }else if(cls==IDCMP_MOUSEMOVE && e->dragging){
            if(!(qual&IEQUALIFIER_LEFTBUTTON))e->dragging=0;else place_cursor(e,mx,my,1);
        }else if(cls==IDCMP_GADGETUP){
            {unsigned int c;for(c=0;c<5;++c)e->controls_value[c]=-1;}
            copy_title(e);
            if(id==100)result=1;else if(id==101)result=2;else if(id==102)result=3;else if(id==103)result=4;
            else if(id==1)e->focus=1;
            else if(id==2)apply_format(e,3,code);
            else if(id==3 || id==4)apply_format(e,id==3 ? 1:2,0);
            else if(id==6)apply_format(e,5,code);
            else if(id==7)apply_format(e,4,code);
            else if(id==8)toggle_mode(e,code!=0);
            else if(id==5){e->top=code;draw(e,0);}
        }else if(cls==IDCMP_RAWKEY && !(code&0x80)){
            if(code==0x45){if(e->context)close_context(e);else result=2;}
            else if(e->focus && !(e->clip && e->clip->pending)){
                cursor=e->edit.cursor;row=cursor_line(e);column=(int)cursor-e->start[row];
                if((qual&(IEQUALIFIER_LCOMMAND|IEQUALIFIER_RCOMMAND|IEQUALIFIER_CONTROL)) && n==1){
                    int key=typed[0];if(key>=1 && key<=26)key+='a'-1;if(key>='A' && key<='Z')key+='a'-'A';
                    if(key=='a'){e->edit.anchor=0;e->edit.cursor=strlen(document(e));changed=1;}
                    else if(key=='s'){copy_title(e);result=1;}
                    else if(key=='b' || key=='i')apply_format(e,key=='b' ? 1:2,0);
                    else if(key=='v' || key=='c' || key=='x'){
                        if(!clipboard_start(e,key=='v',key=='x'))nd_note_editor_notice(e,"Clipboard is unavailable");}
                }else if(code==0x4f || code==0x4e || code==0x4c || code==0x4d){
                    if(code==0x4f && cursor)--cursor;else if(code==0x4e && cursor<strlen(document(e)))++cursor;
                    else if(code==0x4c || code==0x4d){target=row+(code==0x4c ? -1:1);
                        if(target>=0 && target<(int)e->lines){if(column>e->length[target])column=e->length[target];cursor=e->start[target]+column;}}
                    e->edit.cursor=cursor;if(!(qual&(IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT)))e->edit.anchor=cursor;e->typing_set=0;changed=1;
                }else if(code==0x41 || code==0x46){edit_backspace(e,code==0x46);changed=mutated=1;}
                else if(code==0x44 || code==0x43 || code==0x42 || n>0){
                    if(code==0x44 || code==0x43)strcpy(typed,"\n");else if(code==0x42)strcpy(typed,"    ");
                    if(!edit_insert(e,&e->edit,typed))nd_note_editor_notice(e,"Note limit: 8 KB including Markdown formatting");else mutated=1;
                    changed=1;
                }
            }
        }
        if(mutated)nd_note_editor_notice(e,"NovaDock Note - unsaved changes");if(changed){if(mutated)draw(e,1);else e->pending_paint=1;}
        if(result){copy_title(e);if(e->markdown && !reconcile_source(e))result=0;if(e->clip && e->clip->pending){nd_note_editor_notice(e,"Finish the clipboard operation before closing");result=0;}else {if(e->pending_paint)draw_selection(e);return result;}}
    }
    if(e->pending_paint)draw_selection(e);
    return 0;
}
