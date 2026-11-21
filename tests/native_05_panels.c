#include <exec/types.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <stdio.h>
#include <string.h>
#include "nd_panel.h"
struct IntuitionBase *IntuitionBase=NULL;struct Library *GadToolsBase=NULL;long __stack=65536;
int main(void)
{struct NDPanel *p;struct Screen *s;struct Window *w;struct Gadget *g;int i,result=10;char text[513];const char *b[]={"Pick (3s)","Copy hex","Save colour","Palette"};struct NDPanelField f[]={{"Hex colour","#123456",8},{"RGB","18, 52, 86",0},{"Palette name","Unsaved name",32},{"Status","Ready",0},{"Preview","",ND_PANEL_SWATCH}};
 IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GadToolsBase=OpenLibrary("gadtools.library",39);if(!IntuitionBase||!GadToolsBase)return 10;s=LockPubScreen("Workbench");if(!s)return 10;p=nd_panel_open("Native in-place panel QA",f,5,b,4);if(!p)goto done;w=s->FirstWindow;
 for(g=w->FirstGadget;g&&g->GadgetID!=1;g=g->NextGadget){}if(!g)goto close;GT_SetGadgetAttrs(g,w,NULL,GTST_String,(ULONG)"#654321",TAG_DONE);
 for(i=0;i<100;++i){nd_panel_set(p,1,"Updated RGB");nd_panel_set(p,3,"No window reconstruction");nd_panel_swatch(p,4,i&1 ? 0xff0000UL:0x123456UL);nd_panel_sync_string(p,0,"#abcdef");nd_panel_button(p,1,"Copy RGB",i&1);if(s->FirstWindow!=w||strcmp(nd_panel_get(p,0),"#654321")||strcmp(nd_panel_get(p,2),"Unsaved name"))goto close;}
 puts("PASS: 100 in-place updates preserve window, user-edited hex and unsaved name; swatch pens are replaced safely.");nd_panel_close(p);p=NULL;
 f[0].label="Text / URL";f[0].limit=513;f[0].value="https://novadock.retrocs.org/";p=nd_panel_open("QR long-string QA",f,1,b,4);if(!p)goto done;memset(text,'x',512);text[512]=0;if(!nd_panel_replace_string(p,0,text)||strlen(nd_panel_get(p,0))!=512||nd_panel_replace_string(p,0,"" )!=1)goto close;puts("PASS: explicit paste replaces a 512-byte input in place.");result=0;
 close:nd_panel_close(p);
 done:UnlockPubScreen(NULL,s);CloseLibrary(GadToolsBase);CloseLibrary((struct Library *)IntuitionBase);return result;}
