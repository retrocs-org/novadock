/* Isolated live add/remove regression: preserve native launcher icons while
 * MUSIC switches between wide live controls and compact artwork. */
#define main grip_reference_main
#include "native_grip_ghost.c"
#undef main
#include "nd_client.h"
static int fake_music(void)
{
 struct NDClient c;unsigned int event=0,volume;FILE *f;int active;
 if(!nd_client_init(&c,"MUSIC"))return 20;
 while(!(event&ND_EVENT_QUIT)){
  f=fopen("QATests:music-mode","r");active=f?fgetc(f)=='1':0;if(f)fclose(f);
  event=nd_client_update_media(&c,"QA player",active?"PLAYING":"IDLE",active?7:0,active?1:0,50,&volume);
  nd_client_wait(&c,5);
 }nd_client_close(&c);return 0;
}
static int reload_config(unsigned int count)
{
 FILE *f=fopen("ENV:NovaDock/novadock.cfg","w");if(!f)return 0;
 fputs("DRAG_BAR=YES\nALWAYS_ON_TOP=NO\nAUTO_HIDE=NO\nREMOVABLE_MEDIA=NO\nTHEME=Nord\nDOCK_BACKGROUND=ARTWORK\nICON_COLOURS=COLOURFUL\nHOVER_MAGNIFICATION=100\nHOVER_ANIMATION=YES\nTILE_WIDTH=86\nTILE_HEIGHT=76\nTOOLTIPS=NO\nDRAWER=System|SYS:|SYS:Disk|ON|AUTO||||NO|YES\nLAUNCH=Shell|NewShell|SYS:System/Shell|ON|AUTO\nPLUGIN=Music|Plugins/FakeMusic|MUSIC|METER|ON\nPLUGIN=CPU|Plugins/Activity|CPU|METER|ON\nPLUGIN=RAM|Plugins/Memory|RAM|METER|ON\nPLUGIN=Clock|Plugins/Clock|CLOCK|METER|ON\n",f);
 if(count>0)fputs("PLUGIN=Archive|Plugins/ArchiveExtractor|ARCHIVE|METER|ON\n",f);
 if(count>1)fputs("PLUGIN=Saver|Plugins/Saver|SAVER|METER|ON\n",f);
 if(count>2)fputs("PLUGIN=Pet|Plugins/Pet|PET|METER|ON\n",f);
 return fclose(f)==0;
}
static int set_music(int active)
{FILE *f;int tries;for(tries=0;tries<50;++tries){f=fopen("QATests:music-mode","w");if(f){fputc(active?'1':'0',f);return fclose(f)==0;}Delay(1);}printf("Mode-file write failed DOS=%ld\n",IoErr());return 0;}
static int system_pixels(struct Screen *s,UBYTE *out)
{
 struct NDDiagnosticsSnapshot d;int y;
 if(nd_control_diagnostics(&d)!=ND_CONTROL_OK||d.viewport_pending)return 0;
 for(y=0;y<48;++y)if(nd_glass_rgb_span(out+y*144,48,&s->RastPort,d.grip_left+42,d.grip_top+5+y)!=48)return 0;
 return 1;
}
int main(int argc,char **argv)
{
 struct Screen *screen;BPTR lock,in,out;UBYTE reference[48*48*3],current[48*48*3];struct NDDiagnosticsSnapshot d;int i,j,rc=10;
 (void)argc;if(strstr(argv[0],"FakeMusic"))return fake_music();
 lock=Lock("QATests:ISOLATED-QA-ONLY",ACCESS_READ);if(!lock)return 20;UnLock(lock);setvbuf(stdout,NULL,_IONBF,0);
 IntuitionBase=(struct IntuitionBase*)OpenLibrary("intuition.library",39);GfxBase=(struct GfxBase*)OpenLibrary("graphics.library",39);NDGlassCyberGfxBase=OpenLibrary("cybergraphics.library",41);
 if(!IntuitionBase||!GfxBase||!NDGlassCyberGfxBase)return 20;screen=LockPubScreen("Workbench");if(!screen)return 20;
 SystemTags("MakeDir RAM:ReloadEnv RAM:ReloadEnv/NovaDock",TAG_DONE);SystemTags("Assign ENV: RAM:ReloadEnv",TAG_DONE);SystemTags("Assign NovaDock: QATests:Dock",TAG_DONE);
 if(!set_music(0)||!reload_config(0)){printf("FAIL reload setup line %d\n",__LINE__);goto done;}pointer(screen,10,30);
 in=Open("NIL:",MODE_OLDFILE);out=Open("NIL:",MODE_NEWFILE);
 if(SystemTags("QATests:Dock/NovaDock",SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,32768UL,TAG_DONE)<0){printf("FAIL reload setup line %d\n",__LINE__);goto done;}
 for(i=0;i<400;++i){if(dock_window(screen))break;Delay(1);}if(i==400){printf("FAIL reload check line %d\n",__LINE__);goto stop;}Delay(180);
 if(!system_pixels(screen,reference)){printf("FAIL reload check line %d\n",__LINE__);goto stop;}
 for(i=0;i<16;++i){
  pointer(screen,10,30);if(!reload_config(i%4)||nd_control_reload()!=ND_CONTROL_OK){printf("FAIL reload check line %d\n",__LINE__);goto stop;}Delay(80);
  if(!system_pixels(screen,current)||memcmp(reference,current,sizeof(reference))){puts("FAIL System icon changed during plugin add/remove");capture_failure(screen);{printf("FAIL reload check line %d\n",__LINE__);goto stop;}}
  if(!set_music(1)){printf("FAIL reload check line %d\n",__LINE__);goto stop;}Delay(20);{LONG status=nd_control_diagnostics(&d);if(status!=ND_CONTROL_OK){printf("FAIL diagnostic status %ld\n",status);capture_failure(screen);goto stop;}}
  pointer(screen,d.grip_left+293,screen->Height-80);Delay(50);
  if(!set_music(0)){printf("FAIL reload check line %d\n",__LINE__);goto stop;}for(j=0;j<20;++j){Delay(2);if(!AvailMem(MEMF_LARGEST)){printf("FAIL reload check line %d\n",__LINE__);goto stop;}}
  pointer(screen,10,30);Delay(60);
  if(!system_pixels(screen,current)||memcmp(reference,current,sizeof(reference))){puts("FAIL System icon corrupted after widget/art transition");capture_failure(screen);{printf("FAIL reload check line %d\n",__LINE__);goto stop;}}
  printf("PASS live plugin set=%d cycle=%d largest=%lu\n",i%4,i,AvailMem(MEMF_LARGEST));
 }
 puts("PASS 16 live plugin additions/removals and wide-widget/artwork transitions preserve System icon pixels and the Exec free list");rc=0;
 stop:nd_control_send(ND_CONTROL_QUIT);Delay(15);
 done:UnlockPubScreen(NULL,screen);CloseLibrary(NDGlassCyberGfxBase);CloseLibrary((struct Library*)GfxBase);CloseLibrary((struct Library*)IntuitionBase);return rc;
}
