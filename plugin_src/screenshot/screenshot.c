/* Screenshot: click to save the front screen (or Workbench) as an IFF ILBM
 * picture after a short countdown, so another screen can be brought to the
 * front first.  A click during the countdown cancels it.  The screen is
 * copied with one blit while Intuition is locked, then encoded without the
 * lock, so other programs are held up only for that blit. */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <graphics/gfx.h>
#include <graphics/rastport.h>
#include <graphics/view.h>
#include <graphics/modeid.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/screens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_version.h"
#include "nd_client.h"
#include "nd_option_runtime.h"
#include "nd_shotfile.h"

const char nd_version[]=ND_VER_STRING("Screenshot");
long __stack=16384;
struct IntuitionBase *IntuitionBase=NULL;
struct GfxBase *GfxBase=NULL;

struct ShotSettings {
    char drawer[96];
    unsigned int delay;   /* seconds */
    int workbench;        /* 1: Workbench, 0: front screen */
};
static void read_settings(const char *id,struct ShotSettings *s)
{
    char delay[16],screen[16];
    struct NDOptionString fields[3];
    fields[0].key="DRAWER";fields[0].fallback="RAM:";fields[0].out=s->drawer;fields[0].limit=sizeof(s->drawer);
    fields[1].key="DELAY";fields[1].fallback="2";fields[1].out=delay;fields[1].limit=sizeof(delay);
    fields[2].key="SCREEN";fields[2].fallback="FRONT";fields[2].out=screen;fields[2].limit=sizeof(screen);
    nd_plugin_option_strings(id,fields,3);
    if(!s->drawer[0])strcpy(s->drawer,"RAM:");
    s->delay=(unsigned int)atoi(delay);if(s->delay>30)s->delay=2;
    s->workbench=!strcmp(screen,"WORKBENCH");
}
/* A free name in the drawer for this moment. */
static int pick_name(const char *drawer,char *path,unsigned int size)
{
    struct DateStamp now;
    BPTR lock;
    int serial;
    APTR old=((struct Process *)FindTask(NULL))->pr_WindowPtr;
    DateStamp(&now);
    ((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
    for (serial=0;serial<50;++serial) {
        if (!nd_shot_name(path,size,drawer,now.ds_Days,now.ds_Minute,
                          now.ds_Tick,serial)) break;
        lock=Lock((STRPTR)path,ACCESS_READ);
        if (!lock) {
            ((struct Process *)FindTask(NULL))->pr_WindowPtr=old;
            return 1;
        }
        UnLock(lock);
    }
    ((struct Process *)FindTask(NULL))->pr_WindowPtr=old;
    return 0;
}
/* Returns NULL on success, or the text the tile shows. */
static const char *capture(struct NDClient *client,const struct ShotSettings *s,
                           char *saved,unsigned int saved_size)
{
    static ULONG rgb[3*256+2];
    static unsigned char palette[3*256];
    struct Screen *workbench=NULL,*screen;
    struct BitMap *copy=NULL,*line_map=NULL;
    struct RastPort rp,temp;
    struct NDShotWriter writer;
    unsigned char *line=NULL;
    char path[200],progress[32];
    ULONG lock,mode;
    unsigned int width,height,depth,colors,y,i,percent,shown=101;
    const char *problem=NULL;
    BPTR dir;
    APTR old;
    old=((struct Process *)FindTask(NULL))->pr_WindowPtr;
    ((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
    dir=Lock((STRPTR)s->drawer,ACCESS_READ);
    ((struct Process *)FindTask(NULL))->pr_WindowPtr=old;
    if (!dir) return "NO DRAWER";
    UnLock(dir);
    if (!pick_name(s->drawer,path,sizeof(path))) return "NAME FAILED";
    if (s->workbench) {
        workbench=LockPubScreen((STRPTR)"Workbench");
        if (!workbench) return "NO WORKBENCH";
    }
    lock=LockIBase(0);
    screen=workbench ? workbench : IntuitionBase->FirstScreen;
    if (!screen) {
        UnlockIBase(lock);
        return "NO SCREEN";
    }
    width=(unsigned int)screen->Width;height=(unsigned int)screen->Height;
    depth=(unsigned int)GetBitMapAttr(screen->RastPort.BitMap,BMA_DEPTH);
    mode=GetVPModeID(&screen->ViewPort);
    if (depth<1 || depth>8) problem="DEEP SCREEN";
    else if (width>ND_SHOT_MAX_WIDTH) problem="TOO WIDE";
    else {
        colors=1U<<depth;
        GetRGB32(screen->ViewPort.ColorMap,0,colors,rgb);
        copy=AllocBitMap(width,height,depth,0,screen->RastPort.BitMap);
        if (copy) {
            BltBitMap(screen->RastPort.BitMap,0,0,copy,0,0,(WORD)width,
                      (WORD)height,0xC0,0xFF,NULL);
            WaitBlit();
        } else problem="NO MEMORY";
    }
    UnlockIBase(lock);
    if (workbench) UnlockPubScreen(NULL,workbench);
    if (problem) return problem;
    colors=1U<<depth;
    for (i=0;i<colors*3U;++i) palette[i]=(unsigned char)(rgb[i]>>24);
    /* ReadPixelLine8 reads planar and chunky (RTG) bitmaps alike. */
    InitRastPort(&rp);rp.BitMap=copy;
    temp=rp;temp.Layer=NULL;
    line_map=AllocBitMap(width,1,depth,0,copy);
    line=(unsigned char *)AllocVec(((width+15U)>>4)<<4,MEMF_PUBLIC|MEMF_CLEAR);
    if (!line_map || !line) problem="NO MEMORY";
    else {
        temp.BitMap=line_map;
        if (!nd_shot_begin(&writer,path,width,height,depth,palette,colors,mode))
            problem="WRITE FAILED";
        for (y=0;!problem && y<height;++y) {
            ReadPixelLine8(&rp,0,(LONG)y,width,line,&temp);
            if (!nd_shot_row(&writer,line)) problem="WRITE FAILED";
            /* Keep the dock answered while a big screen is encoded. */
            percent=(y*10U/height)*10U;
            if (percent!=shown) {
                shown=percent;
                sprintf(progress,"SAVING %u%%",percent);
                (void)nd_client_update_quiet(client,"SHOT",progress,percent,0,500U);
            }
        }
        if (!nd_shot_end(&writer,path) && !problem) problem="WRITE FAILED";
    }
    if (line) FreeVec(line);
    WaitBlit();
    if (line_map) FreeBitMap(line_map);
    FreeBitMap(copy);
    if (!problem && saved) {
        strncpy(saved,path,saved_size-1);
        saved[saved_size-1]='\0';
    }
    return problem;
}
int main(int argc,char **argv)
{
    struct NDClient client;
    struct ShotSettings settings;
    char value[32],badge[8],saved[200];
    const char *problem;
    unsigned int event,elapsed=0,countdown=0,message=0,wait,taken=0;
    int reread=1;
    if (argc<2) return 20;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
    if (!IntuitionBase || !GfxBase || !nd_client_init(&client,argv[1])) {
        if (GfxBase) CloseLibrary((struct Library *)GfxBase);
        if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
        return 20;
    }
    strcpy(value,"READY");
    for (;;) {
        if (reread) {read_settings(argv[1],&settings);reread=0;}
        if (countdown) {
            countdown=countdown>elapsed ? countdown-elapsed : 0;
            sprintf(value,"IN %u",(countdown+49U)/50U);
            if (!countdown) {
                problem=capture(&client,&settings,saved,sizeof(saved));
                if (problem) strcpy(value,problem);
                else {
                    strcpy(value,"SAVED");
                    if (taken<99) ++taken;
                    sprintf(badge,"%u",taken);
                    (void)nd_client_set_badge(&client,badge);
                }
                message=250;
            }
        } else if (message) {
            message=message>elapsed ? message-elapsed : 0;
            if (!message) strcpy(value,"READY");
        }
        event=nd_client_update_quiet(&client,"SHOT",value,ND_NO_METER,elapsed,500U);
        if (event&ND_EVENT_QUIT) break;
        if (event&ND_EVENT_SETTINGS) reread=1;
        if (event&ND_EVENT_CLICK) {
            if (countdown) {countdown=0;strcpy(value,"CANCELLED");message=100;}
            else {
                /* Delay 0 still waits one tick, so the click is finished. */
                countdown=settings.delay ? settings.delay*50U : 1U;
                message=0;
                sprintf(value,"IN %u",settings.delay);
            }
        }
        wait=countdown ? (countdown<25U ? countdown : 25U) :
             message ? (message<50U ? message : 50U) :
             nd_client_delay_ticks(&client,500U);
        elapsed=nd_client_wait(&client,wait);
    }
    nd_client_close(&client);
    CloseLibrary((struct Library *)GfxBase);
    CloseLibrary((struct Library *)IntuitionBase);
    return 0;
}
