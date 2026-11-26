#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "nd_client.h"
#include "nd_task.h"
#include "nd_panel.h"
#include "nd_option_runtime.h"
#include "nd_plugin_versions.h"
#include "nd_pictures.h"
#include "nd_picture_view.h"
#if defined(ND_PLUGIN_RESOURCES)
#include "nd_plugin_resources.h"
#endif
const char nd_version[]=ND_PLUGIN_VER_STRING(PICTURES,"Pictures");long __stack=65536;
int nd_pictures_work(unsigned int action,void *data,unsigned int bytes,const char *parent);
static void settings(const char *id,struct NDPicturesJob *j,char label[32],unsigned long *interval)
{struct NDOptionString f[3];char delay[16];const char *key[]={"LABEL","DRAWER","INTERVAL"},*fallback[]={"PICTURES","","30"};char *out[3];unsigned int n,limit[]={32,240,16};out[0]=label;out[1]=j->drawer;out[2]=delay;for(n=0;n<3;++n){f[n].key=key[n];f[n].fallback=fallback[n];f[n].out=out[n];f[n].limit=limit[n];}nd_plugin_option_strings(id,f,3);n=(unsigned int)atoi(delay);*interval=n>=10&&n<=3600 ? n*50UL:1500UL;j->count=j->index=0;}
static struct NDPanel *details(struct NDPanel *p,const struct NDPicturesJob *j,const char *status,int paused)
{struct NDPanelField f[4];const char *b[]={"Previous","Next","Play/Pause","Rescan","Close"};char info[80];unsigned int i;sprintf(info,"%u / %u; %ux%u%s",j->count ? j->index+1:0,j->count,j->image.source_width,j->image.source_height,j->truncated ? "; first 32":"");f[0].label="Picture";f[0].value=j->count ? j->names[j->index]:"Choose a picture drawer in settings";f[1].label="Collection";f[1].value=info;f[2].label="Slideshow";f[2].value=paused ? "Paused":"Playing";f[3].label="Status";f[3].value=status;for(i=0;i<4;++i)f[i].limit=0;if(p){for(i=0;i<4;++i)nd_panel_set(p,i,f[i].value);return p;}return nd_panel_open("Pictures - bounded local slideshow",f,4,b,5);}
int main(int argc,char **argv)
{struct NDClient dock;struct NDTask task;struct NDTaskPacket *reply;struct NDPicturesJob *j;struct NDPanel *panel=NULL;struct NDPictureView *view=NULL;struct NDThumbnail thumb;char label[32],guard[64],status[96]="Choose a picture drawer",value[96];unsigned int event,elapsed=0,action=1,frame_due=1,changed=1;unsigned long interval=1500,wait=0,heartbeat=0;int ready=0,paused=0,button,ve;
 if(argc==3&&!strcmp(argv[1],"TASK"))return nd_task_worker(argv[2],nd_pictures_work);if(argc!=2)return 20;j=AllocVec(sizeof(*j),MEMF_PUBLIC|MEMF_CLEAR);if(!j)return 20;memset(&task,0,sizeof(task));memset(&thumb,0,sizeof(thumb));if(!nd_client_init(&dock,argv[1])){FreeVec(j);return 20;}((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;settings(argv[1],j,label,&interval);snprintf(guard,sizeof(guard),"NovaDock.Pictures.%.19s",argv[1]);
 for(;;){if(action&&!task.port&&!dock.throttled){if(j->drawer[0]&&strchr(j->drawer,':')){if(nd_task_start(&task,argv[0],guard,action,j,sizeof(*j))){action=0;strcpy(status,"Loading in background...");changed=1;}}else{action=0;strcpy(status,"Choose a picture drawer in tile settings");changed=1;}}
  reply=nd_task_poll(&task);if(reply){if(reply->bytes==sizeof(*j)){*j=*(struct NDPicturesJob *)reply->data;if(reply->result==1){ready=1;nd_picture_thumbnail(&j->image,&thumb);strcpy(status,j->truncated ? "First 32 pictures; Rescan checks the drawer":"Picture ready");if(view)nd_picture_view_set(view,&j->image,j->names[j->index]);}else{ready=0;memset(&thumb,0,sizeof(thumb));strncpy(status,j->problem[0] ? j->problem:"Picture worker failed",95);status[95]=0;nd_picture_view_close(view);view=NULL;}}
   else{ready=0;strcpy(status,"Picture worker returned invalid data");memset(&thumb,0,sizeof(thumb));}frame_due=changed=1;wait=interval;FreeVec(reply);}
  if(ready)snprintf(value,sizeof(value),"%.80s",j->names[j->index]);else strncpy(value,status,sizeof(value));value[95]=0;
  event=nd_client_update_art_quiet(&dock,label,value,"pictures",ready ? 0:1,elapsed,250);if(event&ND_EVENT_QUIT)break;
  if(frame_due||heartbeat>=500){event|=nd_client_thumbnail(&dock,&thumb);frame_due=0;heartbeat=0;}else heartbeat+=elapsed;
  if(event&ND_EVENT_SETTINGS){nd_task_close(&task);settings(argv[1],j,label,&interval);ready=0;memset(&thumb,0,sizeof(thumb));frame_due=changed=1;action=1;nd_picture_view_close(view);view=NULL;}
  if(event&ND_EVENT_CLICK){if(panel)nd_panel_raise(panel);else panel=details(NULL,j,status,paused);if(ready){if(view)nd_picture_view_raise(view);else view=nd_picture_view_open(&j->image,j->names[j->index]);}}
  if(changed&&panel){details(panel,j,status,paused);changed=0;}button=nd_panel_event(panel);ve=nd_picture_view_event(view);if(button==-1||button==104||ve==1){nd_panel_close(panel);panel=NULL;nd_picture_view_close(view);view=NULL;}if(button==102||ve==4){paused=!paused;changed=1;wait=interval;}
  if(!task.port){if((button==100||ve==2)&&j->count){j->index=j->index ? j->index-1:j->count-1;action=2;}else if((button==101||ve==3)&&j->count){j->index=(j->index+1)%j->count;action=2;}else if(button==103)action=1;
   else if(!paused&&!dock.throttled&&j->count>1){if(wait>elapsed)wait-=elapsed;else{j->index=(j->index+1)%j->count;action=2;wait=interval;}}}
  elapsed=nd_client_wait_signals(&dock,task.port||panel||view ? 10:250,nd_task_signals(&task)|nd_panel_signals(panel)|nd_picture_view_signals(view));
 }
 memset(&thumb,0,sizeof(thumb));nd_client_thumbnail(&dock,&thumb);nd_task_close(&task);nd_panel_close(panel);nd_picture_view_close(view);nd_client_close(&dock);FreeVec(j);return 0;}
