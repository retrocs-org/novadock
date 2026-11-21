/* Disposable Workbench integration checks; no user files or network. */
#include "dock_internal.h"
#include "nd_task.h"
#include "nd_client.h"
#include "nd_panel.h"
#include "nd_agenda_job.h"
#include "nd_pictures.h"
#include "nd_picture_view.h"
#include "nd_bench.h"
struct IntuitionBase *IntuitionBase=NULL;extern struct GfxBase *GfxBase;
long __stack=65536;
static struct NDTaskPacket *job(struct NDTask *t,const char *program,unsigned int action,void *data,unsigned int bytes)
{struct NDTaskPacket *m=NULL;int i;printf("Starting %s action %u bytes %u\n",program,action,bytes);fflush(stdout);if(!nd_task_start(t,program,"NovaDock.QA.NewFeatures",action,data,bytes))return NULL;for(i=0;i<1500&&!m;++i){m=nd_task_poll(t);if(!m)Delay(2);}printf("Reply result %d bytes %u\n",m ? m->result:-99,m ? m->bytes:0);fflush(stdout);return m;}
static int command(const char *text)
{BPTR in=Open("NIL:",MODE_OLDFILE),out=Open("NIL:",MODE_NEWFILE);LONG rc;if(!in||!out)return 0;rc=SystemTags((STRPTR)text,SYS_Input,in,SYS_Output,out,NP_StackSize,65536UL,TAG_DONE);Close(in);Close(out);return rc==0;}
static int fake_host(void)
{struct MsgPort *port;struct NDMessage *m;int quit=0;Forbid();if(FindPort(ND_PORT_NAME)){Permit();return 10;}Permit();port=CreateMsgPort();if(!port)return 10;port->mp_Node.ln_Name=ND_PORT_NAME;port->mp_Node.ln_Pri=ND_HOST_ABI;AddPort(port);
 while(!quit){WaitPort(port);while((m=(struct NDMessage *)GetMsg(port))!=NULL){m->event=0;if(m->exec_msg.mn_Length>=sizeof(struct NDExtMessage))((struct NDExtMessage *)m)->event2=strcmp(m->value,"NO") ? ND_EVENT2_HOST_THUMBNAIL:0;if(m->command==ND_MSG_THUMBNAIL)m->event=m->exec_msg.mn_Length==sizeof(struct NDThumbnailMessage)&&nd_thumbnail_valid(&((struct NDThumbnailMessage *)m)->picture) ? ND_EVENT_CLICK:ND_EVENT_QUIT;if(m->command==ND_MSG_GOODBYE)quit=1;ReplyMsg((struct Message *)m);}}
 RemPort(port);DeleteMsgPort(port);return 0;}
static int thumbnail_ipc(struct NDThumbnail *picture)
{struct NDClient client;BPTR in,out;LONG rc;int i,ok=0;if(!nd_client_init(&client,"PICTURES_QA"))return 0;in=Open("NIL:",MODE_OLDFILE);out=Open("NIL:",MODE_NEWFILE);if(!in||!out)goto done;
 rc=SystemTags("QATests:FeatureCheck HOST",SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE);if(rc<0){Close(in);Close(out);goto done;}for(i=0;i<100;++i){struct MsgPort *host;Forbid();host=FindPort(ND_PORT_NAME);Permit();if(host)break;Delay(2);}if(i==100)goto done;
 if(nd_client_update(&client,"PICTURES","NO",ND_NO_METER)&ND_EVENT_QUIT)goto done;if(client.host_thumbnail||nd_client_thumbnail(&client,picture)!=ND_EVENT_NONE)goto done;
 if(nd_client_update(&client,"PICTURES","YES",ND_NO_METER)&ND_EVENT_QUIT)goto done;if(!client.host_thumbnail||nd_client_thumbnail(&client,picture)!=ND_EVENT_CLICK)goto done;
 picture->pixels[0]=16;if(nd_client_thumbnail(&client,picture)!=ND_EVENT_NONE)goto done;picture->pixels[0]=0;ok=1;
 done:nd_client_close(&client);return ok;}
