/* ToolTypes edits retain the loaded DiskObject and save through a sibling
 * temporary icon. Every successful save retains the exact previous file. */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <workbench/workbench.h>
#include <libraries/asl.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/asl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_client.h"
#include "nd_panel.h"
#include "nd_list.h"
#include "nd_task.h"
#include "nd_reply.h"
#include "nd_tooltypes.h"
#include "nd_plugin_versions.h"
const char nd_version[]=ND_PLUGIN_VER_STRING(TOOLTYPES,"ToolTypes");long __stack=65536;
struct Library *IconBase=NULL,*AslBase=NULL;
#define TT_LOAD 1
#define TT_SAVE 2
#define TT_BROWSE 3
#define TT_DROP_MAGIC 0x4e445454UL
struct TTData {struct NDToolTypes model;char status[160],backup[300];};
struct TTDrop {struct Message message;unsigned long magic;int result;char path[240];};
static struct TTData edit_data;
static int fingerprint(const char *path,unsigned long *hash,unsigned long *size)
{unsigned char bytes[1024];BPTR f=Open((STRPTR)path,MODE_OLDFILE);LONG n;unsigned long h=2166136261UL,total=0;if(!f)return 0;while((n=Read(f,bytes,sizeof(bytes)))>0){total+=(unsigned long)n;if(total>2097152UL){Close(f);return 0;}h=nd_tt_hash(h,bytes,(unsigned int)n);}Close(f);if(n<0)return 0;*hash=h;*size=total;return 1;}
static int load_icon(struct TTData *d)
{
 char base[240],path[260];struct DiskObject *icon;struct NDToolTypes *m=&d->model;unsigned long hash,size,again,again_size;unsigned int i;int ok=0;
 if(!nd_tt_path(m->path,base)){strcpy(d->status,"Choose an absolute icon path");return 0;}sprintf(path,"%s.info",base);
 if(!fingerprint(path,&hash,&size)){strcpy(d->status,"Cannot read icon (maximum 2 MiB)");return 0;}
 IconBase=OpenLibrary("icon.library",37);if(!IconBase){strcpy(d->status,"icon.library is unavailable");return 0;}
 icon=GetDiskObject(base);if(!icon){strcpy(d->status,"Cannot decode this Workbench icon");goto done;}
 if(icon->do_DefaultTool && strlen(icon->do_DefaultTool)>=sizeof(m->tool)){strcpy(d->status,"Default tool is too long; icon left unchanged");goto free_icon;}
 memset(m,0,sizeof(*m));strcpy(m->path,base);m->hash=hash;m->size=size;m->stack=icon->do_StackSize;m->type=icon->do_Type;if(icon->do_DefaultTool)strcpy(m->tool,icon->do_DefaultTool);
 if(icon->do_ToolTypes)for(i=0;icon->do_ToolTypes[i];++i){if(i==ND_TT_MAX || !nd_tt_line(icon->do_ToolTypes[i])){strcpy(d->status,"Too many/long ToolTypes; icon left unchanged");goto free_icon;}strcpy(m->lines[m->count++],icon->do_ToolTypes[i]);}
 if(!fingerprint(path,&again,&again_size) || again!=hash || again_size!=size){strcpy(d->status,"Icon changed while loading; try again");goto free_icon;}
 strcpy(d->status,"Loaded; changes are staged until Save");ok=1;
free_icon:FreeDiskObject(icon);
done:CloseLibrary(IconBase);IconBase=NULL;return ok;
}
static int parent_alive(const char *parent)
{struct MsgPort *p;if(!parent)return 1;Forbid();p=FindPort((STRPTR)parent);Permit();return p!=NULL;}
static int save_icon(struct TTData *d,const char *parent)
{
 static struct TTData original;struct NDToolTypes *m=&d->model;struct DiskObject *icon;struct FileInfoBlock *fib=NULL;BPTR lock;char path[260],temp[300],temp_info[310],number[32];STRPTR old_tool;STRPTR *old_types,*types[ND_TT_MAX+1];LONG old_stack;unsigned int i;int ok=0;
 original.model.path[0]=0;strcpy(original.model.path,m->path);
 if(!load_icon(&original)){strcpy(d->status,original.status);return 0;}
 if(m->hash!=original.model.hash || m->size!=original.model.size){strcpy(d->status,"Icon changed on disk; Reload before saving");return 0;}
 sprintf(number,"%lu",m->stack);if(!nd_tt_stack(number,&m->stack) || m->count>ND_TT_MAX || strlen(m->tool)>=sizeof(m->tool)){strcpy(d->status,"Invalid default tool or stack size");return 0;}
 for(i=0;i<m->count;++i)if(!nd_tt_line(m->lines[i])){strcpy(d->status,"Invalid ToolType line");return 0;}
 if(!nd_tt_preserves_art(&original.model,m)){strcpy(d->status,"NewIcons artwork entries must remain unchanged");return 0;}
 sprintf(path,"%s.info",m->path);IconBase=OpenLibrary("icon.library",37);if(!IconBase)return 0;icon=GetDiskObject(m->path);if(!icon)goto done;
 fib=AllocDosObject(DOS_FIB,NULL);lock=Lock(path,ACCESS_READ);if(lock){if(fib && !Examine(lock,fib)){FreeDosObject(DOS_FIB,fib);fib=NULL;}UnLock(lock);}
 old_tool=icon->do_DefaultTool;old_types=icon->do_ToolTypes;old_stack=icon->do_StackSize;
 for(i=0;i<m->count;++i)types[i]=m->lines[i];types[m->count]=NULL;icon->do_DefaultTool=m->tool[0] ? m->tool:NULL;icon->do_ToolTypes=types;icon->do_StackSize=m->stack;
 sprintf(temp,"%s.ndedit-%08lx",m->path,(ULONG)FindTask(NULL));sprintf(temp_info,"%s.info",temp);lock=Lock(temp_info,ACCESS_READ);if(lock){UnLock(lock);strcpy(d->status,"Temporary icon exists; try again");}else ok=PutDiskObject(temp,icon)!=0;
 icon->do_DefaultTool=old_tool;icon->do_ToolTypes=old_types;icon->do_StackSize=old_stack;FreeDiskObject(icon);icon=NULL;
 if(!ok){if(!lock)DeleteFile(temp_info);strcpy(d->status,"Could not write a complete temporary icon");goto done;}
 {unsigned long hash,size;if(!fingerprint(path,&hash,&size) || hash!=m->hash || size!=m->size){DeleteFile(temp_info);strcpy(d->status,"Icon changed during Save; Reload before saving");ok=0;goto done;}}
 for(i=1;i<=999;++i){if(i==1)sprintf(d->backup,"%s.bak",path);else sprintf(d->backup,"%s.bak%u",path,i);lock=Lock(d->backup,ACCESS_READ);if(!lock)break;UnLock(lock);}
 if(i>999 || !parent_alive(parent) || !Rename(path,d->backup)){DeleteFile(temp_info);strcpy(d->status,"Cannot preserve original icon; nothing replaced");ok=0;goto done;}
 if(!parent_alive(parent) || !Rename(temp_info,path)){if(Rename(d->backup,path)){DeleteFile(temp_info);strcpy(d->status,"Save cancelled/failed; original restored");}else strcpy(d->status,"Restore failed; original remains in backup file");ok=0;goto done;}
 if(fib){SetFileDate(path,&fib->fib_Date);SetComment(path,fib->fib_Comment);SetProtection(path,fib->fib_Protection);}
 ok=fingerprint(path,&m->hash,&m->size);strcpy(d->status,ok ? "Saved; previous icon retained in .info.bak*":"Saved; reload to check the new icon");
done:if(fib)FreeDosObject(DOS_FIB,fib);if(IconBase)CloseLibrary(IconBase);IconBase=NULL;return ok;
}
static int work(unsigned int action,void *data,unsigned int bytes,const char *parent)
{
 struct TTData *d=data;if(bytes!=sizeof(*d))return 0;
 if(action==TT_SAVE)return save_icon(d,parent);
 if(action==TT_BROWSE){struct FileRequester *f;int chosen=0;AslBase=OpenLibrary("asl.library",37);if(!AslBase){strcpy(d->status,"asl.library is unavailable");return 0;}f=AllocAslRequest(ASL_FileRequest,NULL);if(f){if(AslRequestTags(f,ASLFR_TitleText,(ULONG)"Choose a Workbench .info icon",ASLFR_RejectIcons,FALSE,ASLFR_DoPatterns,TRUE,ASLFR_InitialPattern,(ULONG)"#?.info",TAG_DONE)){strncpy(d->model.path,f->fr_Drawer,219);chosen=AddPart(d->model.path,f->fr_File,sizeof(d->model.path))!=0;}FreeAslRequest(f);}CloseLibrary(AslBase);AslBase=NULL;if(!chosen){strcpy(d->status,"Selection cancelled");return 0;}}
 return action==TT_LOAD || action==TT_BROWSE ? load_icon(d):0;
}
static int helper(const char *id,const char *path)
{struct TTDrop *m;struct MsgPort *reply,*target;char port[80];int i,ok=0;m=AllocVec(sizeof(*m),MEMF_PUBLIC|MEMF_CLEAR);reply=nd_reply_port();if(!m || !reply){if(m)FreeVec(m);if(reply)nd_reply_delete(reply);return 20;}if(!nd_tt_path(path,m->path)){FreeVec(m);nd_reply_delete(reply);return 20;}sprintf(port,"NovaDock.ToolTypes.%s",id);m->magic=TT_DROP_MAGIC;m->message.mn_Length=sizeof(*m);m->message.mn_ReplyPort=reply;Forbid();target=FindPort(port);if(target)PutMsg(target,(struct Message *)m);Permit();if(target){for(i=0;i<250;i+=5){if(GetMsg(reply)){ok=m->result;break;}Delay(5);}if(i==250){nd_reply_abandon(reply);return 10;}}FreeVec(m);nd_reply_delete(reply);return ok ? 0:10;}
static struct NDPanel *open_panel(int loaded)
{char stack[32];struct NDPanelField f[4];static const char *b[]={"Load","Browse...","ToolTypes...","Save"};sprintf(stack,"%lu",edit_data.model.stack);f[0].label="Icon path";f[0].value=edit_data.model.path;f[0].limit=220;f[1].label="Default tool";f[1].value=loaded ? edit_data.model.tool:"";f[1].limit=160;f[2].label="Stack bytes";f[2].value=loaded ? stack:"4096";f[2].limit=20;f[3].label="Status";f[3].value=edit_data.status;f[3].limit=0;return nd_panel_open("Workbench icon ToolTypes",f,4,b,4);}
int main(int argc,char **argv)
{
 struct NDClient dock;struct NDTask job;struct NDTaskPacket *reply;struct NDPanel *panel=NULL,*editor=NULL;struct NDList *list=NULL;struct MsgPort *drops;struct TTDrop *drop;unsigned int event,elapsed=0,i;int action,selected,edit_index=-1,loaded=0,fields_valid=1;char port[80],guard[80],value[32];const char *rows[ND_TT_MAX];static const char *buttons[]={"Add","Edit","Remove","Save","Reload","Close"};
 if(argc==3 && !strcmp(argv[1],"TASK"))return nd_task_worker(argv[2],work);
 if(argc==4 && !strcmp(argv[2],"EDIT"))return helper(argv[1],argv[3]);
 if(argc==3 && !strcmp(argv[1],"SELFTEST") && !strncmp(argv[2],"QATests:",8)){static struct TTData test;strcpy(test.model.path,argv[2]);if(!load_icon(&test) || test.model.count==ND_TT_MAX)return 10;strcpy(test.model.lines[test.model.count++],"NOVADOCK_QA=YES");strcpy(test.model.tool,"C:IconX");test.model.stack=16384;if(!save_icon(&test,NULL))return 10;return load_icon(&test) && test.model.stack==16384 && !strcmp(test.model.tool,"C:IconX") ? 0:10;}
 if(argc!=2 || strlen(argv[1])>19 || !nd_client_init(&dock,argv[1]))return 20;((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;memset(&job,0,sizeof(job));strcpy(edit_data.status,"Drop a Workbench icon, or Browse");sprintf(port,"NovaDock.ToolTypes.%s",argv[1]);sprintf(guard,"NovaDock.ToolTypes.Work.%s",argv[1]);drops=CreateMsgPort();if(!drops){nd_client_close(&dock);return 20;}drops->mp_Node.ln_Name=port;Forbid();if(FindPort(port)){Permit();DeleteMsgPort(drops);nd_client_close(&dock);return 10;}AddPort(drops);Permit();
 for(;;){
  reply=nd_task_poll(&job);if(reply){struct TTData *d=(struct TTData *)reply->data;strcpy(edit_data.status,d->status);if(reply->result==1){edit_data=*d;loaded=1;nd_panel_close(editor);editor=NULL;nd_panel_close(panel);panel=open_panel(1);}else if(panel)nd_panel_title(panel,d->status);FreeVec(reply);}
  while((drop=(struct TTDrop *)GetMsg(drops))!=NULL){drop->result=0;if(drop->magic==TT_DROP_MAGIC && drop->message.mn_Length==sizeof(*drop) && !job.port){struct TTData request;memset(&request,0,sizeof(request));strncpy(request.model.path,drop->path,219);drop->result=nd_task_start(&job,argv[0],guard,TT_LOAD,&request,sizeof(request));}nd_reply_finish((struct Message *)drop);}
  sprintf(value,loaded ? "%u ToolTypes":"Drop .info or click",edit_data.model.count);event=nd_client_update_art_quiet(&dock,"TOOLTYPES",value,"tooltypes",0,elapsed,250);if(event&ND_EVENT_QUIT)break;if(event&ND_EVENT_CLICK){if(panel)nd_panel_raise(panel);else panel=open_panel(loaded);}
  action=nd_panel_event(panel);if(action==-1){nd_panel_close(panel);panel=NULL;}
  else if(action==100 || action==101){if(job.port)nd_panel_title(panel,"An icon worker is still busy");else{struct TTData request;memset(&request,0,sizeof(request));strncpy(request.model.path,nd_panel_get(panel,0),219);nd_task_start(&job,argv[0],guard,action==101 ? TT_BROWSE:TT_LOAD,&request,sizeof(request));}}
  else if(action==102){if(job.port)nd_panel_title(panel,"Wait for the icon worker before editing");else if(!loaded)nd_panel_title(panel,"Load an icon first");else if(list)nd_list_raise(list);else list=nd_list_open("ToolTypes; artwork entries are protected",buttons,6);}
  if(panel && loaded){strncpy(edit_data.model.tool,nd_panel_get(panel,1),159);fields_valid=nd_tt_stack(nd_panel_get(panel,2),&edit_data.model.stack);if(!fields_valid && action==103){nd_panel_title(panel,"Stack must be 0 or 4096 to 1048576 bytes");action=0;}}
  if(action==103){if(!loaded)nd_panel_title(panel,"Load an icon first");else if(!nd_task_start(&job,argv[0],guard,TT_SAVE,&edit_data,sizeof(edit_data)))nd_panel_title(panel,"An icon worker is still busy");else{nd_list_close(list);list=NULL;nd_panel_close(editor);editor=NULL;}}
  for(i=0;i<edit_data.model.count;++i)rows[i]=edit_data.model.lines[i];nd_list_rows(list,rows,loaded ? edit_data.model.count:0);
  action=nd_list_event(list);selected=nd_list_selected(list);if(action==-1 || action==105){nd_list_close(list);list=NULL;}
  else if((action==100 || action==101) && !editor){if(action==100 && edit_data.model.count==ND_TT_MAX)nd_list_title(list,"Maximum 64 ToolTypes");else if(action==100 || selected>=0){edit_index=action==100 ? (int)edit_data.model.count:selected;if(edit_index<(int)edit_data.model.count && nd_tt_protected(edit_data.model.lines[edit_index]))nd_list_title(list,"NewIcons artwork cannot be edited");else{struct NDPanelField f={"ToolType",edit_index<(int)edit_data.model.count ? edit_data.model.lines[edit_index]:"",ND_TT_LINE};static const char *b[]={"Apply","Cancel"};editor=nd_panel_open("Edit ToolType; parentheses disable an entry",&f,1,b,2);}}}
  else if(action==102 && selected>=0){if(nd_tt_protected(edit_data.model.lines[selected]))nd_list_title(list,"NewIcons artwork cannot be removed");else{for(i=(unsigned int)selected+1;i<edit_data.model.count;++i)strcpy(edit_data.model.lines[i-1],edit_data.model.lines[i]);--edit_data.model.count;}}
  else if(action==103 && loaded){if(!fields_valid)nd_list_title(list,"Correct the stack size before Save");else if(nd_task_start(&job,argv[0],guard,TT_SAVE,&edit_data,sizeof(edit_data))){nd_list_close(list);list=NULL;nd_panel_close(editor);editor=NULL;}}
  else if(action==104 && loaded)nd_task_start(&job,argv[0],guard,TT_LOAD,&edit_data,sizeof(edit_data));
  action=nd_panel_event(editor);if(action==-1 || action==101){nd_panel_close(editor);editor=NULL;}else if(action==100 || action==1){if(!nd_tt_line(nd_panel_get(editor,0)))nd_panel_title(editor,"Enter one non-empty ToolType (159 characters max)");else{strcpy(edit_data.model.lines[edit_index],nd_panel_get(editor,0));if(edit_index==(int)edit_data.model.count)++edit_data.model.count;nd_panel_close(editor);editor=NULL;}}
  if(panel)for(i=0;i<3;++i)nd_panel_enable(panel,i,!job.port);
  if(job.busy && panel)nd_panel_title(panel,"Icon worker is busy; dock remains responsive");
  elapsed=nd_client_wait_signals(&dock,panel || list || job.port ? 10:250,nd_panel_signals(panel)|nd_panel_signals(editor)|nd_list_signals(list)|nd_task_signals(&job)|(1UL<<drops->mp_SigBit));
 }
 Forbid();RemPort(drops);Permit();while((drop=(struct TTDrop *)GetMsg(drops))!=NULL){drop->result=0;nd_reply_finish((struct Message *)drop);}DeleteMsgPort(drops);nd_task_close(&job);nd_list_close(list);nd_panel_close(editor);nd_panel_close(panel);nd_client_close(&dock);return 0;
}
