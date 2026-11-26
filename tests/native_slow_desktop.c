/* Disposable SIMPLE_REFRESH backdrop, deliberately slower than three ticks. */
#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuitionbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <stdio.h>
struct IntuitionBase *IntuitionBase;struct GfxBase *GfxBase;long __stack=32768;
static void paint(struct Window *w){int x,y;for(y=0;y<w->Height;y+=8)for(x=0;x<w->Width;x+=16){SetAPen(w->RPort,1+((x/16)*5+(y/8)*13)%3);RectFill(w->RPort,x,y,x+15,y+7);}WaitBlit();}
int main(void){struct Screen *s=NULL;struct Window *w=NULL;struct MsgPort *p=NULL;struct IntuiMessage *m;ULONG sig;FILE *f;unsigned int count=0;BPTR guard=Lock("QATests:ISOLATED-QA-ONLY",ACCESS_READ);if(!guard)return 20;UnLock(guard);
 IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);if(!IntuitionBase||!GfxBase)goto done;s=LockPubScreen("Workbench");if(!s)goto done;
 w=OpenWindowTags(NULL,WA_PubScreen,(ULONG)s,WA_Left,0,WA_Top,s->BarHeight,WA_Width,s->Width,WA_Height,s->Height-s->BarHeight,WA_Backdrop,TRUE,WA_Borderless,TRUE,WA_Activate,FALSE,WA_SimpleRefresh,TRUE,WA_IDCMP,IDCMP_REFRESHWINDOW,TAG_DONE);if(!w)goto done;paint(w);
 p=CreateMsgPort();if(!p)goto done;p->mp_Node.ln_Name="NovaDock.QA.SlowDesktop";AddPort(p);
 for(;;){sig=Wait((1UL<<w->UserPort->mp_SigBit)|SIGBREAKF_CTRL_C);if(sig&SIGBREAKF_CTRL_C)break;while((m=(struct IntuiMessage *)GetMsg(w->UserPort))!=NULL){if(m->Class==IDCMP_REFRESHWINDOW){Delay(12);BeginRefresh(w);paint(w);EndRefresh(w,TRUE);++count;f=fopen("QATests:bg-count","w");if(f){fprintf(f,"%u",count);fclose(f);}}ReplyMsg((struct Message *)m);}}
 done:if(p){RemPort(p);DeleteMsgPort(p);}if(w)CloseWindow(w);if(s)UnlockPubScreen(NULL,s);if(GfxBase)CloseLibrary((struct Library *)GfxBase);if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);return 0;}
