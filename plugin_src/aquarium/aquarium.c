#include <exec/types.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <intuition/intuition.h>
#include <devices/inputevent.h>
#include <graphics/gfx.h>
#include <graphics/rastport.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_plugin_versions.h"
#include "nd_client.h"
#include "nd_option_runtime.h"
#include "nd_aquarium_art.h"
struct IntuitionBase *IntuitionBase=NULL;struct GfxBase *GfxBase=NULL;
#include "nd_aquarium_window.h"
const char nd_version[]=ND_PLUGIN_VER_STRING(AQUARIUM,"Aquarium");
long __stack=16384;
int main(int argc,char **argv)
{
 struct NDClient client;struct NDAquariumWindow scene;struct NDOptionString options[3];
 char fish_text[8],bubbles_text[8],speed[8],value[32];
 unsigned int event,elapsed=0,interval=25,delay;unsigned long frame=0,ticks=0,steps;
 int reread=1,fish=5,bubbles=1,active;
 if(argc<2||!nd_client_init(&client,argv[1]))return 20;memset(&scene,0,sizeof(scene));
 IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
 for(;;){
  if(reread){options[0].key="FISH";options[0].fallback="5";options[0].out=fish_text;options[0].limit=sizeof(fish_text);
   options[1].key="BUBBLES";options[1].fallback="YES";options[1].out=bubbles_text;options[1].limit=sizeof(bubbles_text);
   options[2].key="SPEED";options[2].fallback="Normal";options[2].out=speed;options[2].limit=sizeof(speed);
   nd_plugin_option_strings(argv[1],options,3);fish=atoi(fish_text);if(fish<1)fish=1;if(fish>8)fish=8;
   bubbles=strcmp(bubbles_text,"NO")!=0;interval=!strcmp(speed,"Slow") ? 50:!strcmp(speed,"Fast") ? 15:25;reread=0;
   if(scene.window)aquarium_paint(&scene,fish,bubbles);
  }
  sprintf(value,"FISH %lu",frame);event=nd_client_update_art_quiet(&client,"AQUARIUM",value,"fish",(unsigned int)(frame%70UL),elapsed,500U);
  if(event&ND_EVENT_QUIT)break;if(event&ND_EVENT_SETTINGS)reread=1;
  if(event&ND_EVENT_CLICK)(void)aquarium_open(&scene,fish,bubbles);aquarium_events(&scene);
  active=scene.window&&(scene.window->Flags&WFLG_WINDOWACTIVE);
  delay=!client.throttled||active ? interval:500;
  elapsed=nd_client_wait_signals(&client,delay,scene.window ? 1UL<<scene.window->UserPort->mp_SigBit:0);
  ticks+=elapsed;steps=ticks/interval;ticks%=interval;
  if(!client.throttled)frame=(frame+steps)%65520UL;
  if(active){scene.frame=(scene.frame+steps)%65520UL;if(steps)aquarium_paint(&scene,fish,bubbles);}
 }
 aquarium_close(&scene);if(GfxBase)CloseLibrary((struct Library *)GfxBase);if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);nd_client_close(&client);return 0;
}
