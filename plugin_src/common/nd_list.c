/* Shared native list picker; all visible space belongs to the scrollable list. */
#define IntuitionBase NDListIntuitionBase
#define GadToolsBase NDListGadToolsBase
#define GfxBase NDListGfxBase
#include <exec/types.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "nd_list.h"
struct IntuitionBase *IntuitionBase;struct Library *GadToolsBase;struct GfxBase *GfxBase;
static int users;
struct NDList {struct Screen *screen;struct Window *win;APTR vi;struct Gadget *gadgets,*last,*list,*filter,*buttons[9];char button_text[9][24];unsigned int button_count;int following;struct TextAttr font;struct List labels;struct Node nodes[ND_LIST_MAX];char rows[ND_LIST_MAX][160],query[80],base_title[80],title[112];int map[ND_LIST_MAX],count,shown,selected;ULONG seconds,micros;};
static int contains(const char *s,const char *q)
{const char *a,*b;for(;*s;++s){a=s;b=q;while(*a && *b && tolower((unsigned char)*a)==tolower((unsigned char)*b)){++a;++b;}if(!*b)return 1;}return !*q;}
static void rebuild(struct NDList *p)
{int i,selected=-1;if(p->win)GT_SetGadgetAttrs(p->list,p->win,NULL,GTLV_Labels,~0UL,TAG_DONE);NewList(&p->labels);p->shown=0;for(i=0;i<p->count;++i)if(contains(p->rows[i],p->query)){p->map[p->shown]=i;p->nodes[p->shown].ln_Name=p->rows[i];AddTail(&p->labels,&p->nodes[p->shown]);if(i==p->selected)selected=p->shown;++p->shown;}
 if(selected<0)p->selected=-1;sprintf(p->title,"%.79s - %d/%d",p->base_title,p->shown,p->count);
 if(p->win){GT_SetGadgetAttrs(p->list,p->win,NULL,GTLV_Labels,(ULONG)&p->labels,GTLV_Selected,(ULONG)(selected<0 ? ~0U:(unsigned int)selected),GTLV_Top,0,TAG_DONE);if(p->following && p->shown)GT_SetGadgetAttrs(p->list,p->win,NULL,GTLV_MakeVisible,p->shown-1,TAG_DONE);SetWindowTitles(p->win,p->title,(STRPTR)-1);}}
struct NDList *nd_list_open(const char *title,const char **buttons,unsigned int count)
{
 struct NDList *p;struct NewGadget ng;int width,height,top,font_h,button_h,rows,list_h,i,bw;
 if(!title || !count || count>9)return NULL;
 if(!users){IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GadToolsBase=OpenLibrary("gadtools.library",39);GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);}
 ++users;p=(struct NDList *)calloc(1,sizeof(*p));if(!p){if(--users==0){if(GfxBase)CloseLibrary((struct Library *)GfxBase);if(GadToolsBase)CloseLibrary(GadToolsBase);if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);GfxBase=NULL;GadToolsBase=NULL;IntuitionBase=NULL;}return NULL;}p->selected=-1;
 if(!IntuitionBase || !GadToolsBase || !GfxBase)goto fail;
 p->screen=LockPubScreen("Workbench");if(!p->screen)goto fail;
 width=p->screen->Width-12;if(width>620)width=620;height=p->screen->Height-p->screen->BarHeight-14;if(height>360)height=360;
 font_h=p->screen->RastPort.Font->tf_YSize;button_h=font_h+10;rows=((int)count+2)/3;top=p->screen->WBorTop+font_h+4;
 list_h=height-top-(font_h+12)-rows*(button_h+4)-12;if(width<280 || list_h<font_h*2)goto fail;
 p->font.ta_Name=p->screen->RastPort.Font->tf_Message.mn_Node.ln_Name;p->font.ta_YSize=(UWORD)font_h;p->font.ta_Style=0;p->font.ta_Flags=0;
 p->vi=GetVisualInfo(p->screen,TAG_DONE);if(!p->vi || !CreateContext(&p->gadgets))goto fail;p->last=p->gadgets;
 memset(&ng,0,sizeof(ng));ng.ng_TextAttr=&p->font;ng.ng_VisualInfo=p->vi;ng.ng_LeftEdge=62;ng.ng_TopEdge=(WORD)top;ng.ng_Width=(WORD)(width-72);ng.ng_Height=(WORD)(font_h+6);ng.ng_GadgetText="Filter";ng.ng_Flags=PLACETEXT_LEFT;ng.ng_GadgetID=1;
 p->filter=p->last=CreateGadget(STRING_KIND,p->last,&ng,GTST_MaxChars,79,TAG_DONE);if(!p->last)goto fail;
 NewList(&p->labels);ng.ng_LeftEdge=10;ng.ng_TopEdge+=(WORD)(font_h+12);ng.ng_Width=(WORD)(width-20);ng.ng_Height=(WORD)list_h;ng.ng_GadgetText=NULL;ng.ng_Flags=0;ng.ng_GadgetID=2;
 p->list=p->last=CreateGadget(LISTVIEW_KIND,p->last,&ng,GTLV_Labels,(ULONG)&p->labels,GTLV_ShowSelected,NULL,GTLV_Selected,~0UL,GTLV_ScrollWidth,16,TAG_DONE);if(!p->last)goto fail;
 top=height-rows*(button_h+4)-5;bw=(width-24)/3;
 for(i=0;i<(int)count;++i){ng.ng_LeftEdge=(WORD)(10+(i%3)*(bw+2));ng.ng_TopEdge=(WORD)(top+(i/3)*(button_h+4));ng.ng_Width=(WORD)bw;ng.ng_Height=(WORD)button_h;strncpy(p->button_text[i],buttons[i],23);ng.ng_GadgetText=p->button_text[i];ng.ng_Flags=PLACETEXT_IN;ng.ng_GadgetID=(UWORD)(100+i);p->buttons[i]=p->last=CreateGadget(BUTTON_KIND,p->last,&ng,TAG_DONE);if(!p->last)goto fail;}
 p->button_count=count;
 strncpy(p->base_title,title,sizeof(p->base_title)-1);strcpy(p->title,p->base_title);
 p->win=OpenWindowTags(NULL,WA_CustomScreen,(ULONG)p->screen,WA_Left,(p->screen->Width-width)/2,WA_Top,p->screen->BarHeight+4,WA_Width,width,WA_Height,height,WA_Title,(ULONG)p->title,WA_Gadgets,(ULONG)p->gadgets,WA_Flags,WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_CLOSEGADGET|WFLG_ACTIVATE|WFLG_SMART_REFRESH,WA_IDCMP,LISTVIEWIDCMP|STRINGIDCMP|BUTTONIDCMP|IDCMP_GADGETUP|IDCMP_CLOSEWINDOW|IDCMP_VANILLAKEY|IDCMP_REFRESHWINDOW|IDCMP_INTUITICKS,TAG_DONE);
 if(!p->win)goto fail;GT_RefreshWindow(p->win,NULL);return p;
