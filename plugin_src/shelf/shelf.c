/* Persistent shortcuts only: removing/reordering never touches target files. */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/wb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_plugin_versions.h"
#include "nd_client.h"
#include "nd_reply.h"
#include "nd_list.h"
#include "nd_panel.h"
#include "nd_option_runtime.h"
#include "nd_shelf.h"
#include "nd_interaction.h"
struct Library *WorkbenchBase=NULL;
const char nd_version[]=ND_PLUGIN_VER_STRING(SHELF,"Shelf");long __stack=32768;
#define SHELF_MAGIC 0x53484c46UL
struct ShelfDrop {struct Message message;ULONG magic;LONG result;UWORD count;char paths[32][ND_SHELF_PATH];};
static struct NDShelf shelf,next_shelf;
static char storage[300],label[32],parent[240],status[32];static int writable=1;
static void make_parents(const char *path)
{char p[300];unsigned int i;BPTR l;strcpy(p,path);for(i=0;p[i];++i)if(p[i]=='/' && i){p[i]=0;l=Lock(p,ACCESS_READ);if(l)UnLock(l);else{l=CreateDir(p);if(l)UnLock(l);}p[i]='/';}}
static int load(const char *id)
{
 FILE *f;char old[304];struct NDOptionString fields[2];memset(&shelf,0,sizeof(shelf));
 fields[0].key="LABEL";fields[0].fallback="SHELF";fields[0].out=label;fields[0].limit=sizeof(label);
 fields[1].key="DRAWER";fields[1].fallback="ENVARC:NovaDock/Shelves";fields[1].out=parent;fields[1].limit=sizeof(parent);nd_plugin_option_strings(id,fields,2);
 if(!*parent || !strchr(parent,':') || strlen(parent)>230){strcpy(status,"Choose a storage drawer");return 0;}
 sprintf(storage,"%s%s%s.shelf",parent,parent[strlen(parent)-1]==':' || parent[strlen(parent)-1]=='/' ? "":"/",id);
 f=fopen(storage,"r");if(!f){BPTR lock=Lock(storage,ACCESS_READ);if(lock){UnLock(lock);strcpy(status,"Cannot read saved shelf");return 0;}strcpy(status,"Drop shortcuts here");return 1;}
 writable=nd_shelf_read(f,&shelf);fclose(f);
 if(!writable){sprintf(old,"%s.bak",storage);f=fopen(old,"r");if(f){writable=nd_shelf_read(f,&shelf);fclose(f);}}
 strcpy(status,writable ? "Shortcuts restored":"Saved shelf needs repair");return writable;
}
static int save(void)
{if(!writable)return 0;make_parents(storage);if(!nd_shelf_save(storage,&next_shelf)){strcpy(status,"Cannot save shelf");return 0;}shelf=next_shelf;strcpy(status,"Saved permanently");return 1;}
static int open_target(const char *path)
{BPTR lock;lock=Lock((STRPTR)path,ACCESS_READ);if(!lock){strcpy(status,"Shortcut target unavailable");return 0;}UnLock(lock);if(WorkbenchBase && WorkbenchBase->lib_Version>=44 && OpenWorkbenchObjectA((STRPTR)path,NULL))return 1;
 {char quoted[530],command[550];BPTR in,out;LONG rc;if(!nd_quote_path(quoted,sizeof(quoted),path))return 0;in=Open("NIL:",MODE_OLDFILE);out=Open("NIL:",MODE_NEWFILE);if(!in || !out){if(in)Close(in);if(out)Close(out);return 0;}sprintf(command,"C:WBRUN %s",quoted);rc=SystemTags(command,SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,TAG_DONE);if(rc==-1){Close(in);Close(out);strcpy(status,"Could not open shortcut");return 0;}}return 1;}
