/* Native guided report form; base layout also fits 640x256 Workbench. */
#define IntuitionBase NDSupportFormIntuitionBase
#define GadToolsBase NDSupportFormGadToolsBase
#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <gadgets/texteditor.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/texteditor.h>
#include <proto/alib.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "nd_prefs_layout.h"
#include "nd_support_report.h"
#include "nd_support.h"
#include "nd_control_client.h"
struct IntuitionBase *IntuitionBase;
struct Library *GadToolsBase,*TextFieldBase;
#define R_CANCEL 100
#define R_NEXT 101
#define R_BACK 102
#define R_SAVE 103
#define R_FREQUENCY 104
#define R_TYPE 105
#define R_DESCRIPTION 106
struct ReportForm {
    struct NDSupportReport *report;struct Screen *screen;struct Window *window;
    struct NDPrefsLayout layout;APTR visual;
    struct Gadget *gadgets,*last,*fields[5],*editor,*scroll,*next;
    char *values[5];unsigned int sizes[5];int page,next_disabled;
};
static void field(struct ReportForm *p,struct NewGadget *ng,int x,int y,int width,int height,int id,const char *label)
{
    memset(ng,0,sizeof(*ng));ng->ng_LeftEdge=nd_prefs_layout_x(&p->layout,x);ng->ng_TopEdge=nd_prefs_layout_y(&p->layout,y);
    ng->ng_Width=nd_prefs_layout_w(&p->layout,width);ng->ng_Height=nd_prefs_layout_h(&p->layout,height);
    ng->ng_GadgetID=(UWORD)id;ng->ng_GadgetText=(STRPTR)label;ng->ng_TextAttr=p->screen->Font;
    ng->ng_VisualInfo=p->visual;ng->ng_Flags=PLACETEXT_LEFT;
}
static int text_line(struct ReportForm *p,int y,const char *value)
{
    struct NewGadget ng;field(p,&ng,20,y,578,12,0,NULL);ng.ng_Flags=0;
    p->last=CreateGadget(TEXT_KIND,p->last,&ng,GTTX_Text,(ULONG)value,TAG_DONE);return p->last!=NULL;
}
static int string_field(struct ReportForm *p,int slot,int y,const char *label,char *value,unsigned int size)
{
    struct NewGadget ng;int x=p->page==3?184:164;field(p,&ng,x,y,598-x,18,slot+1,label);
    p->fields[slot]=p->last=CreateGadget(STRING_KIND,p->last,&ng,GTST_String,(ULONG)value,GTST_MaxChars,(ULONG)(size-1),GA_TabCycle,TRUE,TAG_DONE);
    p->values[slot]=value;p->sizes[slot]=size;return p->last!=NULL;
}
static int button(struct ReportForm *p,int x,int width,int id,const char *label)
{
    struct NewGadget ng;field(p,&ng,x,205,width,18,id,label);ng.ng_Flags=PLACETEXT_IN;
    p->last=CreateGadget(BUTTON_KIND,p->last,&ng,GA_Disabled,FALSE,TAG_DONE);
    if(id==R_NEXT){p->next=p->last;p->next_disabled=-1;}return p->last!=NULL;
}
static int can_next(struct ReportForm *p)
{
    if(p->page==1&&p->fields[2])return nd_support_report_has_summary(((struct StringInfo *)p->fields[2]->SpecialInfo)->Buffer);
    return 1;
}
static void update_next(struct ReportForm *p)
{int disabled=!can_next(p);if(p->next&&p->window&&disabled!=p->next_disabled){GT_SetGadgetAttrs(p->next,p->window,NULL,GA_Disabled,(ULONG)disabled,TAG_DONE);p->next_disabled=disabled;}}
static int detected_field(struct ReportForm *p,int x,int y,int width,const char *label,const char *value)
{
    struct NewGadget ng;field(p,&ng,x,y,width,18,0,label);
    p->last=CreateGadget(TEXT_KIND,p->last,&ng,GTTX_Text,(ULONG)value,GTTX_Border,TRUE,TAG_DONE);
    return p->last!=NULL;
}
static int remember_fields(struct ReportForm *p)
{
    int i;STRPTR text;
    for(i=0;i<5;++i)if(p->fields[i]){
        text=((struct StringInfo *)p->fields[i]->SpecialInfo)->Buffer;
        strncpy(p->values[i],text,p->sizes[i]-1);p->values[i][p->sizes[i]-1]=0;
    }
    if(p->editor){
        text=(STRPTR)DoGadgetMethod(p->editor,p->window,NULL,GM_TEXTEDITOR_ExportText,NULL);
        if(!text){SetWindowTitles(p->window,(STRPTR)"Could not read description - try again or Cancel",(STRPTR)-1);return 0;}
        if(strlen(text)>=sizeof(p->report->description)){
            FreeVec(text);SetWindowTitles(p->window,(STRPTR)"Description too long - maximum 8191 characters",(STRPTR)-1);return 0;
        }
        strcpy(p->report->description,text);FreeVec(text);
    }
    return 1;
}
static void close_page(struct ReportForm *p)
{
    if(p->window){
        if(p->editor)RemoveGList(p->window,p->editor,1);
        if(p->scroll)RemoveGList(p->window,p->scroll,1);
        CloseWindow(p->window);
    }
    p->window=NULL;
    if(p->editor)DisposeObject((Object *)p->editor);p->editor=NULL;
    if(p->scroll)DisposeObject((Object *)p->scroll);p->scroll=NULL;
    if(p->gadgets)FreeGadgets(p->gadgets);p->gadgets=p->last=p->next=NULL;memset(p->fields,0,sizeof(p->fields));
}
static int description_editor(struct ReportForm *p)
{
    static struct TagItem to_scroll[]={
        {GA_TEXTEDITOR_Prop_Entries,PGA_Total},{GA_TEXTEDITOR_Prop_Visible,PGA_Visible},
        {GA_TEXTEDITOR_Prop_First,PGA_Top},{TAG_DONE,0}};
    static struct TagItem to_editor[]={{PGA_Top,GA_TEXTEDITOR_Prop_First},{TAG_DONE,0}};
    if(!TextFieldBase)return 1;
    p->editor=(struct Gadget *)NewObject(TEXTEDITOR_GetClass(),NULL,
        GA_ID,R_DESCRIPTION,GA_Left,nd_prefs_layout_x(&p->layout,20),GA_Top,nd_prefs_layout_y(&p->layout,43),
        GA_Width,nd_prefs_layout_w(&p->layout,558),GA_Height,nd_prefs_layout_h(&p->layout,142),
        GA_RelVerify,TRUE,GA_TEXTEDITOR_Contents,(ULONG)p->report->description,
        GA_TEXTEDITOR_ImportHook,GV_TEXTEDITOR_ImportHook_Plain,GA_TEXTEDITOR_ExportHook,GV_TEXTEDITOR_ExportHook_Plain,
        GA_TEXTEDITOR_ExportWrap,0,GA_TEXTEDITOR_FixedFont,TRUE,TAG_DONE);
    if(!p->editor)return 0;
    p->scroll=(struct Gadget *)NewObject(NULL,"propgclass",GA_Left,nd_prefs_layout_x(&p->layout,582),
        GA_Top,nd_prefs_layout_y(&p->layout,43),GA_Width,nd_prefs_layout_w(&p->layout,16),GA_Height,nd_prefs_layout_h(&p->layout,142),
        PGA_Freedom,FREEVERT,PGA_NewLook,TRUE,PGA_Total,1,PGA_Visible,1,PGA_Top,0,
        ICA_TARGET,(ULONG)p->editor,ICA_MAP,(ULONG)to_editor,TAG_DONE);
    if(!p->scroll)return 0;
    SetAttrs((Object *)p->editor,ICA_TARGET,(ULONG)p->scroll,ICA_MAP,(ULONG)to_scroll,TAG_DONE);
    AddGList(p->window,p->editor,(UWORD)-1,1,NULL);AddGList(p->window,p->scroll,(UWORD)-1,1,NULL);
    RefreshGList(p->editor,p->window,NULL,1);RefreshGList(p->scroll,p->window,NULL,1);return 1;
}
static int open_page(struct ReportForm *p)
{
    struct NewGadget ng;struct NDSupportReport *r=p->report;struct NDSupportMachine *m=&r->machine;
    static STRPTR frequencies[]={"Not sure","Every time","Sometimes","Once",NULL};
    static STRPTR types[]={"Display / layout","Slow / unresponsive","Crash / freeze","Other / suggestion","Animation / magnify",NULL};
    static const char *const titles[]={"NovaDock Support Report - prepare (1/5)","NovaDock Support Report - details (2/5)",
        "NovaDock Support Report - description (3/5)","NovaDock Support Report - what happened (4/5)","NovaDock Support Report - system / save (5/5)"};
    static const char *const examples[]={
        "Example: Opened Preferences; part of the old window stayed visible.",
        "Example: Clicked Network settings; the window took 30 seconds.",
        "Example: Clicked Save, then NovaDock froze.",
        "Skip anything that does not apply to your suggestion or question.",
        "Example: Moved over RAM; icons overlapped the window behind."};
    if(!CreateContext(&p->gadgets))return 0;p->last=p->gadgets;
    if(p->page==0){
        if(!text_line(p,20,"Prepare your report"))return 0;
        field(p,&ng,164,38,434,18,R_TYPE,"Problem type:");
        p->last=CreateGadget(CYCLE_KIND,p->last,&ng,GTCY_Labels,(ULONG)types,GTCY_Active,(ULONG)r->type,TAG_DONE);
        if(!p->last||!text_line(p,68,"Choose the kind of issue you want to report."))return 0;
        if(!text_line(p,98,r->recording==2?"The 60-second recording is ready to include.":"Existing logs and any partial recording will be included.")||
           !text_line(p,120,"We collect system details and plugin activity automatically.")||
           !text_line(p,142,"Next, tell us what went wrong and review your report.")||
           !text_line(p,164,"You choose where to save the report.")||
           !text_line(p,183,"Files stay local. Nothing is uploaded automatically."))return 0;
    }else if(p->page==1){
        if(!text_line(p,20,"Contact and summary")||
           !string_field(p,0,45,"Name:",r->name,sizeof(r->name))||
           !string_field(p,1,73,"Email:",r->email,sizeof(r->email))||
           !string_field(p,2,101,"Summary *:",r->summary,sizeof(r->summary))||
           !string_field(p,3,157,"Setup notes:",r->hardware,sizeof(r->hardware)))return 0;
        field(p,&ng,164,129,434,18,R_FREQUENCY,"How often:");
        p->last=CreateGadget(CYCLE_KIND,p->last,&ng,GTCY_Labels,(ULONG)frequencies,GTCY_Active,(ULONG)r->frequency,TAG_DONE);if(!p->last)return 0;
        if(!text_line(p,183,"* Summary is required. Name and email are optional."))return 0;
    }else if(p->page==2){
        if(!text_line(p,20,"Detailed description (up to 8191 characters; multiple lines):"))return 0;
        if(!TextFieldBase&&(!text_line(p,67,"Multiline editing is unavailable on this system.")||
            !text_line(p,85,"You can describe the problem on the next page.")))return 0;
    }else if(p->page==3){
        if(!text_line(p,20,"Help us repeat it. Fill in what you know; blanks are OK.")||
           !string_field(p,0,45,"What did you do?",r->steps,sizeof(r->steps))||
           !string_field(p,3,73,"What went wrong?",r->actual,sizeof(r->actual))||
           !string_field(p,2,101,"What should happen?",r->expected,sizeof(r->expected))||
           !string_field(p,1,129,"When does it fail?",r->more_steps,sizeof(r->more_steps))||
           !string_field(p,4,157,"Changed anything?",r->changes,sizeof(r->changes))||
           !text_line(p,183,examples[r->type<5?r->type:3]))return 0;
    }else{
        if(!text_line(p,20,"Review your system details, then choose where to save.")||
           !detected_field(p,100,38,186,"CPU:",m->cpu)||!detected_field(p,438,38,160,"NovaDock:",m->app)||
           !detected_field(p,164,60,434,"AmigaOS/WB:",m->os)||
           !detected_field(p,100,82,186,"Kickstart:",m->kickstart)||!detected_field(p,438,82,160,"Exec:",m->exec)||
           !detected_field(p,164,104,434,"Chip RAM:",m->chip_ram)||
           !detected_field(p,164,126,434,"Fast RAM:",m->fast_ram)||
           !detected_field(p,164,148,434,"Graphics:",m->graphics)||
           !detected_field(p,164,170,434,"Mode name:",m->display_mode))return 0;
    }
    if(!button(p,20,110,R_CANCEL,"Cancel"))return 0;
    if(p->page>0&&!button(p,150,110,R_BACK,"< Back"))return 0;
    if(!button(p,p->page==4?438:478,p->page==4?160:120,p->page==4?R_SAVE:R_NEXT,p->page==4?"Save report...":"Next >"))return 0;
    p->window=OpenWindowTags(NULL,WA_PubScreen,(ULONG)p->screen,WA_Left,nd_prefs_layout_left(&p->layout),WA_Top,nd_prefs_layout_top(&p->layout),
        WA_Width,p->layout.window_width,WA_Height,p->layout.window_height,WA_Title,(ULONG)titles[p->page],
        WA_Gadgets,(ULONG)p->gadgets,WA_DragBar,TRUE,WA_CloseGadget,TRUE,WA_DepthGadget,TRUE,WA_Activate,TRUE,
        WA_IDCMP,STRINGIDCMP|CYCLEIDCMP|CHECKBOXIDCMP|BUTTONIDCMP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_RAWKEY|(p->page==1?IDCMP_INTUITICKS:0),WA_Flags,WFLG_SMART_REFRESH|WFLG_ACTIVATE,TAG_DONE);
    if(!p->window)return 0;update_next(p);GT_RefreshWindow(p->window,NULL);
    if(p->page==2&&!description_editor(p))return 0;return 1;
}
int nd_support_report_form_at(struct NDSupportReport *report,unsigned int page)
{
    struct ReportForm p;struct IntuiMessage *msg;ULONG cls;UWORD id,code;int action=-1,result=-1;
    memset(&p,0,sizeof(p));p.report=report;p.page=page<5?(int)page:0;
    if(!page){int capture=nd_support_capture(60U,1);report->recording=capture==0?2:capture==5?1:0;}
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GadToolsBase=OpenLibrary("gadtools.library",39);
    if(!IntuitionBase||!GadToolsBase)goto done;TextFieldBase=OpenLibrary("gadgets/texteditor.gadget",40);
    p.screen=LockPubScreen("Workbench");if(!p.screen)goto done;
    nd_prefs_layout_init(&p.layout,p.screen->RastPort.Font?p.screen->RastPort.Font->tf_XSize:8,p.screen->RastPort.Font?p.screen->RastPort.Font->tf_YSize:8,p.screen->Width,p.screen->Height,p.screen->BarHeight,620,239);
    if(!nd_prefs_layout_fits(&p.layout,10,12))goto done;
    p.visual=GetVisualInfo(p.screen,TAG_DONE);if(!p.visual)goto done;
    for(;;){
        if(!open_page(&p))goto done;action=-1;
        while(action<0){
            Wait(1UL<<p.window->UserPort->mp_SigBit);
            while((msg=GT_GetIMsg(p.window->UserPort))!=NULL){
                cls=msg->Class;code=msg->Code;id=msg->IAddress?((struct Gadget *)msg->IAddress)->GadgetID:0;GT_ReplyIMsg(msg);
                if(cls==IDCMP_CLOSEWINDOW || (cls==IDCMP_RAWKEY && code==0x45))action=R_CANCEL;
                else if(cls==IDCMP_INTUITICKS)update_next(&p);
                else if(cls==IDCMP_REFRESHWINDOW){GT_BeginRefresh(p.window);GT_EndRefresh(p.window,TRUE);}
                else if(cls==IDCMP_GADGETUP){
                    if(id==R_FREQUENCY)report->frequency=code<4?code:0;
                    else if(id==R_TYPE){report->type=code<5?code:3;action=R_TYPE;}
                    else if(id==R_CANCEL)action=R_CANCEL;
                    else if(id==R_BACK||id==R_NEXT||id==R_SAVE){
                        if(id==R_NEXT&&!can_next(&p))continue;
                        if(!remember_fields(&p))continue;
                        if(p.page==1&&id==R_NEXT&&!nd_support_report_valid(report))SetWindowTitles(p.window,(STRPTR)"Please enter a short problem summary",(STRPTR)-1);
                        else action=id;
                    }
                }
            }
        }
        close_page(&p);
        if(action==R_CANCEL){result=0;break;}if(action==R_SAVE){result=1;break;}
        if(action==R_NEXT)++p.page;else if(action==R_BACK)--p.page;
    }
done:
    close_page(&p);if(p.visual)FreeVisualInfo(p.visual);if(p.screen)UnlockPubScreen(NULL,p.screen);
    if(TextFieldBase)CloseLibrary(TextFieldBase);TextFieldBase=NULL;
    if(GadToolsBase)CloseLibrary(GadToolsBase);if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);
    GadToolsBase=NULL;IntuitionBase=NULL;return result;
}

int nd_support_report_form(struct NDSupportReport *report)
{return nd_support_report_form_at(report,0);}
