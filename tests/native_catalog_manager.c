/* Isolated Workbench only: exercise actual async Preferences windows and workers. */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <stdio.h>
#include <string.h>
#include "nd_reply.h"
struct IntuitionBase *IntuitionBase=NULL;
long __stack=65536;
static struct Screen *screen;
static unsigned long ticks(void){struct DateStamp d;DateStamp(&d);return d.ds_Days*4320000UL+d.ds_Minute*3000UL+d.ds_Tick;}
static struct Window *window(const char *title){struct Window *w;for(w=screen->FirstWindow;w;w=w->NextWindow)if(w->Title&&w->UserPort&&!strncmp(w->Title,title,strlen(title)))return w;return NULL;}
static int wait_window(const char *title,int ready,unsigned long limit)
{unsigned long begin=ticks();struct Window *w;int ok;do{Forbid();w=window(title);ok=w&&(!ready||!strcmp(w->Title,title));Permit();if(ok)return 1;Delay(1);}while(ticks()-begin<limit);printf("FAIL: missing %s (%s).\n",title,ready ? "complete":"open");return 0;}
static int event(const char *title,ULONG kind,UWORD id)
{struct Window *w;struct Gadget *g=NULL;struct MsgPort *port=nd_reply_port();struct IntuiMessage *m;int i,found=0;if(!port)return 0;m=AllocVec(sizeof(*m),MEMF_PUBLIC|MEMF_CLEAR);if(!m)return 0;Delay(5);Forbid();w=window(title);if(w){if(id)for(g=w->FirstGadget;g&&g->GadgetID!=id;g=g->NextGadget){}if(!id||g){m->ExecMessage.mn_Length=sizeof(*m);m->ExecMessage.mn_ReplyPort=port;m->Class=kind;m->IAddress=g;m->IDCMPWindow=w;PutMsg(w->UserPort,(struct Message *)m);found=1;}}Permit();if(!found){FreeVec(m);nd_reply_delete(port);return 0;}for(i=0;i<1000;++i){if(GetMsg(port))break;Delay(1);}if(i==1000){nd_reply_abandon(port);return 0;}Delay(2);FreeVec(m);nd_reply_delete(port);return 1;}
static int workers(void)
{struct Node *n;int count=0;Forbid();for(n=SysBase->PortList.lh_Head;n->ln_Succ;n=n->ln_Succ)if(n->ln_Name&&(!strncmp(n->ln_Name,"NovaDock.Catalog.",17)||!strncmp(n->ln_Name,"NovaDock.Task.",14)))++count;Permit();return count;}
static int finished(const char *log)
{unsigned long begin=ticks();BPTR file;do{file=Open((STRPTR)log,MODE_READWRITE);if(file){Close(file);if(!workers())return 1;}Delay(5);}while(ticks()-begin<1000);puts("FAIL: Preferences/scan worker did not finish after closing.");return 0;}
static int start(int serial,char log[80])
{char cmd[200];BPTR input,output;LONG rc;sprintf(log,"AutoRun:manager-%d.log",serial);input=Open("NIL:",MODE_OLDFILE);output=Open(log,MODE_NEWFILE);if(!input||!output)return 0;strcpy(cmd,"QATests:After/NovaDockPrefs PAGE=PLUGINS");rc=SystemTags(cmd,SYS_Input,input,SYS_Output,output,SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE);if(rc<0){Close(input);Close(output);}return rc>=0;}
int main(void)
{BPTR marker,dir;char log[80];unsigned long begin;const char *plugins="NovaDock Preferences - Plugins",*general="NovaDock Preferences - General";int result=10;
 setvbuf(stdout,NULL,_IONBF,0);marker=Lock("QATests:ISOLATED-QA-ONLY",ACCESS_READ);if(!marker)return 10;UnLock(marker);IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);if(!IntuitionBase)return 10;screen=LockPubScreen("Workbench");if(!screen)return 10;
 printf("SCREEN %d x %d, depth %d\n",screen->Width,screen->Height,screen->RastPort.BitMap->Depth);
 SystemTags("QATests:After/NovaDock QUIT",TAG_DONE);Delay(100);dir=CreateDir("RAM:CatalogEnv");if(dir)UnLock(dir);dir=CreateDir("RAM:CatalogArchive");if(dir)UnLock(dir);if(SystemTags("Assign ENV: RAM:CatalogEnv",TAG_DONE)||SystemTags("Assign ENVARC: RAM:CatalogArchive",TAG_DONE))goto done;
 begin=ticks();if(!start(1,log)||!wait_window(plugins,0,750))goto done;printf("MANAGER window_ticks %lu\n",ticks()-begin);if(!wait_window(plugins,1,6000))goto done;puts("PASS: Plugins opens before background inventory completes, then publishes a complete list.");if(!event(plugins,IDCMP_CLOSEWINDOW,0)||!finished(log))goto done;
 if(!start(2,log)||!wait_window(plugins,1,6000)||!event(plugins,IDCMP_GADGETUP,621))goto done;
 begin=ticks();if(!event(plugins,IDCMP_GADGETUP,601)||!wait_window(general,0,500))goto done;printf("MANAGER navigation_ticks %lu\n",ticks()-begin);if(!event(general,IDCMP_CLOSEWINDOW,0)||!finished(log))goto done;puts("PASS: navigation stays responsive during forced revalidation; worker ownership is reclaimed.");
 if(!start(3,log)||!wait_window(plugins,0,750))goto done;begin=ticks();if(!event(plugins,IDCMP_CLOSEWINDOW,0)||!finished(log))goto done;printf("MANAGER close_ticks %lu\n",ticks()-begin);puts("PASS: closing during discovery leaves no catalogue guard or reply port.");result=0;
 done:UnlockPubScreen(NULL,screen);CloseLibrary((struct Library *)IntuitionBase);return result;}
