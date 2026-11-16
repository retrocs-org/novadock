/* On-demand drawer shortcuts, personal bookmarks and mounted volumes. */
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/wb.h>
#include <stdio.h>
#include <string.h>
#include "nd_plugin_versions.h"
#include "nd_client.h"
#include "nd_option_runtime.h"
#include "nd_interaction.h"
#include "nd_places.h"
#include "../clipboard/nd_clipboard_popup.h"
struct IntuitionBase *IntuitionBase=NULL;struct GfxBase *GfxBase=NULL;
struct Library *WorkbenchBase=NULL;
const char nd_version[]=ND_PLUGIN_VER_STRING(PLACES,"Places");long __stack=16384;
static struct NDPlaces places;
struct Settings {char system[8],volumes[8],label[5][48],path[5][96];};
static void read_settings(const char *id,struct Settings *s)
{
 struct NDOptionString fields[12];char keys[10][16];int i;
 fields[0].key="SYSTEM";fields[0].fallback="YES";fields[0].out=s->system;fields[0].limit=8;
 fields[1].key="VOLUMES";fields[1].fallback="YES";fields[1].out=s->volumes;fields[1].limit=8;
 for(i=0;i<5;++i){sprintf(keys[i*2],"NAME%d",i+1);sprintf(keys[i*2+1],"BOOKMARK%d",i+1);
  fields[2+i*2].key=keys[i*2];fields[2+i*2].fallback="";fields[2+i*2].out=s->label[i];fields[2+i*2].limit=48;
  fields[3+i*2].key=keys[i*2+1];fields[3+i*2].fallback="";fields[3+i*2].out=s->path[i];fields[3+i*2].limit=96;}
 nd_plugin_option_strings(id,fields,12);
}
static int directory(const char *path)
{
 BPTR lock;struct FileInfoBlock *info;int ok=0;
 lock=Lock((STRPTR)path,ACCESS_READ);if(!lock)return 0;
 info=AllocDosObject(DOS_FIB,NULL);if(info){ok=Examine(lock,info)&&info->fib_DirEntryType>0;FreeDosObject(DOS_FIB,info);}UnLock(lock);return ok;
}
static void collect(const struct Settings *s)
{
 static const char *names[]={"System","RAM Disk","Preferences","Utilities","Tools","Trash"};
 static const char *paths[]={"SYS:","RAM:","SYS:Prefs","SYS:Utilities","SYS:Tools","SYS:Trashcan"};
 struct DosList *start,*entry;const unsigned char *b;char path[240];int i,n;
 memset(&places,0,sizeof(places));
 if(strcmp(s->system,"NO"))for(i=0;i<6;++i)if(directory(paths[i]))nd_places_add(&places,names[i],paths[i]);
 for(i=0;i<5;++i)nd_places_add(&places,s->label[i],s->path[i]);
 if(!strcmp(s->volumes,"NO"))return;
 /* Copy mounted names only while holding the DOS list. Never touch a disk
  * or open a requester under this lock, and never poll the list when idle. */
 start=LockDosList(LDF_VOLUMES|LDF_READ);if(!start)return;
 for(entry=start;(entry=NextDosEntry(entry,LDF_VOLUMES))!=NULL;){
  if(!entry->dol_Task)continue;b=(const unsigned char *)BADDR(entry->dol_Name);if(!b)continue;
  n=b[0];if(n<1||n>237)continue;memcpy(path,b+1,(size_t)n);path[n]=0;
  if(strchr(path,':')||strchr(path,'/'))continue;path[n]=':';path[n+1]=0;nd_places_add(&places,NULL,path);
 }
 UnLockDosList(LDF_VOLUMES|LDF_READ);
}
static int open_picker(struct NDClipQuick *picker,const struct Settings *s)
{
 struct NDClipHistory history;char text[ND_PLACES_MAX][300];int i;
 collect(s);memset(&history,0,sizeof(history));history.count=(unsigned int)places.count;
 for(i=0;i<places.count;++i){sprintf(text[i],"%s\n%s",places.items[i].label,places.items[i].path);history.text[i]=text[i];}
 return nd_clip_quick_open(picker,&history);
}
static int open_place(const char *path)
{
 char quoted[484],command[510];BPTR input,output;LONG result;
 if(!nd_places_path(path)||!directory(path))return 0;
 if(WorkbenchBase&&WorkbenchBase->lib_Version>=44&&OpenWorkbenchObject((STRPTR)path,TAG_DONE))return 1;
 if(!nd_quote_path(quoted,sizeof(quoted),path))return 0;
 input=Open("NIL:",MODE_OLDFILE);output=Open("NIL:",MODE_NEWFILE);
 if(!input||!output){if(input)Close(input);if(output)Close(output);return 0;}
 sprintf(command,"C:WBRUN %s",quoted);result=SystemTags(command,SYS_Input,input,SYS_Output,output,SYS_Asynch,TRUE,TAG_DONE);
 if(result==-1){Close(input);Close(output);return 0;}return 1;
}
int main(int argc,char **argv)
{
 struct NDClient client;struct NDClipQuick *picker=NULL;struct Settings settings;
 unsigned int event,elapsed=0;int reread=1,selected,result=20;char status[32]="Click for places";
 if(argc<2)return 20;((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
 IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);WorkbenchBase=OpenLibrary("workbench.library",36);
 if(!IntuitionBase||!GfxBase||!nd_client_init(&client,argv[1]))goto done;
 picker=nd_clip_quick_create();if(!picker){nd_client_close(&client);goto done;}
 nd_clip_quick_labels(picker,"PLACES","No places configured","NovaDock Places","Click or Return to open - Esc to close","Place");
 for(;;){
  if(reread){nd_clip_quick_close(picker);read_settings(argv[1],&settings);reread=0;}
  event=nd_client_update_quiet(&client,"PLACES",status,ND_NO_METER,elapsed,500U);
  if(event&ND_EVENT_QUIT){result=0;break;}if(event&ND_EVENT_SETTINGS){reread=1;continue;}
  if(event&ND_EVENT_CLICK){if(nd_clip_quick_active(picker))nd_clip_quick_close(picker);else if(!open_picker(picker,&settings))strcpy(status,"Picker unavailable");}
  if(nd_clip_quick_events(picker)){
   selected=nd_clip_quick_selected(picker);nd_clip_quick_close(picker);
   if(selected>=0&&selected<places.count)strcpy(status,open_place(places.items[selected].path) ? "Click for places":"Drawer unavailable; check path");
  }
  elapsed=nd_client_wait_signals(&client,500U,nd_clip_quick_signals(picker));
 }
 nd_clip_quick_free(picker);nd_client_close(&client);
done:
 if(WorkbenchBase)CloseLibrary(WorkbenchBase);if(GfxBase)CloseLibrary((struct Library *)GfxBase);if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);return result;
}