static int helper(const char *id,const char *verb,const char *path)
{
 struct ShelfDrop *m;struct MsgPort *reply,*target;struct Message *answer=NULL;char port[64],directory[300],*end;FILE *f;int ticks,result=10;BPTR lock;
 if(!nd_shelf_id(id))return 20;m=AllocVec(sizeof(*m),MEMF_PUBLIC|MEMF_CLEAR);reply=nd_reply_port();if(!m || !reply){if(m)FreeVec(m);if(reply)nd_reply_delete(reply);return 20;}
 if(!strcmp(verb,"PUT")){m->count=1;strncpy(m->paths[0],path,ND_SHELF_PATH-1);}
 else if(!strcmp(verb,"HANDLELIST") && !strncmp(path,"T:NovaDockShelfDrop-",sizeof("T:NovaDockShelfDrop-")-1) && strlen(path)<sizeof(directory)){
  f=fopen(path,"r");if(!f)goto done;while(m->count<32 && fgets(m->paths[m->count],ND_SHELF_PATH,f)){end=strchr(m->paths[m->count],'\n');if(!end){fclose(f);goto done;}*end=0;++m->count;}if(fgetc(f)!=EOF || ferror(f)){fclose(f);goto done;}fclose(f);
 }else goto done;
 sprintf(port,"NovaDock.Shelf.%s",id);m->magic=SHELF_MAGIC;m->message.mn_Length=sizeof(*m);m->message.mn_ReplyPort=reply;
 Forbid();target=FindPort(port);if(target)PutMsg(target,(struct Message *)m);Permit();if(!target)goto done;
 for(ticks=0;ticks<250 && !answer;ticks+=5){answer=GetMsg(reply);if(!answer)Delay(5);}
 if(!answer){nd_reply_abandon(reply);reply=NULL;m=NULL;goto done;}result=(int)m->result;
done:
 if(m)FreeVec(m);if(reply)nd_reply_delete(reply);
 if(!strcmp(verb,"HANDLELIST") && !strncmp(path,"T:NovaDockShelfDrop-",sizeof("T:NovaDockShelfDrop-")-1) && strlen(path)<sizeof(directory)){
  strcpy(directory,path);end=strrchr(directory,'/');if(end && !strcmp(end,"/items")){*end=0;lock=Lock((STRPTR)path,ACCESS_READ);if(lock){UnLock(lock);DeleteFile((STRPTR)path);}DeleteFile(directory);}
 }
 return result;
}
int main(int argc,char **argv)
{
 struct NDClient client;struct NDList *list=NULL;struct NDPanel *add=NULL;struct MsgPort *drops;struct ShelfDrop *m;char port[64],value[32],badge[8],rows[ND_SHELF_MAX][160];const char *pointers[ND_SHELF_MAX];unsigned int elapsed=0,event,i;int action,selected,ok;
 static const char *buttons[]={"Open","Remove shortcut","Up","Down","Add path...","Close"};
 if(argc<2 || !nd_shelf_id(argv[1]))return 20;((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
 if(argc==4)return helper(argv[1],argv[2],argv[3]);
 if(!nd_client_init(&client,argv[1]))return 20;WorkbenchBase=OpenLibrary("workbench.library",36);drops=CreateMsgPort();if(!drops){nd_client_close(&client);return 20;}
 sprintf(port,"NovaDock.Shelf.%s",argv[1]);drops->mp_Node.ln_Name=port;Forbid();if(FindPort(port)){Permit();DeleteMsgPort(drops);nd_client_close(&client);return 20;}AddPort(drops);Permit();writable=load(argv[1]);
 for(;;){
  while((m=(struct ShelfDrop *)GetMsg(drops))!=NULL){m->result=10;
   if(m->message.mn_Length==sizeof(*m) && m->magic==SHELF_MAGIC && m->count && m->count<=32){next_shelf=shelf;ok=1;for(i=0;i<m->count;++i){m->paths[i][ND_SHELF_PATH-1]=0;if(!nd_shelf_add(&next_shelf,m->paths[i])){ok=0;break;}}if(ok && save())m->result=0;else if(ok)strcpy(status,"Cannot save shelf");else strcpy(status,"Shelf full or invalid shortcut");}
   nd_reply_finish((struct Message *)m);
  }
  sprintf(value,"%u shortcuts",shelf.count);sprintf(badge,"%u",shelf.count);event=nd_client_set_badge(&client,badge);event|=writable ? nd_client_update_quiet(&client,label,value,ND_NO_METER,elapsed,250):nd_client_update_art_quiet(&client,label,status,"shelf",1,elapsed,250);
  if(event&ND_EVENT_QUIT)break;if(event&ND_EVENT_SETTINGS){writable=load(argv[1]);nd_list_close(list);list=NULL;}
  if(event&ND_EVENT_CLICK){if(list)nd_list_raise(list);else list=nd_list_open(label,buttons,6);}
  for(i=0;i<shelf.count;++i){sprintf(rows[i],"%.159s",shelf.paths[i]);pointers[i]=rows[i];}nd_list_rows(list,pointers,shelf.count);
  nd_list_title(list,status);action=nd_list_event(list);selected=nd_list_selected(list);
  if(action==-1 || action==105){nd_list_close(list);list=NULL;}
  else if(action==100 && selected>=0)open_target(shelf.paths[selected]);
  else if(action>=101 && action<=103 && selected>=0){next_shelf=shelf;ok=action==101 ? nd_shelf_remove(&next_shelf,(unsigned int)selected):nd_shelf_move(&next_shelf,(unsigned int)selected,action==102 ? -1:1);if(ok && save()){int next=action==102 ? selected-1:action==103 ? selected+1:selected;if(next>=(int)shelf.count)next=(int)shelf.count-1;nd_list_select(list,next);}}
  else if(action==104 && !add){struct NDPanelField f={"File or drawer","",239};static const char *b[]={"Add shortcut","Cancel"};add=nd_panel_open("Add Shelf shortcut",&f,1,b,2);}
  action=nd_panel_event(add);if(action==-1 || action==101){nd_panel_close(add);add=NULL;}
  else if(action==100){next_shelf=shelf;if(nd_shelf_add(&next_shelf,nd_panel_get(add,0)) && save()){nd_panel_close(add);add=NULL;}else nd_panel_title(add,"Could not add or save shortcut");}
  elapsed=nd_client_wait_signals(&client,list || add ? 10:250,nd_list_signals(list)|nd_panel_signals(add)|(1UL<<drops->mp_SigBit));
 }
 Forbid();RemPort(drops);Permit();while((m=(struct ShelfDrop *)GetMsg(drops))!=NULL){m->result=10;nd_reply_finish((struct Message *)m);}DeleteMsgPort(drops);nd_list_close(list);nd_panel_close(add);if(WorkbenchBase)CloseLibrary(WorkbenchBase);nd_client_close(&client);return 0;
}