int main(int argc,char **argv)
{struct NDTask task;struct NDTaskPacket *m=NULL;struct NDAgendaJob *a=NULL;struct NDPicturesJob *p=NULL;struct NDDock *d=NULL;struct NDPictureView *v=NULL;struct NDPanel *panel=NULL;struct Screen *screen=NULL;struct NDThumbnail thumb;struct RastPort rp;struct BitMap *normal,*tiny;ULONG before[12],after[12];unsigned int i,k;int ok=0,locked=0;BPTR dir;char percent[24],date[16];struct NDPanelField fields[]={{"Task","Task with a recurring due date",96},{"Due date","2026-11-21",11},{"Time","12:30",6},{"Repeat","MONTHLY",8},{"Status","YYYY-MM-DD; NONE, DAILY, WEEKLY or MONTHLY",0}};const char *buttons[]={"New","Edit","Complete","Remove","Close"};
 if(argc==2&&!strcmp(argv[1],"HOST"))return fake_host();
 memset(&task,0,sizeof(task));a=AllocVec(sizeof(*a),MEMF_PUBLIC|MEMF_CLEAR);p=AllocVec(sizeof(*p),MEMF_PUBLIC|MEMF_CLEAR);d=AllocVec(sizeof(*d),MEMF_PUBLIC|MEMF_CLEAR);if(!a||!p||!d)goto done;
 dir=Lock("QATests:State",ACCESS_READ);if(!dir||!AssignLock("ENVARC",dir))goto done;
 strcpy(a->path,"ENVARC:NovaDock/Agenda/AGENDA_QA.data");m=job(&task,"QATests:Agenda",1,a,sizeof(*a));if(!m||m->result!=1||((struct NDAgendaJob *)m->data)->book.count)goto done;FreeVec(m);m=NULL;
 if(!nd_agenda_put(&a->book,-1,"Recurring task","2027-01-31","12:30","MONTHLY"))goto done;
 m=job(&task,"QATests:Agenda",2,a,sizeof(*a));if(!m||m->result!=1)goto done;FreeVec(m);m=NULL;memset(&a->book,0,sizeof(a->book));m=job(&task,"QATests:Agenda",1,a,sizeof(*a));if(!m||m->result!=1)goto done;*a=*(struct NDAgendaJob *)m->data;FreeVec(m);m=NULL;if(a->book.count!=1||strcmp(a->book.items[0].title,"Recurring task"))goto done;
 if(!nd_agenda_complete(&a->book,0,a->book.items[0].day))goto done;nd_agenda_date_text(a->book.items[0].day,date);if(strcmp(date,"2027-02-28"))goto done;
 if(!command("C:Lock QATests: ON"))goto done;locked=1;m=job(&task,"QATests:Agenda",2,a,sizeof(*a));if(!m||m->result==1)goto done;FreeVec(m);m=NULL;if(!command("C:Lock QATests: OFF"))goto done;locked=0;
 m=job(&task,"QATests:Agenda",1,a,sizeof(*a));if(!m||m->result!=1)goto done;*a=*(struct NDAgendaJob *)m->data;FreeVec(m);m=NULL;nd_agenda_date_text(a->book.items[0].day,date);if(strcmp(date,"2027-01-31"))goto done;
 puts("PASS: AGENDA actual workers load/save/reload and failed protected-volume saves preserve the previous recurring task.");
 strcpy(p->drawer,"QATests:Images");m=job(&task,"QATests:Pictures",1,p,sizeof(*p));if(!m||m->result!=1)goto done;*p=*(struct NDPicturesJob *)m->data;FreeVec(m);m=NULL;if(p->count!=2||p->image.source_width!=32||p->image.source_height!=24)goto done;
 nd_picture_thumbnail(&p->image,&thumb);if(!nd_thumbnail_valid(&thumb)||!thumbnail_ipc(&thumb))goto done;puts("PASS: actual Exec client negotiates thumbnail support, preserves legacy fallback and sends the exact bounded payload.");nd_picture_thumbnail(&p->image,&thumb);
 IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);if(!IntuitionBase||!GfxBase)goto done;screen=LockPubScreen("Workbench");if(!screen)goto done;d->screen=screen;GetRGB32(screen->ViewPort.ColorMap,0,4,before);
 if(!receive_thumbnail(d,0,&thumb))goto done;normal=thumbnail_bitmap(d,0,0);tiny=thumbnail_bitmap(d,0,1);if(!normal||!tiny||receive_thumbnail(d,0,&thumb))goto done;InitRastPort(&rp);rp.BitMap=normal;
 for(i=0;i<32;++i){LONG expected=FindColor(screen->ViewPort.ColorMap,(ULONG)thumb.rgb[thumb.pixels[i]][0]*0x01010101UL,(ULONG)thumb.rgb[thumb.pixels[i]][1]*0x01010101UL,(ULONG)thumb.rgb[thumb.pixels[i]][2]*0x01010101UL,-1);if(ReadPixel(&rp,i,0)!=expected)goto done;}
 GetRGB32(screen->ViewPort.ColorMap,0,4,after);if(memcmp(before,after,sizeof(before)))goto done;
 thumb.pixels[0]=16;if(receive_thumbnail(d,0,&thumb)||thumbnail_bitmap(d,0,0)!=normal)goto done;nd_picture_thumbnail(&p->image,&thumb);
 v=nd_picture_view_open(&p->image,"Picture preview QA");if(!v)goto done;for(i=0;i<5;++i){nd_picture_view_set(v,&p->image,"Picture preview QA");Delay(2);}nd_picture_view_close(v);v=NULL;
 panel=nd_panel_open("Agenda editor QA",fields,5,buttons,5);if(!panel)goto done;{struct Window *w=screen->FirstWindow;struct Gadget *g;unsigned int count=0;if(w->TopEdge+w->Height>screen->Height)goto done;for(g=w->FirstGadget;g;g=g->NextGadget)if(g->GadgetID>=100&&g->GadgetID<=104){if(g->TopEdge+g->Height>w->Height-w->BorderBottom)goto done;++count;}if(count!=5)goto done;}nd_panel_close(panel);panel=NULL;
 puts("PASS: PICTURES actual DataType worker decodes ILBM, cached 32/16-pixel dock images retain the screen palette, malformed frames are rejected, and viewer/editor buttons fit.");
 p->index=1;m=job(&task,"QATests:Pictures",2,p,sizeof(*p));if(!m||m->result==1||!strstr(((struct NDPicturesJob *)m->data)->problem,"oversized"))goto done;FreeVec(m);m=NULL;
 memset(&thumb,0,sizeof(thumb));if(!receive_thumbnail(d,0,&thumb)||thumbnail_bitmap(d,0,0)||thumbnail_bitmap(d,0,1))goto done;
 puts("PASS: oversized image headers are rejected before decoding; clearing a tile releases both cached bitmaps.");
 nd_bench_change(125,100,percent);if(strcmp(percent,"+25.0%"))goto done;nd_bench_change(75,100,percent);if(strcmp(percent,"-25.0%"))goto done;puts("PASS: BENCH native percentage comparisons report positive and negative changes.");ok=1;
 done:if(locked)command("C:Lock QATests: OFF");if(m)FreeVec(m);nd_task_close(&task);nd_panel_close(panel);nd_picture_view_close(v);if(d){for(k=0;k<ND_MAX_ITEMS;++k)release_thumbnail(d,k);FreeVec(d);}if(screen)UnlockPubScreen(NULL,screen);if(GfxBase)CloseLibrary((struct Library *)GfxBase);if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);if(a)FreeVec(a);if(p)FreeVec(p);if(!ok)puts("FAIL: native Agenda/Pictures integration did not finish.");return ok ? 0:10;}
