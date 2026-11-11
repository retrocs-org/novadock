#ifndef ND_NETWORK_PICKER_H
#define ND_NETWORK_PICKER_H
#include <dos/dosextens.h>
#include "nd_interface_list.h"
struct NDNetworkPicker {
    struct NDInterfaceChoices choices;
    struct List labels;
    struct Node nodes[ND_INTERFACE_CHOICES+1];
    char text[ND_INTERFACE_CHOICES+1][144];
    struct NDPrefsLayout layout;
};
static int network_profile_driver_installed(const char *device)
{
    char path[192];BPTR lock;
    if(strchr(device,':') || strchr(device,'/')){
        if(strlen(device)>=sizeof(path))return 0;
        strcpy(path,device);
    }else{
        strcpy(path,"DEVS:Networks");
        if(!AddPart(path,(STRPTR)device,sizeof(path)))return 0;
    }
    lock=Lock(path,ACCESS_READ);if(!lock)return 0;
    UnLock(lock);return 1;
}
static void network_profiles_scan(struct NDInterfaceChoices *choices,const char *drawer,int storage)
{
    BPTR lock;struct FileInfoBlock *fib;struct NDInterfaceChoice entry;char path[192];
    lock=Lock((STRPTR)drawer,ACCESS_READ);if(!lock)return;
    fib=AllocDosObject(DOS_FIB,NULL);
    if(fib && Examine(lock,fib))while(ExNext(lock,fib)){
        if(fib->fib_DirEntryType>=0 || !nd_interface_name_valid(fib->fib_FileName))continue;
        strcpy(path,drawer);
        if(AddPart(path,fib->fib_FileName,sizeof(path)) &&
           nd_interface_choice_read(&entry,fib->fib_FileName,path,storage) &&
           network_profile_driver_installed(entry.device))
            (void)nd_interface_choice_add(choices,&entry);
    }
    if(fib)FreeDosObject(DOS_FIB,fib);UnLock(lock);
}
static void network_picker_field(struct PluginPrefs *p,struct NDPrefsLayout *layout,
                                  struct NewGadget *ng,int x,int y,int w,int h,int id)
{
    memset(ng,0,sizeof(*ng));ng->ng_LeftEdge=nd_prefs_layout_x(layout,x);
    ng->ng_TopEdge=nd_prefs_layout_y(layout,y);ng->ng_Width=nd_prefs_layout_w(layout,w);
    ng->ng_Height=nd_prefs_layout_h(layout,h);ng->ng_TextAttr=p->screen->Font;
    ng->ng_VisualInfo=p->visual;ng->ng_GadgetID=(UWORD)id;ng->ng_Flags=PLACETEXT_IN;
}
static void choose_network_profile(struct PluginPrefs *p,int row)
{
    struct NDNetworkPicker *picker;struct Gadget *gadgets=NULL,*last,*list,*g,*enabled[128];
    struct Window *win=NULL;struct NewGadget ng;struct IntuiMessage *im;
    int at=visible_start(p)+row,i,j,selected=0,done=0,accepted=0,enabled_count=0;
    int fy=p->screen->RastPort.Font ? p->screen->RastPort.Font->tf_YSize:8;
    int fx=p->screen->RastPort.Font ? p->screen->RastPort.Font->tf_XSize:8;
    int height=(p->screen->Height-p->screen->BarHeight)*8/(fy<8 ? 8:fy)-20;
    int last_selected=-1;ULONG old_flags,secs=0,micros=0;
    picker=AllocVec(sizeof(*picker),MEMF_PUBLIC|MEMF_REVERSE|MEMF_CLEAR);
    if(!picker){DisplayBeep(p->screen);return;}
    network_profiles_scan(&picker->choices,"DEVS:NetInterfaces",0);
    network_profiles_scan(&picker->choices,"SYS:Storage/NetInterfaces",1);
    picker->labels.lh_Head=(struct Node *)&picker->labels.lh_Tail;
    picker->labels.lh_Tail=NULL;picker->labels.lh_TailPred=(struct Node *)&picker->labels.lh_Head;
    strcpy(picker->text[0],"AUTO - detect active interface");
    for(i=0;i<=picker->choices.count;++i){
        if(i){
            struct NDInterfaceChoice *entry=&picker->choices.entry[i-1];
            const char *driver=FilePart(entry->device);
            sprintf(picker->text[i],"%s - %s%s",entry->name,driver,entry->storage ? " [Template]":"");
            if(!nd_interface_compare(entry->name,p->edits[at]))selected=i;
        }
        picker->nodes[i].ln_Name=picker->text[i];
        picker->nodes[i].ln_Pred=picker->labels.lh_TailPred;
        picker->nodes[i].ln_Succ=(struct Node *)&picker->labels.lh_Tail;
        picker->labels.lh_TailPred->ln_Succ=&picker->nodes[i];picker->labels.lh_TailPred=&picker->nodes[i];
    }
    if(height>260)height=260;if(height<120)goto finish;
    nd_prefs_layout_init(&picker->layout,fx,fy,p->screen->Width,p->screen->Height,
        p->screen->BarHeight,480,height);
    if(!nd_prefs_layout_fits(&picker->layout,8,4) || !CreateContext(&gadgets))goto finish;
    last=gadgets;
    network_picker_field(p,&picker->layout,&ng,12,12,456,16,0);
    last=CreateGadget(TEXT_KIND,last,&ng,GTTX_Text,(ULONG)(picker->choices.truncated ?
        "Profile list is full; you can still type a name in settings.":
        "Interfaces with installed drivers; Template = inactive profile."),TAG_DONE);
    if(!last)goto finish;
    network_picker_field(p,&picker->layout,&ng,12,34,456,height-76,1);
    list=last=CreateGadget(LISTVIEW_KIND,last,&ng,GTLV_Labels,(ULONG)&picker->labels,
        GTLV_ShowSelected,NULL,GTLV_Selected,(ULONG)selected,GTLV_MakeVisible,(ULONG)selected,GTLV_ScrollWidth,16,TAG_DONE);
    if(!last)goto finish;
    network_picker_field(p,&picker->layout,&ng,284,height-30,86,16,2);ng.ng_GadgetText=(STRPTR)"Select";
    last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);if(!last)goto finish;
    network_picker_field(p,&picker->layout,&ng,380,height-30,86,16,3);ng.ng_GadgetText=(STRPTR)"Cancel";
    last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);if(!last)goto finish;
    win=OpenWindowTags(NULL,WA_PubScreen,(ULONG)p->screen,
        WA_Left,nd_prefs_layout_left(&picker->layout),WA_Top,nd_prefs_layout_top(&picker->layout),
        WA_Width,picker->layout.window_width,WA_Height,picker->layout.window_height,
        WA_Title,(ULONG)"Select network interface",WA_Gadgets,(ULONG)gadgets,
        WA_DragBar,TRUE,WA_DepthGadget,TRUE,WA_CloseGadget,TRUE,WA_Activate,TRUE,
        WA_Flags,WFLG_SMART_REFRESH|WFLG_ACTIVATE,
        WA_IDCMP,LISTVIEWIDCMP|BUTTONIDCMP|IDCMP_RAWKEY|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW,TAG_DONE);
    if(!win)goto finish;
    old_flags=p->win->IDCMPFlags;
    for(g=p->gadgets;g && enabled_count<128;g=g->NextGadget)
        if(!(g->Flags&GFLG_DISABLED)){enabled[enabled_count++]=g;OffGadget(g,p->win,NULL);}
    ModifyIDCMP(p->win,IDCMP_REFRESHWINDOW);
    GT_RefreshWindow(win,NULL);
    while(!done){
        Wait((1UL<<win->UserPort->mp_SigBit)|(1UL<<p->win->UserPort->mp_SigBit));
        while((im=GT_GetIMsg(win->UserPort))!=NULL){
            ULONG kind=im->Class,now_secs=im->Seconds,now_micros=im->Micros;
            UWORD code=im->Code,id=kind==IDCMP_GADGETUP && im->IAddress ? ((struct Gadget *)im->IAddress)->GadgetID:0;
            GT_ReplyIMsg(im);
            if(kind==IDCMP_CLOSEWINDOW){done=1;}
            else if(kind==IDCMP_REFRESHWINDOW){GT_BeginRefresh(win);GT_EndRefresh(win,TRUE);}
            else if(kind==IDCMP_GADGETUP){
                if(id==1 && code<=picker->choices.count){
                    selected=(int)code;
                    if(last_selected==selected && DoubleClick(secs,micros,now_secs,now_micros)){accepted=done=1;}
                    last_selected=selected;secs=now_secs;micros=now_micros;
                }else if(id==2){accepted=done=1;}else if(id==3)done=1;
            }else if(kind==IDCMP_RAWKEY){
                if(code==0x45)done=1;
                else if(code==0x44){accepted=done=1;}
                else if(code==0x4c || code==0x4d){
                    if(code==0x4c && selected>0)--selected;
                    if(code==0x4d && selected<picker->choices.count)++selected;
                    GT_SetGadgetAttrs(list,win,NULL,GTLV_Selected,(ULONG)selected,GTLV_MakeVisible,(ULONG)selected,TAG_DONE);
                }
            }
        }
        while((im=GT_GetIMsg(p->win->UserPort))!=NULL){
            ULONG kind=im->Class;GT_ReplyIMsg(im);
            if(kind==IDCMP_REFRESHWINDOW){GT_BeginRefresh(p->win);GT_EndRefresh(p->win,TRUE);}
        }
    }
    CloseWindow(win);win=NULL;ModifyIDCMP(p->win,old_flags);
    for(j=0;j<enabled_count;++j)OnGadget(enabled[j],p->win,NULL);
    RefreshGList(p->gadgets,p->win,NULL,-1);
    if(accepted){
        strcpy(p->edits[at],selected ? picker->choices.entry[selected-1].name:"AUTO");
        GT_SetGadgetAttrs(p->fields[row],p->win,NULL,GTST_String,(ULONG)p->edits[at],TAG_DONE);
        SetWindowTitles(p->win,(STRPTR)p->title,(STRPTR)-1);
    }
    ActivateWindow(p->win);
finish:
    if(!done){SetWindowTitles(p->win,(STRPTR)"Could not open network interface list",(STRPTR)-1);DisplayBeep(p->screen);}
    if(win)CloseWindow(win);if(gadgets)FreeGadgets(gadgets);FreeVec(picker);
}
#endif
