/* Native asynchronous paged entity chooser using the AmiHASS service. */
#define IntuitionBase NDHassIntuitionBase
#define GadToolsBase NDHassGadToolsBase
#define AslBase NDHassAslBase
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/asl.h>
#include <clib/alib_protos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_hass_native.h"
#include "nd_hass_picker.h"
struct IntuitionBase *IntuitionBase=NULL;
struct Library *GadToolsBase=NULL,*AslBase=NULL;
struct Chooser {struct Screen *screen;struct Window *window;struct Gadget *gadgets,*list,*filter,*room,*status,*room_button,*extra;APTR visual;struct List labels;struct Node nodes[ND_HASS_PICKER_MAX];char lines[ND_HASS_PICKER_MAX][160],status_text[160],normal_title[96];struct NDHassPicker model;};
static void empty_labels(struct Chooser *p)
{p->labels.lh_Head=(struct Node *)&p->labels.lh_Tail;p->labels.lh_Tail=NULL;p->labels.lh_TailPred=(struct Node *)&p->labels.lh_Head;}
static void show(struct Chooser *p)
{
 int i,bias;struct NDHassPicker *m=&p->model;char *status=p->status_text;
 GT_SetGadgetAttrs(p->list,p->window,NULL,GTLV_Labels,~0UL,TAG_DONE);empty_labels(p);bias=m->devices==2 && !m->offset ? 1:0;
 if(bias){strcpy(p->lines[0],"All rooms (including unassigned)");p->nodes[0].ln_Name=p->lines[0];AddTail(&p->labels,&p->nodes[0]);}
 for(i=0;i<m->count;++i){sprintf(p->lines[i+bias],"%.40s  [%.80s]",m->items[i].name,m->items[i].id);p->nodes[i+bias].ln_Name=p->lines[i+bias];AddTail(&p->labels,&p->nodes[i+bias]);}
 GT_SetGadgetAttrs(p->list,p->window,NULL,GTLV_Labels,(ULONG)&p->labels,GTLV_Selected,~0UL,TAG_DONE);
 if(m->result==ND_HASS_OK){
  if(m->devices==2)sprintf(status,"%d rooms; page %d. Choose a room or All rooms.",m->total,m->offset/m->capacity+1);
  else if(!m->count)strcpy(status,m->devices ? "No devices found. Try Refresh or another filter.":"No matching entities. Try Refresh or another filter.");
  else sprintf(status,"%d %s; page %d. Select a row, then Choose.",m->total,m->devices ? "devices":"entities",m->offset/m->capacity+1);
 }else if(m->result==ND_HASS_UNSUPPORTED)strcpy(status,"Room/device browsing requires an updated AmiHASS service.");
 else strcpy(status,nd_hass_problem(m->result));
 GT_SetGadgetAttrs(p->status,p->window,NULL,GTTX_Text,(ULONG)status,TAG_DONE);m->dirty=0;
}
static int picker_selftest,picker_rooms_seen;
static void chooser_mode(struct Chooser *p,int mode,const char *device,const char *domain,const char *area,const char *filter,int capacity)
{
 nd_hass_client_close(&p->model.client);nd_hass_picker_init(&p->model,mode,device,domain);p->model.capacity=capacity-(mode==2 ? 1:0);nd_hass_picker_scope(&p->model,area,filter);
 GT_SetGadgetAttrs(p->room,p->window,NULL,GTST_String,(ULONG)area,GA_Disabled,(ULONG)(mode==2),TAG_DONE);
 GT_SetGadgetAttrs(p->room_button,p->window,NULL,GA_Disabled,(ULONG)(mode==2),TAG_DONE);
 GT_SetGadgetAttrs(p->filter,p->window,NULL,GTST_String,(ULONG)filter,GA_Disabled,(ULONG)(mode==2),TAG_DONE);
 SetWindowTitles(p->window,(STRPTR)(mode==2 ? "AmiHASS - choose room":p->normal_title),(STRPTR)-1);
 if(p->extra)GT_SetGadgetAttrs(p->extra,p->window,NULL,GA_Disabled,(ULONG)(mode==2),TAG_DONE);
}

