/* Bounded asynchronous local requests. Late replies own their abandoned port. */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#include <stdio.h>
#include "nd_hass_native.h"
#include "service/amihass_config.h"
#include "nd_reply.h"
#include "nd_interaction.h"
static unsigned long ticks(void)
{struct DateStamp d;DateStamp(&d);return ((unsigned long)d.ds_Days*1440UL+(unsigned long)d.ds_Minute)*3000UL+(unsigned long)d.ds_Tick;}
void amihass_erase(void *p,unsigned long n)
{volatile unsigned char *v=p;while(n--)*v++=0;}
static int send_request(struct AmiHASSClient *c,int kind,const char *owner,const char *entity,const char *action,int offset,int refresh,const char *domain,const char *filter,const char *area)
{
 struct AmiHASSMessage *m;struct MsgPort *server;int sent=0;
 if(!c || c->pending || (area && (strlen(area)>95 || (kind!=AMIHASS_LIST && kind!=AMIHASS_DEVICES))))return 0;
 if(!c->port)c->port=nd_reply_port();if(!c->port)return 0;
 m=AllocVec(sizeof(*m),MEMF_PUBLIC|MEMF_CLEAR);if(!m)return 0;
 m->magic=AMIHASS_MAGIC;m->abi=AMIHASS_ABI;m->kind=kind;m->offset=offset;m->refresh=refresh;
 if(owner){strncpy(m->owner,owner,sizeof(m->owner)-1);}if(entity){strncpy(m->entity,entity,sizeof(m->entity)-1);}if(action){strncpy(m->action,action,sizeof(m->action)-1);}
 if(domain)strncpy(m->domain,domain,sizeof(m->domain)-1);if(filter)strncpy(m->filter,filter,sizeof(m->filter)-1);
 if(area)strcpy(m->item.id,area);
 m->msg.mn_Length=sizeof(*m);m->msg.mn_ReplyPort=c->port;
 Forbid();server=(kind!=AMIHASS_STOP && kind!=AMIHASS_STATUS && kind!=AMIHASS_DIAGNOSTICS && FindPort("AmiHASS.Maintenance")) ? NULL:FindPort(AMIHASS_PORT);if(server){PutMsg(server,(struct Message *)m);sent=1;}Permit();
 if(!sent){FreeVec(m);return 0;}c->pending=m;c->started=ticks();return 1;
}
int amihass_send(struct AmiHASSClient *c,int kind,const char *owner,const char *entity,const char *action,int offset,int refresh,const char *domain,const char *filter)
{return send_request(c,kind,owner,entity,action,offset,refresh,domain,filter,NULL);}
int amihass_send_area(struct AmiHASSClient *c,int kind,const char *owner,const char *device,int offset,int refresh,const char *domain,const char *filter,const char *area)
{return send_request(c,kind,owner,device,NULL,offset,refresh,domain,filter,area);}
int amihass_subscribe(struct AmiHASSClient *c,const char *owner,const char *entity,unsigned int seconds)
{if(seconds && (seconds<5 || seconds>3600))return 0;return amihass_send(c,AMIHASS_SUBSCRIBE,owner,entity,NULL,0,AMIHASS_QUERY_INTERVAL(seconds),NULL,NULL);}
int amihass_unsubscribe(struct AmiHASSClient *c,const char *entity)
{return amihass_send(c,AMIHASS_UNSUBSCRIBE,NULL,entity,NULL,0,0,NULL,NULL);}
/* No reply is requested: the old port is an identity token, never dereferenced.
 * Enqueue before freeing the port so a later client cannot overtake cleanup. */
