/* Native read-only details helper, launched separately from the sampler.
 * ShowNetStatus uses numeric output (no NAMES / reverse DNS lookups).
 */
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>

struct IntuitionBase *IntuitionBase=NULL;
struct Library *GadToolsBase=NULL;
#define ND_DETAIL_ROWS 192
#define ND_DETAIL_COLS 96
struct NetDetails {
    struct List lines;
    struct Node nodes[ND_DETAIL_ROWS];
    char text[ND_DETAIL_ROWS][ND_DETAIL_COLS];
    int count,cols;
};
static void detail_line(struct NetDetails *p,const char *text)
{
    int n,i;
    do {
        if(p->count>=ND_DETAIL_ROWS)return;
        n=(int)strlen(text);if(n>p->cols)n=p->cols;
        for(i=0;i<n;++i)p->text[p->count][i]=
            (unsigned char)text[i]<32 ? ' ':text[i];
        p->text[p->count][n]=0;
        p->nodes[p->count].ln_Name=p->text[p->count];
        AddTail(&p->lines,&p->nodes[p->count]);++p->count;text+=n;
    }while(*text);
}
static int detail_command(struct NetDetails *p,const char *cmd)
{
    char path[80],line[256];BPTR in,out;FILE *file;LONG rc=20;int n,start=p->count,unsupported=0;
    sprintf(path,"T:NovaDockNetDetails-%08lx",(ULONG)FindTask(NULL));
    in=Open("NIL:",MODE_OLDFILE);out=Open(path,MODE_NEWFILE);
    if(in && out)rc=SystemTags((STRPTR)cmd,SYS_Input,in,SYS_Output,out,TAG_DONE);
    if(in)Close(in);if(out)Close(out);
    file=fopen(path,"r");
    if(file){
        while(p->count<ND_DETAIL_ROWS && fgets(line,sizeof(line),file)){
            n=(int)strlen(line);while(n && (line[n-1]=='\n' || line[n-1]=='\r'))line[--n]=0;
            if(strstr(line,"does not support the status query"))unsupported=1;
            detail_line(p,line);
        }
        fclose(file);
    }
    DeleteFile(path);
    if(unsupported){
        /* Discard the repeated command diagnostics and explain once. */
        while(p->count>start){RemTail(&p->lines);--p->count;}
        detail_line(p,"The active socket library cannot answer Roadshow status queries.");
        detail_line(p,"The active TCP/IP stack may not support Roadshow commands.");
        detail_line(p,"Live IP configuration is unavailable through Roadshow commands.");
        return 0;
    }
    if(rc!=0)detail_line(p,"Could not read this network information.");
    else if(p->count==start)detail_line(p,"No information returned.");
    return rc==0;
}
static void detail_load(struct NetDetails *p)
{
    struct NCConfig cfg;const struct StackBackend *backend;char line[256];
    p->count=0;
    p->lines.lh_Head=(struct Node *)&p->lines.lh_Tail;
    p->lines.lh_Tail=NULL;p->lines.lh_TailPred=(struct Node *)&p->lines.lh_Head;
    load_config(&cfg);backend=backend_for(&cfg);
    if(network_stopping()){
        detail_line(p,"Roadshow shutdown is pending.");
        detail_line(p,"Close applications using the network and wait for shutdown to finish.");
        detail_line(p,"Network status queries are paused during shutdown.");return;
    }
    if(backend)backend->resolve(&cfg);
    sprintf(line,"Stack: %s",backend ? backend->name:"Not detected");detail_line(p,line);
    sprintf(line,"Interface: %s",cfg.interface_name);detail_line(p,line);
    if(cfg.traffic.device[0])sprintf(line,"Sampling driver: %s   Unit: %lu",cfg.traffic.device,(unsigned long)cfg.traffic.unit);
    else strcpy(line,"Traffic device: not configured");
    detail_line(p,line);
    sprintf(line,"Connection timeout: %u seconds",cfg.timeout);detail_line(p,line);
    if(!cfg.traffic.device[0] && strcmp(cfg.interface_name,"AUTO")){
        detail_line(p,"Device metadata unavailable for this interface.");
        detail_line(p,"Set SANA-II device in Network settings to enable traffic sampling.");
    }
    detail_line(p,"");
    if(!backend || !backend->available()){
        detail_line(p,"Roadshow commands were not found in C:.");return;
    }
    if(!backend->resident()){
        detail_line(p,"Network stack is offline.");
        detail_line(p,"Live IP addresses, routes and DNS are unavailable.");
        detail_line(p,"Use Online in the tile menu, then Refresh here.");return;
    }
    if(!exists("C:ShowNetStatus")){
        detail_line(p,"Install C:ShowNetStatus to display IP configuration.");return;
    }
    detail_line(p,"INTERFACES / IP CONFIGURATION");
    if(!detail_command(p,"C:ShowNetStatus INTERFACES"))return;
    if(strcmp(cfg.interface_name,"AUTO") && rd_valid_interface(cfg.interface_name)){
        sprintf(line,"C:ShowNetStatus INTERFACE=\"%s\"",cfg.interface_name);
        detail_command(p,line);
    }
    detail_line(p,"");detail_line(p,"ROUTES / GATEWAY");
    detail_command(p,"C:ShowNetStatus ROUTES");
    detail_line(p,"");detail_line(p,"DNS SERVERS");
    detail_command(p,"C:ShowNetStatus DNS");
    if(p->count==ND_DETAIL_ROWS){
        strcpy(p->text[ND_DETAIL_ROWS-1],"More information exists; showing first 192 lines.");
    }
}
static int network_details(void)
{
    struct NetDetails *p=NULL;struct Screen *screen=NULL;struct Window *win=NULL;
    struct Gadget *gadgets=NULL,*last,*list=NULL;struct NewGadget ng;
    struct MsgPort *single=NULL,*prior;struct IntuiMessage *im;APTR visual=NULL;
    int width,height,fx,fy,button_y,done=0,refresh=0,result=20;
    ULONG signals,kind;UWORD id,code;
    /* Find and claim the name in one Forbid(): two quick clicks must not
     * both open a window. */
    single=CreateMsgPort();if(!single)goto end;
    single->mp_Node.ln_Name="NovaDock.NetworkDetails";
    Forbid();prior=FindPort("NovaDock.NetworkDetails");
    if(prior){if(prior->mp_SigTask)Signal(prior->mp_SigTask,SIGBREAKF_CTRL_F);}
    else AddPort(single);
    Permit();
    if(prior){DeleteMsgPort(single);return 0;}
    ((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    GadToolsBase=OpenLibrary("gadtools.library",39);
    if(!IntuitionBase || !GadToolsBase)goto end;
    p=AllocVec(sizeof(*p),MEMF_PUBLIC|MEMF_CLEAR);if(!p)goto end;
    screen=LockPubScreen("Workbench");if(!screen)goto end;
    fx=screen->RastPort.Font->tf_XSize;fy=screen->RastPort.Font->tf_YSize;
    width=80*fx+40;if(width>screen->Width-16)width=screen->Width-16;
    height=20*fy+90;if(height>screen->Height-screen->BarHeight-8)height=screen->Height-screen->BarHeight-8;
    if(width<300 || height<140)goto end;
    p->cols=(width-56)/fx;if(p->cols>=ND_DETAIL_COLS)p->cols=ND_DETAIL_COLS-1;
    visual=GetVisualInfo(screen,TAG_DONE);if(!visual || !CreateContext(&gadgets))goto end;
    last=gadgets;memset(&ng,0,sizeof(ng));
    ng.ng_LeftEdge=12;ng.ng_TopEdge=screen->BarHeight+8;
    ng.ng_Width=width-24;ng.ng_Height=height-ng.ng_TopEdge-fy-32;
    ng.ng_GadgetID=1;ng.ng_TextAttr=screen->Font;ng.ng_VisualInfo=visual;
    detail_load(p);
    list=last=CreateGadget(LISTVIEW_KIND,last,&ng,GTLV_Labels,(ULONG)&p->lines,
        GTLV_ReadOnly,TRUE,GTLV_ScrollWidth,16,TAG_DONE);
    if(!last)goto end;
    button_y=height-fy-22;ng.ng_TopEdge=button_y;ng.ng_Width=12*fx;
    ng.ng_Height=fy+8;ng.ng_Flags=PLACETEXT_IN;
    ng.ng_LeftEdge=width-26*fx;ng.ng_GadgetID=2;ng.ng_GadgetText="Refresh";
    last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);if(!last)goto end;
    ng.ng_LeftEdge=width-13*fx;ng.ng_GadgetID=3;ng.ng_GadgetText="Close";
    last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);if(!last)goto end;
    win=OpenWindowTags(NULL,WA_PubScreen,(ULONG)screen,
        WA_Left,(screen->Width-width)/2,WA_Top,screen->BarHeight+1+(screen->Height-screen->BarHeight-height)/2,
        WA_Width,width,WA_Height,height,WA_Title,(ULONG)"NovaDock Network Details",
        WA_Gadgets,(ULONG)gadgets,WA_DragBar,TRUE,WA_CloseGadget,TRUE,
        WA_DepthGadget,TRUE,WA_Activate,TRUE,
        WA_IDCMP,LISTVIEWIDCMP|BUTTONIDCMP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_RAWKEY,
        WA_Flags,WFLG_SMART_REFRESH|WFLG_ACTIVATE,TAG_DONE);
    if(!win)goto end;GT_RefreshWindow(win,NULL);result=0;
    while(!done){
        signals=Wait((1UL<<win->UserPort->mp_SigBit)|SIGBREAKF_CTRL_C|SIGBREAKF_CTRL_F);
        if(signals&SIGBREAKF_CTRL_C)done=1;
        if(signals&SIGBREAKF_CTRL_F){WindowToFront(win);ActivateWindow(win);}
        while((im=GT_GetIMsg(win->UserPort))!=NULL){
            kind=im->Class;code=im->Code;
            id=im->IAddress ? ((struct Gadget *)im->IAddress)->GadgetID:0;
            GT_ReplyIMsg(im);
            if(kind==IDCMP_CLOSEWINDOW || (kind==IDCMP_RAWKEY && code==0x45))done=1;
            else if(kind==IDCMP_REFRESHWINDOW){GT_BeginRefresh(win);GT_EndRefresh(win,TRUE);}
            else if(kind==IDCMP_GADGETUP){if(id==3)done=1;else if(id==2)refresh=1;}
        }
        if(refresh && !done){
            SetWindowTitles(win,"NovaDock Network Details - refreshing",(STRPTR)-1);
            GT_SetGadgetAttrs(list,win,NULL,GTLV_Labels,~0UL,TAG_DONE);
            detail_load(p);
            GT_SetGadgetAttrs(list,win,NULL,GTLV_Labels,(ULONG)&p->lines,GTLV_Top,0,TAG_DONE);
            SetWindowTitles(win,"NovaDock Network Details",(STRPTR)-1);refresh=0;
        }
    }
end:
    if(win)CloseWindow(win);if(gadgets)FreeGadgets(gadgets);
    if(visual)FreeVisualInfo(visual);if(screen)UnlockPubScreen(NULL,screen);
    if(p)FreeVec(p);if(GadToolsBase)CloseLibrary(GadToolsBase);
    if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);
    if(single){RemPort(single);DeleteMsgPort(single);}return result;
}

