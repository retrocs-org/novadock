#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_client.h"
#include "nd_task.h"
#include "nd_list.h"
#include "nd_panel.h"
#include "nd_option_runtime.h"
#include "nd_plugin_versions.h"
#include "nd_agenda_job.h"
#if defined(ND_PLUGIN_RESOURCES)
#include "nd_plugin_resources.h"
#endif
const char nd_version[]=ND_PLUGIN_VER_STRING(AGENDA,"Agenda");long __stack=65536;
static int work(unsigned int action,void *data,unsigned int bytes,const char *parent)
{struct NDAgendaJob *j=data;FILE *f;BPTR lock;LONG error;int ok;(void)parent;if(bytes!=sizeof(*j)||!memchr(j->path,0,sizeof(j->path))||strncmp(j->path,"ENVARC:NovaDock/Agenda/",sizeof("ENVARC:NovaDock/Agenda/")-1)||strstr(j->path,".."))return 0;
 if(action==1){f=fopen(j->path,"rb");if(!f){lock=Lock(j->path,ACCESS_READ);if(lock){UnLock(lock);return 0;}error=IoErr();if(error!=ERROR_OBJECT_NOT_FOUND&&error!=ERROR_DIR_NOT_FOUND)return 0;memset(&j->book,0,sizeof(j->book));return 1;}ok=nd_agenda_read(f,&j->book);if(fclose(f))ok=0;return ok;}
 if(action==2){lock=CreateDir("ENVARC:NovaDock");if(lock)UnLock(lock);lock=CreateDir("ENVARC:NovaDock/Agenda");if(lock)UnLock(lock);return nd_agenda_save(j->path,&j->book);}return 0;}
static int earlier(const struct NDAgendaItem *a,const struct NDAgendaItem *b)
{if(a->done!=b->done)return !a->done;if(a->day!=b->day)return a->day<0 ? 0:b->day<0 ? 1:a->day<b->day;if(a->minute!=b->minute)return a->minute<b->minute;return strcmp(a->title,b->title)<0;}
static void rows(struct NDList *list,const struct NDAgenda *a,unsigned int map[ND_AGENDA_MAX])
{char text[ND_AGENDA_MAX][160],date[16];const char *p[ND_AGENDA_MAX];unsigned int i,k,v;for(i=0;i<a->count;++i){map[i]=i;for(k=i;k&&earlier(&a->items[map[k]],&a->items[map[k-1]]);--k){v=map[k];map[k]=map[k-1];map[k-1]=v;}}
 for(i=0;i<a->count;++i){const struct NDAgendaItem *t=&a->items[map[i]];nd_agenda_date_text(t->day,date);sprintf(text[i],"[%c] %s %02u:%02u %.95s%s",t->done ? 'x':' ',date[0] ? date:"Undated",t->minute/60,t->minute%60,t->title,t->repeat ? " (repeat)":"");p[i]=text[i];}nd_list_rows(list,p,a->count);}
static struct NDPanel *editor(const struct NDAgendaItem *i,long today)
{char date[16],time[8];struct NDPanelField f[5];const char *b[]={"Save","Cancel"};nd_agenda_date_text(i ? i->day:today,date);sprintf(time,"%02u:%02u",i ? i->minute/60:0,i ? i->minute%60:0);
 f[0].label="Task";f[0].value=i ? i->title:"";f[0].limit=96;f[1].label="Due date";f[1].value=date;f[1].limit=11;f[2].label="Time";f[2].value=time;f[2].limit=6;f[3].label="Repeat";f[3].value=nd_agenda_repeat_text(i ? i->repeat:0);f[3].limit=8;f[4].label="Status";f[4].value="YYYY-MM-DD; NONE, DAILY, WEEKLY or MONTHLY";f[4].limit=0;return nd_panel_open(i ? "Edit Agenda task":"New Agenda task",f,5,b,2);}
