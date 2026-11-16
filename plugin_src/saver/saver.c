/* Manual, full-screen screen saver. No input hooks and no password lock. */
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
#include "nd_saver_art.h"
#include "nd_saver_modes.h"
struct IntuitionBase *IntuitionBase=NULL;struct GfxBase *GfxBase=NULL;
#include "nd_saver_window.h"
const char nd_version[]=ND_PLUGIN_VER_STRING(SAVER,"Saver");long __stack=16384;
int main(int argc,char **argv)
{
 struct NDClient client;struct NDSaverWindow scene;struct NDOptionString options[11];char mode_text[32],speed[8],hold[12],selected[8][8],value[32];
 unsigned int event,elapsed=0,interval=10,delay;unsigned long steps;int reread=1,mode=0,active,current=0,previous=-1;unsigned int mask=0,hold_seconds=40;unsigned long shuffle_ticks=0;
 if(argc<2||!nd_client_init(&client,argv[1]))return 20;memset(&scene,0,sizeof(scene));
 IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
 for(;;){
  if(reread){int at=3;
   options[0].key="SCENE";options[0].fallback="Starfield";options[0].out=mode_text;options[0].limit=sizeof(mode_text);
   options[1].key="SPEED";options[1].fallback="Normal";options[1].out=speed;options[1].limit=sizeof(speed);
   options[2].key="SHUFFLE_SECONDS";options[2].fallback="40";options[2].out=hold;options[2].limit=sizeof(hold);
#define SAVER_READ(optkey,label,number,fallback_value) options[at].key="SHUFFLE_" #optkey;options[at].fallback=fallback_value;options[at].out=selected[number];options[at].limit=8;++at;
   ND_SAVER_SHUFFLE_OPTIONS(SAVER_READ)
#undef SAVER_READ
   nd_plugin_option_strings(argv[1],options,11);mode=nd_saver_mode(mode_text);mask=0;
   for(at=0;at<8;++at)if(strcmp(selected[at],"NO"))mask|=1U<<at;
   hold_seconds=(unsigned int)atoi(hold);if(hold_seconds<10 || hold_seconds>300)hold_seconds=40;
   interval=!strcmp(speed,"Slow") ? 20:!strcmp(speed,"Fast") ? 5:10;reread=0;previous=-1;shuffle_ticks=0;
  }
  current=nd_saver_pick(mode,mask,shuffle_ticks,(unsigned long)hold_seconds*50UL);
  if(previous!=current){scene.frame=scene.ticks=0;previous=current;if(scene.window)saver_paint(&scene,current,0);}
  if(mode==ND_SAVER_SHUFFLE)sprintf(value,"Shuffle: %.22s",nd_saver_name(current));else strcpy(value,nd_saver_name(current));
  event=nd_client_update_quiet(&client,"SAVER",value,ND_NO_METER,elapsed,500U);
  if(event&ND_EVENT_QUIT)break;if(event&ND_EVENT_SETTINGS)reread=1;
  if(event&ND_EVENT_CLICK)(void)saver_open(&scene,current,0);saver_events(&scene);
  active=scene.window&&(scene.window->Flags&WFLG_WINDOWACTIVE);delay=active&&current!=2 ? interval:500;
  elapsed=nd_client_wait_signals(&client,delay,scene.window ? 1UL<<scene.window->UserPort->mp_SigBit:0);
  if(active)shuffle_ticks=(shuffle_ticks+elapsed)%120000000UL;
  if(active&&current!=2){scene.ticks+=elapsed;steps=scene.ticks/interval;scene.ticks%=interval;
   if(steps){scene.frame=(scene.frame+steps)%32760UL;saver_paint(&scene,current,0);}}
 }
 saver_close(&scene);if(GfxBase)CloseLibrary((struct Library *)GfxBase);if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);nd_client_close(&client);return 0;
}
