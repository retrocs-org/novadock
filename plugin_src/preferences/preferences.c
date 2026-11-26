/* Theme-aware Preferences tile. Opening the editor is a host action; this
 * lightweight process owns artwork/status and never changes user settings. */
#include <string.h>
#include <intuition/intuitionbase.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include "nd_client.h"
#include "nd_plugin_versions.h"
const char nd_version[]=ND_PLUGIN_VER_STRING(PREFERENCES,"Preferences");
long __stack=16384;
struct IntuitionBase *IntuitionBase=NULL;
static int editor_open(void)
{struct Screen*s;struct Window*w;ULONG lock;int found=0;if(!IntuitionBase)return 0;
 lock=LockIBase(0);for(s=IntuitionBase->FirstScreen;s&&!found;s=s->NextScreen)
 for(w=s->FirstWindow;w;w=w->NextWindow)if(w->Title && (!strncmp((char*)w->Title,"NovaDock Preferences",20)||!strncmp((char*)w->Title,"NovaDock Appearance",19))){found=1;break;}
 UnlockIBase(lock);return found;}
int main(int argc,char **argv)
{struct NDClient c;unsigned int event,elapsed=0;int was=-1;
 if(argc<2 || !nd_client_init(&c,argv[1]))return 20;
 IntuitionBase=(struct IntuitionBase*)OpenLibrary("intuition.library",39);
 for(;;){int active=editor_open();event=nd_client_update_art_quiet(&c,"Preferences","Open editor","preferences",0,elapsed,500U);
  if(active!=was){event|=nd_client_set_badge(&c,active ? "*":"");was=active;}
  if(event&ND_EVENT_QUIT)break;
  elapsed=nd_client_wait(&c,50U);
 }
 if(IntuitionBase)CloseLibrary((struct Library*)IntuitionBase);nd_client_close(&c);return 0;}