fail:nd_list_close(p);return NULL;
}
void nd_list_button(struct NDList *p,unsigned int index,const char *label)
{if(p && index<p->button_count && label && strcmp(p->button_text[index],label)){strncpy(p->button_text[index],label,23);p->button_text[index][23]=0;if(p->buttons[index]->GadgetText){p->buttons[index]->GadgetText->IText=p->button_text[index];p->buttons[index]->GadgetText->LeftEdge=(p->buttons[index]->Width-(int)strlen(p->button_text[index])*p->font.ta_YSize)/2;}RefreshGList(p->buttons[index],p->win,NULL,1);}}
void nd_list_follow(struct NDList *p,int following)
{if(p){p->following=following!=0;if(p->following && p->shown)GT_SetGadgetAttrs(p->list,p->win,NULL,GTLV_MakeVisible,p->shown-1,TAG_DONE);}}
int nd_list_following(const struct NDList *p){return p && p->following;}
void nd_list_close(struct NDList *p)
{if(!p)return;if(p->win)CloseWindow(p->win);if(p->gadgets)FreeGadgets(p->gadgets);if(p->vi)FreeVisualInfo(p->vi);if(p->screen)UnlockPubScreen(NULL,p->screen);free(p);if(--users==0){if(GfxBase)CloseLibrary((struct Library *)GfxBase);if(GadToolsBase)CloseLibrary(GadToolsBase);if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);GfxBase=NULL;GadToolsBase=NULL;IntuitionBase=NULL;}}
void nd_list_title(struct NDList *p,const char *status){char title[112];if(p && p->win){sprintf(title,"%.50s - %.32s",p->base_title,status);if(strcmp(title,p->title)){strcpy(p->title,title);SetWindowTitles(p->win,p->title,(STRPTR)-1);}}}
void nd_list_select(struct NDList *p,int index){if(p)p->selected=index;}
void nd_list_raise(struct NDList *p){if(p && p->win){WindowToFront(p->win);ActivateWindow(p->win);}}
void nd_list_rows(struct NDList *p,const char *const *rows,unsigned int count)
{unsigned int i;int changed=0;if(!p)return;if(count>ND_LIST_MAX)count=ND_LIST_MAX;if((int)count!=p->count)changed=1;for(i=0;i<count;++i){if(strncmp(p->rows[i],rows[i],159))changed=1;}if(changed && p->win)GT_SetGadgetAttrs(p->list,p->win,NULL,GTLV_Labels,~0UL,TAG_DONE);for(i=0;i<count;++i){strncpy(p->rows[i],rows[i],159);p->rows[i][159]=0;}p->count=(int)count;if(changed)rebuild(p);}
int nd_list_event(struct NDList *p)
{
 struct IntuiMessage *m;ULONG cls;UWORD code,id;ULONG sec,micro;int action=0;struct StringInfo *si;
 if(!p || !p->win)return 0;
 while((m=GT_GetIMsg(p->win->UserPort))!=NULL){cls=m->Class;code=m->Code;sec=m->Seconds;micro=m->Micros;id=cls==IDCMP_GADGETUP && m->IAddress ? ((struct Gadget *)m->IAddress)->GadgetID:0;GT_ReplyIMsg(m);
  if(cls==IDCMP_CLOSEWINDOW || (cls==IDCMP_VANILLAKEY && code==27))action=-1;
  else if(cls==IDCMP_GADGETUP && id>=100)action=id;
  else if(cls==IDCMP_GADGETUP && id==2 && code<p->shown){int next=p->map[code];p->following=0;if(next==p->selected && DoubleClick(p->seconds,p->micros,sec,micro))action=100;p->selected=next;p->seconds=sec;p->micros=micro;}
  else if(cls==IDCMP_VANILLAKEY && code==13 && p->selected>=0)action=100;
  else if(cls==IDCMP_REFRESHWINDOW){GT_BeginRefresh(p->win);GT_EndRefresh(p->win,TRUE);}
 }
 si=(struct StringInfo *)p->filter->SpecialInfo;if(si && strcmp(p->query,si->Buffer)){strncpy(p->query,si->Buffer,79);p->query[79]=0;rebuild(p);}
 return action;
}
int nd_list_selected(const struct NDList *p){return p ? p->selected:-1;}
unsigned long nd_list_signals(const struct NDList *p){return p && p->win ? 1UL<<p->win->UserPort->mp_SigBit:0;}