static int choose(int devices,const char *device,const char *domain,char area[96],struct NDHassEntity *chosen)
{
 struct Chooser *p;struct NewGadget ng;struct Gadget *last;struct IntuiMessage *im;struct TextAttr font={(STRPTR)"topaz.font",8,0,0};
 char filter[64]="",saved_filter[64]="",selected_room[96],title[96];int capacity,test_ticks=0,i,w,h,top,result=0,done=0,button_count=nd_hass_global_type(domain) ? 5:6;ULONG cls;UWORD id,code;static const char *buttons[]={"Previous","Next","Choose","Refresh","Cancel","Devices..."};
 p=AllocVec(sizeof(*p),MEMF_PUBLIC|MEMF_CLEAR);if(!p)return 0;
 if(!nd_hass_picker_init(&p->model,devices,device,domain) || !nd_hass_picker_scope(&p->model,area,"")){FreeVec(p);return 0;}
 strcpy(selected_room,area);(void)nd_hass_start(NULL);
 IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GadToolsBase=OpenLibrary("gadtools.library",39);if(!IntuitionBase || !GadToolsBase)goto finished;
 p->screen=LockPubScreen("Workbench");if(!p->screen)goto finished;w=p->screen->Width-20;if(w>700)w=700;if(w<400)goto finished;top=p->screen->WBorTop+p->screen->Font->ta_YSize+8;h=p->screen->Height-p->screen->BarHeight-12;if(h>350)h=350;if(h<180)goto finished;
 p->visual=GetVisualInfo(p->screen,TAG_DONE);if(!p->visual)goto finished;last=CreateContext(&p->gadgets);if(!last)goto finished;empty_labels(p);
 memset(&ng,0,sizeof(ng));ng.ng_VisualInfo=p->visual;ng.ng_TextAttr=&font;ng.ng_LeftEdge=64;ng.ng_TopEdge=top;ng.ng_Width=w-80;ng.ng_Height=16;ng.ng_GadgetID=1;ng.ng_GadgetText="Filter";ng.ng_Flags=PLACETEXT_LEFT;
 p->filter=last=CreateGadget(STRING_KIND,last,&ng,GTST_String,(ULONG)filter,GTST_MaxChars,63,TAG_DONE);if(!last)goto finished;
 ng.ng_TopEdge=top+24;ng.ng_GadgetID=4;ng.ng_GadgetText="Room";
 ng.ng_Width=w-164;p->room=last=CreateGadget(STRING_KIND,last,&ng,GTST_String,(ULONG)p->model.area,GTST_MaxChars,95,TAG_DONE);if(!last)goto finished;
 ng.ng_LeftEdge=w-92;ng.ng_Width=76;ng.ng_GadgetID=5;ng.ng_GadgetText="Rooms...";ng.ng_Flags=PLACETEXT_IN;p->room_button=last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);if(!last)goto finished;
 ng.ng_LeftEdge=12;ng.ng_TopEdge=top+48;ng.ng_Width=w-24;ng.ng_Height=h-top-124;ng.ng_GadgetID=2;ng.ng_GadgetText=NULL;
 capacity=p->model.capacity=nd_hass_picker_rows(ng.ng_Height,font.ta_YSize);
 p->list=last=CreateGadget(LISTVIEW_KIND,last,&ng,GTLV_Labels,(ULONG)&p->labels,GTLV_ShowSelected,0,TAG_DONE);if(!last)goto finished;
 ng.ng_TopEdge=h-68;ng.ng_Height=16;ng.ng_GadgetID=3;p->status=last=CreateGadget(TEXT_KIND,last,&ng,GTTX_Text,(ULONG)"Loading from AmiHASS...",TAG_DONE);if(!last)goto finished;
 for(i=0;i<button_count;++i){ng.ng_LeftEdge=12+i*(w-24)/button_count;ng.ng_TopEdge=h-42;ng.ng_Width=(w-24)/button_count-5;ng.ng_Height=20;ng.ng_GadgetID=100+i;ng.ng_GadgetText=(STRPTR)(i==5 && devices ? "Entities":buttons[i]);ng.ng_Flags=PLACETEXT_IN;last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);if(!last)goto finished;if(i==5)p->extra=last;}
 if(nd_hass_global_type(domain))sprintf(title,"AmiHASS - choose %s",domain);else if(devices && domain && *domain && strcmp(domain,"All"))sprintf(title,"AmiHASS - choose %s device",domain);else strcpy(title,devices ? "AmiHASS - choose device":"AmiHASS - choose entity on device");
 strcpy(p->normal_title,title);p->window=OpenWindowTags(NULL,WA_Title,(ULONG)title,WA_PubScreen,(ULONG)p->screen,WA_Left,(p->screen->Width-w)/2,WA_Top,p->screen->BarHeight+4,WA_Width,w,WA_Height,h,WA_Flags,WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_ACTIVATE|WFLG_SMART_REFRESH,WA_IDCMP,LISTVIEWIDCMP|STRINGIDCMP|BUTTONIDCMP|IDCMP_CLOSEWINDOW|IDCMP_RAWKEY,WA_Gadgets,(ULONG)p->gadgets,TAG_DONE);if(!p->window)goto finished;GT_RefreshWindow(p->window,NULL);
 if(picker_selftest && devices)chooser_mode(p,2,NULL,domain,selected_room,"",capacity);
 while(!done){
  nd_hass_picker_step(&p->model);if(p->model.dirty)show(p);
  if(picker_selftest){if(++test_ticks>600)done=1;if(p->model.result==ND_HASS_OK){
   if(p->model.devices==2){for(i=0;i<p->model.count;++i)if(!strcmp(p->model.items[i].name,"Study")){strcpy(selected_room,p->model.items[i].id);picker_rooms_seen=1;break;}if(!picker_rooms_seen)done=1;else chooser_mode(p,devices,device,domain,selected_room,"",capacity);}
   else {if(p->model.count){*chosen=p->model.items[0];strcpy(area,p->model.area);result=1;}done=1;}
  }else if(p->model.result!=ND_HASS_WAIT && p->model.result!=ND_HASS_BUSY)done=1;}
  while((im=GT_GetIMsg(p->window->UserPort))!=NULL){cls=im->Class;code=im->Code;id=cls==IDCMP_GADGETUP && im->IAddress ? ((struct Gadget *)im->IAddress)->GadgetID:0;GT_ReplyIMsg(im);
   if(cls==IDCMP_CLOSEWINDOW || (cls==IDCMP_RAWKEY && code==0x45) ){done=1;break;}
   if(id==104){if(p->model.devices==2){strcpy(filter,saved_filter);chooser_mode(p,devices,device,domain,selected_room,filter,capacity);continue;}done=1;break;}
   if(cls==IDCMP_REFRESHWINDOW){GT_BeginRefresh(p->window);GT_EndRefresh(p->window,TRUE);}
   if(id==5){strncpy(saved_filter,((struct StringInfo *)p->filter->SpecialInfo)->Buffer,63);saved_filter[63]=0;chooser_mode(p,2,NULL,domain,selected_room,"",capacity);continue;}
   if(p->model.devices==2){
    if(id==2)p->model.selected=!p->model.offset && code==0 ? -2:(int)code-(!p->model.offset ? 1:0);
    if(id==102 && p->model.result==ND_HASS_OK){if(p->model.selected==-2)selected_room[0]=0;else if(p->model.selected>=0 && p->model.selected<p->model.count)strcpy(selected_room,p->model.items[p->model.selected].id);else continue;strcpy(filter,saved_filter);chooser_mode(p,devices,device,domain,selected_room,filter,capacity);continue;}
    if(id==100 && p->model.offset>=p->model.capacity)nd_hass_picker_request(&p->model,"",p->model.offset-p->model.capacity,0);
    if(id==101 && p->model.offset+p->model.count<p->model.total)nd_hass_picker_request(&p->model,"",p->model.offset+p->model.capacity,0);
    if(id==103)nd_hass_picker_request(&p->model,"",0,1);
    continue;
   }
   if(id>=100 && id<=105 && (strcmp(p->model.area,((struct StringInfo *)p->room->SpecialInfo)->Buffer) || strcmp(p->model.filter,((struct StringInfo *)p->filter->SpecialInfo)->Buffer))){
    strncpy(filter,((struct StringInfo *)p->filter->SpecialInfo)->Buffer,63);filter[63]=0;
    if(nd_hass_picker_scope(&p->model,((struct StringInfo *)p->room->SpecialInfo)->Buffer,filter)){strcpy(selected_room,p->model.area);continue;}
    GT_SetGadgetAttrs(p->status,p->window,NULL,GTTX_Text,(ULONG)"Invalid room; use a name or ID",TAG_DONE);continue;
   }
   if(id==2){p->model.selected=code<p->model.count ? code:-1;}
   if(id==1 || id==4){strncpy(filter,((struct StringInfo *)p->filter->SpecialInfo)->Buffer,sizeof(filter)-1);filter[63]=0;if(!nd_hass_picker_scope(&p->model,((struct StringInfo *)p->room->SpecialInfo)->Buffer,filter))GT_SetGadgetAttrs(p->status,p->window,NULL,GTTX_Text,(ULONG)"Enter a room name or ID; blank means all rooms",TAG_DONE);else strcpy(selected_room,p->model.area);}
   if(id==100 && p->model.offset>=p->model.capacity)nd_hass_picker_request(&p->model,filter,p->model.offset-p->model.capacity,0);
   if(id==101 && p->model.offset+p->model.count<p->model.total)nd_hass_picker_request(&p->model,filter,p->model.offset+p->model.capacity,0);
   if(id==102 && p->model.result==ND_HASS_OK && p->model.selected>=0 && p->model.selected<p->model.count){*chosen=p->model.items[p->model.selected];strcpy(area,p->model.area);result=1;done=1;}
   if(id==105){strcpy(area,p->model.area);result=devices ? 3:2;done=1;}
   if(id==103)nd_hass_picker_request(&p->model,filter,0,1);
  }Delay(5);
 }