static int network_error(int kind)
{
    struct EasyStruct easy;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    if(!IntuitionBase)return 20;
    memset(&easy,0,sizeof(easy));easy.es_StructSize=sizeof(easy);
    easy.es_Title="NovaDock Network";easy.es_GadgetFormat="OK";
    switch(kind){
    case 1:easy.es_TextFormat="Could not take the Roadshow stack offline.\nCheck Network settings and the Roadshow installation.";break;
    case 2:easy.es_TextFormat="Roadshow is still shutting down.\nApplications may be holding network resources.\nClose network applications; NovaDock will wait without querying the stack.";break;
    case 3:easy.es_TextFormat="A network operation is still running.\nNo second operation has been started.\nClose network applications and wait for the current operation.";break;
    case 4:easy.es_TextFormat="Online did not finish within the configured wait.\nThe background command is still running.\nNovaDock will wait before accepting another network operation.";break;
    case 5:easy.es_TextFormat="The offline operation has not replied within 15 seconds.\nThe background shutdown command is still running.\nNovaDock remains responsive and will wait for shutdown to finish.";break;
    case 8:easy.es_TextFormat="Could not start Roadshow log capture.\nCheck that C:NetLogViewer is installed.\nThe network operation was not started.";break;
    case 7:easy.es_TextFormat="Could not take the selected network interface offline.\nFull stack shutdown was not started.\nCheck the interface and ConfigureNetInterface installation.";break;
    case 6:easy.es_TextFormat="The network driver did not release its pending request.\nShutdown was not started.\nClose network applications and try again.";break;
    default:easy.es_TextFormat="Could not bring the Roadshow interface online.\nCheck the selected interface, device and Roadshow installation.";break;
    }
    EasyRequestArgs(NULL,&easy,NULL,NULL);
    CloseLibrary((struct Library *)IntuitionBase);return 0;
}
