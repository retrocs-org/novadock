/* On-demand public-screen picker. No screen scan runs while idle. */
#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <stdio.h>
#include <string.h>
#include "nd_plugin_versions.h"
#include "nd_client.h"
#include "../clipboard/nd_clipboard_popup.h"
#include "nd_screens.h"
const char nd_version[]=ND_PLUGIN_VER_STRING(SCREENSWITCHER,"ScreenSwitcher");
long __stack=16384;
struct IntuitionBase *IntuitionBase=NULL;
struct GfxBase *GfxBase=NULL;
static struct NDScreenEntry screens[ND_SCREEN_MAX];
static int open_screens(struct NDClipQuick *picker,int *count)
{
    struct NDClipHistory history;char labels[ND_SCREEN_MAX][160];int i;
    memset(&history,0,sizeof(history));*count=nd_screen_collect(screens);
    history.count=(unsigned int)*count;
    for(i=0;i<*count;++i){sprintf(labels[i],"%s\n%ux%u",screens[i].name,screens[i].width,screens[i].height);history.text[i]=labels[i];}
    return nd_clip_quick_open(picker,&history);
}
int main(int argc,char **argv)
{
    struct NDClient client;struct NDClipQuick *picker=NULL;
    unsigned int event,elapsed=0;int count=0,chosen,result=20;char value[32];
    if(argc<2)return 20;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
    if(!IntuitionBase || !GfxBase || !nd_client_init(&client,argv[1]))goto cleanup;
    picker=nd_clip_quick_create();if(!picker){nd_client_close(&client);goto cleanup;}
    nd_clip_quick_labels(picker,"SCREENS","No public screens available","NovaDock Screens","Click or Return to switch - Esc to close","Screen");
    strcpy(value,"Click to switch");
    for(;;){
        event=nd_client_update_quiet(&client,"SCREENS",value,ND_NO_METER,elapsed,500U);
        if(event&ND_EVENT_QUIT){result=0;break;}
        if(event&ND_EVENT_CLICK){
            if(nd_clip_quick_active(picker))nd_clip_quick_close(picker);
            else if(open_screens(picker,&count))strcpy(value,"Click to switch");
            else strcpy(value,"Picker unavailable");
        }
        if(nd_clip_quick_events(picker)){
            chosen=nd_clip_quick_selected(picker);nd_clip_quick_close(picker);
            if(chosen>=0 && chosen<count)
                strcpy(value,nd_screen_raise(screens[chosen].name) ? "Click to switch":"Screen closed");
        }
        elapsed=nd_client_wait_signals(&client,500U,nd_clip_quick_signals(picker));
    }
    nd_clip_quick_free(picker);nd_client_close(&client);
cleanup:
    if(GfxBase)CloseLibrary((struct Library *)GfxBase);
    if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);return result;
}