static void release_selections(struct MsgPort *identity)
{struct AmiHASSMessage *m;struct MsgPort *server;m=AllocVec(sizeof(*m),MEMF_PUBLIC|MEMF_CLEAR);if(!m)return;m->magic=AMIHASS_MAGIC;m->abi=AMIHASS_ABI;m->kind=AMIHASS_UNSUBSCRIBE;m->age=(ULONG)(unsigned long)identity;m->msg.mn_Length=sizeof(*m);Forbid();server=FindPort(AMIHASS_PORT);if(server)PutMsg(server,(struct Message *)m);Permit();if(!server)FreeVec(m);}
struct AmiHASSMessage *amihass_reply(struct AmiHASSClient *c)
{struct AmiHASSMessage *m;if(!c || !c->port)return NULL;m=(struct AmiHASSMessage *)GetMsg(c->port);if(m){c->pending=NULL;if(m->magic!=AMIHASS_MAGIC || m->abi!=AMIHASS_ABI || m->msg.mn_Length!=sizeof(*m) || m->count<0 || m->count>AMIHASS_PAGE || m->total<0){FreeVec(m);return NULL;}return m;}if(c->pending && ticks()-c->started>500UL)amihass_client_close(c);return NULL;}
void amihass_client_close(struct AmiHASSClient *c)
{
 struct AmiHASSMessage *m;if(!c || !c->port)return;
 release_selections(c->port);
 Forbid();while((m=(struct AmiHASSMessage *)GetMsg(c->port))!=NULL){c->pending=NULL;FreeVec(m);}
 if(c->pending)nd_reply_abandon(c->port);else nd_reply_delete(c->port);
 Permit();memset(c,0,sizeof(*c));
}
int amihass_launch(const char *name,const char *argument)
{
 static const char *roots[]={"AmiHASS:","SYS:Utilities/AmiHASS","PROGDIR:","C:",NULL};
 char path[520],record[300],quoted[1100],command[1200];BPTR lock,in,out;FILE *f;int i,found=0;LONG rc;struct Process *self=(struct Process *)FindTask(NULL);APTR saved=self->pr_WindowPtr;
 if(strcmp(name,"AmiHASSPrefs") && strcmp(name,"AmiHASSService") && strcmp(name,"AmiHASSBrowser"))return 0;
 if(!strcmp(name,"AmiHASSBrowser") && argument)return 0;
 if(argument && strcmp(argument,"CONFIG") && strcmp(argument,"SERVICE"))return 0;
 self->pr_WindowPtr=(APTR)-1L;
 record[0]=0;
 if(!strcmp(name,"AmiHASSBrowser"))strcpy(record,"SYS:Utilities");
 else {f=fopen(AMIHASS_CONFIG_FILE,"rb");
 if(f){char line[1100];int row,target=!strcmp(name,"AmiHASSPrefs") ? 6:5;
  if(fgets(line,sizeof(line),f) && !strcmp(line,"AMIHASSCFG1\n"))for(row=2;row<=target;++row){if(!fgets(line,sizeof(line),f))break;if(row==target){line[strcspn(line,"\r\n")]=0;if(strlen(line)<sizeof(record))strcpy(record,line);}}
  amihass_erase(line,sizeof(line));fclose(f);
 }
 if(!record[0]){f=fopen("ENVARC:AmiHASS/InstallPath","r");if(f){if(!fgets(record,sizeof(record),f))record[0]=0;fclose(f);record[strcspn(record,"\r\n")]=0;}}
 }
 for(i=-1;i<4 && !found;++i){const char *root=i<0 ? record:roots[i];struct FileInfoBlock *fib;
  if(!root[0] || strlen(root)>300)continue;strcpy(path,root);if(!AddPart(path,(STRPTR)name,sizeof(path)))continue;
  lock=Lock(path,ACCESS_READ);if(!lock)continue;fib=AllocDosObject(DOS_FIB,NULL);
  if(fib && Examine(lock,fib) && fib->fib_DirEntryType<0)found=NameFromLock(lock,path,sizeof(path))!=0;
  if(fib)FreeDosObject(DOS_FIB,fib);UnLock(lock);
 }
 self->pr_WindowPtr=saved;if(!found || !nd_quote_path(quoted,sizeof(quoted),path))return 0;
 strcpy(command,quoted);if(argument){strcat(command," ");strcat(command,argument);}
 in=Open("NIL:",MODE_OLDFILE);out=Open("NIL:",MODE_NEWFILE);
 if(!in || !out){if(in)Close(in);if(out)Close(out);return 0;}
 rc=SystemTags(command,SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE);
 if(rc<0){Close(in);Close(out);return 0;}return 1;
}
int amihass_start(const char *unused)
{int present;(void)unused;Forbid();if(FindPort("AmiHASS.Maintenance")){Permit();return 0;}present=FindPort(AMIHASS_PORT)!=NULL;Permit();return present || amihass_launch("AmiHASSService","SERVICE");}
const char *amihass_problem(int r)
{switch(r){case 9:return "Install/start AmiHASS service";case AMIHASS_OK:return "Connected";case AMIHASS_UNAVAILABLE:return "Entity unavailable; no action sent";case AMIHASS_WAIT:return "Updating...";case AMIHASS_UNCONFIGURED:return "Set up Home Assistant connection";case AMIHASS_STALE:return "Stale; last reading is not current";case AMIHASS_BUSY:return "Busy; try again shortly";case AMIHASS_UNSUPPORTED:return "Action is not supported for this entity";default:return "Connection/authentication failed; check server, token and curl";}}