finished:nd_hass_client_close(&p->model.client);if(p->window)CloseWindow(p->window);if(p->gadgets)FreeGadgets(p->gadgets);if(p->visual)FreeVisualInfo(p->visual);if(p->screen)UnlockPubScreen(NULL,p->screen);if(GadToolsBase)CloseLibrary(GadToolsBase);GadToolsBase=NULL;if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);IntuitionBase=NULL;FreeVec(p);return result;
}

int nd_hass_choose_device(const char *domain,char device[96],char name[64])
{struct NDHassEntity chosen;char area[96]="";if(choose(1,NULL,domain,area,&chosen)!=1)return 0;strcpy(device,chosen.id);nd_hass_device_name(name,chosen.name);return 1;}
int nd_hass_choose(const char *device,const char *domain,char entity[96])
{struct NDHassEntity chosen;char area[96]="";if(choose(0,device,domain,area,&chosen)!=1)return 0;strcpy(entity,chosen.id);return 1;}

int nd_hass_choose_target_area(const char *domain,char area[96],char device[96],char name[64],struct NDHassEntity *entity)
{
 char room[96],chosen_device[96],chosen_name[64];struct NDHassEntity picked;int result,all=0;
 if(!amihass_area_valid(area))return 0;strcpy(room,area);
 if(nd_hass_global_type(domain)){if(choose(0,"",domain,room,entity)!=1)return 0;strcpy(area,room);device[0]=name[0]=0;return 1;}
 strcpy(chosen_device,device);strcpy(chosen_name,name);
 for(;;){
  if(!all && !nd_hass_picker_device_id(chosen_device)){
   result=choose(1,NULL,domain,room,&picked);
   if(result==3){all=1;chosen_device[0]=chosen_name[0]=0;}
   else if(result==1){strcpy(chosen_device,picked.id);nd_hass_device_name(chosen_name,picked.name);}
   else return 0;
  }
  result=choose(0,chosen_device,domain,room,entity);
  if(result==1){strcpy(area,room);strcpy(device,chosen_device);strcpy(name,chosen_name);return 1;}
  if(result!=2)return 0;
  all=0;chosen_device[0]=0;
 }
}
int nd_hass_choose_target(const char *domain,char device[96],char name[64],struct NDHassEntity *entity)
{char area[96]="";return nd_hass_choose_target_area(domain,area,device,name,entity);}
int nd_hass_picker_selftest(void)
{char area[96]="Study",device[96]="",name[64]="";struct NDHassEntity e;int result;memset(&e,0,sizeof(e));picker_selftest=1;picker_rooms_seen=0;result=nd_hass_choose_target_area("light",area,device,name,&e);picker_selftest=0;return result && picker_rooms_seen && !strcmp(area,"study") && !strcmp(e.id,"light.amihass_sample_desk") ? 0:10;}
