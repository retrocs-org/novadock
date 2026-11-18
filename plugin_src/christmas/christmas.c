/* Seasonal tree and a non-modal Christmas scene with bounded animation. */
#include <exec/types.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <intuition/intuition.h>
#include <devices/inputevent.h>
#include <graphics/gfx.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <string.h>
#include <stdio.h>
#include "nd_plugin_versions.h"
#include "nd_client.h"
#include "nd_option_runtime.h"
#include "nd_christmas_scene.h"
struct IntuitionBase *IntuitionBase=NULL;
struct GfxBase *GfxBase=NULL;
#include "nd_christmas_window.h"
#include "nd_christmas_audio.h"
#include <stdlib.h>
const char nd_version[]=ND_PLUGIN_VER_STRING(CHRISTMAS,"Christmas");
long __stack=16384;
int main(int argc,char **argv)
{
 struct NDClient client;struct NDOptionString options[6];char animation[8],lights[8],pattern[12],speed[8],sound[8],volume[8],value[32];
 struct NDXmasWindow scene;struct NDXmasAudio audio;int music=1,music_volume=30;unsigned long tile_ticks=0,steps;
 unsigned int frame=0,event,elapsed=0,delay=25,interval;int reread=1,animate=1,light=1,mode=0,rate=1,hidden=0,tile_active,scene_active;
 if(argc<2 || !nd_client_init(&client,argv[1]))return 20;
 memset(&scene,0,sizeof(scene));memset(&audio,0,sizeof(audio));
 IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
 GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
 for(;;){
  if(reread){
   options[0].key="ANIMATE";options[0].fallback="YES";options[0].out=animation;options[0].limit=sizeof(animation);
   options[1].key="LIGHTS";options[1].fallback="YES";options[1].out=lights;options[1].limit=sizeof(lights);
   options[2].key="PATTERN";options[2].fallback="Twinkle";options[2].out=pattern;options[2].limit=sizeof(pattern);
   options[3].key="SPEED";options[3].fallback="Normal";options[3].out=speed;options[3].limit=sizeof(speed);
   options[4].key="MUSIC";options[4].fallback="YES";options[4].out=sound;options[4].limit=sizeof(sound);
   options[5].key="VOLUME";options[5].fallback="30";options[5].out=volume;options[5].limit=sizeof(volume);
   nd_plugin_option_strings(argv[1],options,6);
   music=strcmp(sound,"NO")!=0;music_volume=atoi(volume);if(music_volume<0)music_volume=0;if(music_volume>100)music_volume=100;animate=strcmp(animation,"NO")!=0;light=strcmp(lights,"NO")!=0;
   mode=!strcmp(pattern,"Chase") ? 1:!strcmp(pattern,"Steady") ? 2:0;
   rate=!strcmp(speed,"Slow") ? 0:!strcmp(speed,"Fast") ? 2:1;reread=0;
   if(scene.window)xmas_paint(&scene,mode,light);
  }
  sprintf(value,"TREE %u %d %d",frame,mode,light);
  event=nd_client_update_art_quiet(&client,"XMAS",value,"tree",(unsigned int)(mode*16+light*8+(frame&7)),elapsed,500U);
  if(event&ND_EVENT_QUIT)break;if(event&ND_EVENT_SETTINGS)reread=1;
  if(event&ND_EVENT_CLICK)(void)xmas_open(&scene,mode,light);
  xmas_events(&scene);
  hidden=client.throttled!=0;
  interval=nd_christmas_delay(rate,0);
  tile_active=animate && !hidden && light && mode!=2;
  scene_active=scene.window && (scene.window->Flags&WFLG_WINDOWACTIVE);
  xmas_audio_update(&audio,scene_active && music,music_volume,elapsed);
  delay=tile_active || scene_active ? interval:500U;
   elapsed=nd_client_wait_signals(&client,delay,
   (scene.window ? 1UL<<scene.window->UserPort->mp_SigBit:0)|xmas_audio_signals(&audio));
  if(tile_active){steps=nd_xmas_advance(&tile_ticks,elapsed,interval);frame=(frame+(unsigned int)steps)&7U;}
  else tile_ticks=0;
  if(scene_active && scene.window){
   scene.story_ticks=(scene.story_ticks+elapsed)%3200UL;
   steps=nd_xmas_advance(&scene.ticks,elapsed,interval);
   if(steps){scene.frame=(scene.frame+steps)%3520UL;xmas_paint(&scene,mode,light);}
  }else scene.ticks=0;
 }
 xmas_audio_close(&audio);xmas_close(&scene);
 if(GfxBase)CloseLibrary((struct Library *)GfxBase);
 if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);
 nd_client_close(&client);return 0;
}