static void editable(struct NDPanel *p,int enabled){unsigned int i;for(i=0;i<4;++i)nd_panel_enable(p,i,enabled);nd_panel_button(p,0,"Save",enabled);}
static int save(struct NDTask *t,const char *program,const char *guard,struct NDAgendaJob *j,struct NDPanel *p)
{if(!nd_task_start(t,program,guard,2,j,sizeof(*j)))return 0;editable(p,0);if(p)nd_panel_set(p,4,"Saving permanently...");return 1;}
int main(int argc,char **argv)
{struct NDClient dock;struct NDTask task;struct NDTaskPacket *reply;struct NDAgendaJob *j,*next;struct NDList *list=NULL;struct NDPanel *edit=NULL;struct DateStamp now;unsigned int event,elapsed=0,map[ND_AGENDA_MAX],due=0,i,minute=0,k;unsigned long timer=3000;long today=0;int ready=0,dirty=1,action,index=-1,edit_index=-1,reason=0,reminders=1,check=1;char status[96]="Loading saved tasks",label[32]="AGENDA",guard[64],badge[8],value[96]="Loading saved tasks",notify[8];const char *buttons[]={"New","Edit","Complete","Remove","Close"};
 if(argc==3&&!strcmp(argv[1],"TASK"))return nd_task_worker(argv[2],work);if(argc!=2)return 20;j=AllocVec(sizeof(*j),MEMF_PUBLIC|MEMF_CLEAR);next=AllocVec(sizeof(*next),MEMF_PUBLIC|MEMF_CLEAR);if(!j||!next){if(j)FreeVec(j);if(next)FreeVec(next);return 20;}memset(&task,0,sizeof(task));
 if(strncmp(argv[1],"AGENDA",6)||strlen(argv[1])>19)goto fail;for(i=0;argv[1][i];++i)if(!((argv[1][i]>='A'&&argv[1][i]<='Z')||(argv[1][i]>='0'&&argv[1][i]<='9')||argv[1][i]=='_'))goto fail;
 sprintf(j->path,"ENVARC:NovaDock/Agenda/%s.data",argv[1]);sprintf(guard,"NovaDock.Agenda.%s",argv[1]);if(!nd_client_init(&dock,argv[1]))goto fail;((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
 {struct NDOptionString f[2];f[0].key="LABEL";f[0].fallback="AGENDA";f[0].out=label;f[0].limit=32;f[1].key="REMINDERS";f[1].fallback="YES";f[1].out=notify;f[1].limit=8;nd_plugin_option_strings(argv[1],f,2);reminders=strcmp(notify,"NO")!=0;}
 if(!nd_task_start(&task,argv[0],guard,1,j,sizeof(*j)))strcpy(value,"Cannot start task loader; reopen AGENDA to retry");
 for(;;){DateStamp(&now);today=now.ds_Days;minute=(unsigned int)now.ds_Minute;
  reply=nd_task_poll(&task);if(reply){if(reply->result==1&&reply->bytes==sizeof(*j)){struct NDAgendaJob *saved=(struct NDAgendaJob *)reply->data;if(reason==3&&ready){for(i=0;i<saved->book.count;++i)if(saved->book.items[i].notified==saved->book.items[i].day){for(k=0;k<j->book.count;++k)if(j->book.items[k].id==saved->book.items[i].id&&j->book.items[k].notified!=saved->book.items[i].notified)event=nd_client_notify(&dock,"Agenda reminder",saved->book.items[i].title,1);}}
    *j=*saved;ready=1;dirty=1;strcpy(status,"Saved tasks ready");if(reason==1&&edit){nd_panel_close(edit);edit=NULL;}}
   else{strcpy(value,reply->action==1 ? "Cannot read tasks; saved data retained":"Save failed; previous tasks and edits retained");if(edit){editable(edit,1);nd_panel_set(edit,4,value);}}if(reply->result!=1)strcpy(status,value);reason=0;FreeVec(reply);}
  due=0;for(i=0;i<j->book.count;++i)if(nd_agenda_due(&j->book.items[i],today))++due;sprintf(badge,"%u",due);event=nd_client_set_badge(&dock,ready&&due ? badge:"");if(ready)sprintf(value,"%u due; %u tasks",due,j->book.count);event|=nd_client_update_art_quiet(&dock,label,value,"agenda",due ? 1:0,elapsed,250);if(event&ND_EVENT_QUIT)break;
  if(event&ND_EVENT_SETTINGS){struct NDOptionString f[2];f[0].key="LABEL";f[0].fallback="AGENDA";f[0].out=label;f[0].limit=32;f[1].key="REMINDERS";f[1].fallback="YES";f[1].out=notify;f[1].limit=8;nd_plugin_option_strings(argv[1],f,2);reminders=strcmp(notify,"NO")!=0;}
  if(event&ND_EVENT_CLICK){if(list)nd_list_raise(list);else{list=nd_list_open(label,buttons,5);dirty=1;}}
  if(dirty&&list){rows(list,&j->book,map);dirty=0;}nd_list_title(list,status);action=nd_list_event(list);index=nd_list_selected(list);if(index>=0&&(unsigned int)index<j->book.count)index=(int)map[index];else index=-1;
  if(action==-1||action==104){nd_list_close(list);list=NULL;}else if(ready&&!task.port&&!edit){if(action==100){nd_panel_close(edit);edit=editor(NULL,today);edit_index=-1;/* Preserve editor selection independently from the live list. */}
   else if(action==101&&index>=0){nd_panel_close(edit);edit=editor(&j->book.items[index],today);edit_index=index;}
   else if((action==102||action==103)&&index>=0){*next=*j;if((action==102 ? nd_agenda_complete(&next->book,(unsigned int)index,today):nd_agenda_remove(&next->book,(unsigned int)index))&&save(&task,argv[0],guard,next,NULL))reason=2;}}
  action=nd_panel_event(edit);if(action==-1||action==101){nd_panel_close(edit);edit=NULL;}else if(action==100&&ready&&!task.port){*next=*j;if(!nd_agenda_put(&next->book,edit_index,nd_panel_get(edit,0),nd_panel_get(edit,1),nd_panel_get(edit,2),nd_panel_get(edit,3)))nd_panel_set(edit,4,"Check task, date/time and NONE/DAILY/WEEKLY/MONTHLY");else if(save(&task,argv[0],guard,next,edit))reason=1;}
  if(elapsed>=timer){check=1;timer=3000;}else timer-=elapsed;
  if(check&&ready&&!task.port&&!edit&&reminders){int changed=0;*next=*j;for(i=0;i<next->book.count;++i)if(nd_agenda_remind(&next->book.items[i],today,minute)){next->book.items[i].notified=next->book.items[i].day;changed=1;}check=0;if(changed&&save(&task,argv[0],guard,next,NULL))reason=3;}
  elapsed=nd_client_wait_signals(&dock,edit||list||task.port ? 10:250,nd_task_signals(&task)|nd_panel_signals(edit)|nd_list_signals(list));
 }
 nd_task_close(&task);nd_panel_close(edit);nd_list_close(list);nd_client_close(&dock);FreeVec(j);FreeVec(next);return 0;
 fail:FreeVec(j);FreeVec(next);return 20;}
