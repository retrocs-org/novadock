/* Real live-drag input, exact saved offsets and cold-start restoration. */
#define main legacy_grip_main
#include "native_dock_grip.c"
#undef main
static int control_present(void){int yes;Forbid();yes=FindPort(ND_CONTROL_PORT)!=NULL;Permit();return yes;}
static int launch_test_dock(void){BPTR in=Open("NIL:",MODE_OLDFILE),out=Open("NIL:",MODE_NEWFILE);LONG rc;if(!in||!out)return 0;rc=SystemTags("QATests:Dock/NovaDock",SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE);return rc>=0;}
static int bounds(struct Screen *s,const struct NDDiagnosticsSnapshot *d)
{return d->viewport_left>=0&&d->viewport_top>=s->BarHeight&&d->viewport_left+d->viewport_width<=s->Width&&d->viewport_top+d->viewport_height<=s->Height;}
static int origin(struct Screen *s,const struct NDDiagnosticsSnapshot *d,int side)
{return side<2?d->grip_left:d->grip_top-s->BarHeight;}
int main(int argc,char **argv)
{
 struct Screen *s=NULL;struct NDDiagnosticsSnapshot d;static struct NDConfig saved;BPTR guard;
 int side,mag,target,axis,length,wide,travel,want,x,y,i,ok=0,starts=0,cold;
 guard=Lock("QATests:ISOLATED-QA-ONLY",ACCESS_READ);if(!guard)return 20;UnLock(guard);setvbuf(stdout,NULL,_IONBF,0);
 IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
 if(!IntuitionBase||!GfxBase)goto done;s=LockPubScreen("Workbench");if(!s||control_present())goto done;
 qa_top=argc>1&&!strcmp(argv[1],"BACKDROP")?0:1;
 qa_artwork=argc>2&&!strcmp(argv[2],"ARTWORK");
 SystemTags("MakeDir RAM:GripEnv RAM:GripEnv/NovaDock",TAG_DONE);SystemTags("Assign ENV: RAM:GripEnv",TAG_DONE);SystemTags("Assign ENVARC: RAM:GripEnv",TAG_DONE);SystemTags("Assign NovaDock: QATests:Dock",TAG_DONE);
 if(!config(0,1,0)||!launch_test_dock())goto done;
 for(i=0;i<400&&!control_present();++i)Delay(1);Delay(40);
 for(mag=0;mag<=100;mag+=100)for(side=0;side<4;++side){
  pointer(s,s->Width/2,s->Height/2);
  if(!config(side,1,mag))goto stop;
  {LONG rc=nd_control_reload();printf("Reload side=%d mag=%d rc=%ld\n",side,mag,rc);if(rc!=ND_CONTROL_OK)goto stop;}Delay(40);
  wide=shelf_length(s,side);length=side<2?s->Width:s->Height-s->BarHeight;travel=length-wide;
  if(travel<2){puts("FAIL no drag travel in fixture");goto stop;}
  for(target=0;target<5;++target){
   want=target==0?0:target==1?travel/3:target==2?travel/3+1:target==3?travel:travel/2;
   if(!grip_point(&x,&y)){printf("FAIL no grip side=%d mag=%d target=%d\n",side,mag,target);goto stop;}pointer(s,x,y);Delay(8);button(IECODE_LBUTTON);Delay(3);
   axis=side<2?want+8:s->BarHeight+want+8;
   if(target==0)axis=0;if(target==3)axis=side<2?s->Width-1:s->Height-1;
   if(side<2)x=axis;else y=axis;
   pointer(s,x,y);
   /* The dock must follow before release, and every physical window must fit. */
   for(i=0;i<100;++i){Delay(2);if(nd_control_diagnostics(&d)==ND_CONTROL_OK&&bounds(s,&d)&&origin(s,&d,side)==want)break;}
   if(i==100){printf("FAIL live side=%d mag=%d target=%d want=%d actual=%d viewport=%d,%d %dx%d\n",side,mag,target,want,origin(s,&d,side),d.viewport_left,d.viewport_top,d.viewport_width,d.viewport_height);button(IECODE_LBUTTON|IECODE_UP_PREFIX);goto stop;}
   button(IECODE_LBUTTON|IECODE_UP_PREFIX);Delay(20);
   if(!nd_control_sync_settings()||nd_load_effective(&saved)>0||saved.position!=ND_POSITION_CUSTOM||saved.position_offset!=want){printf("FAIL save side=%d mag=%d want=%d mode=%d offset=%d\n",side,mag,want,saved.position,saved.position_offset);goto stop;}
   pointer(s,s->Width/2,s->Height/2);Delay(30);
   if(nd_control_diagnostics(&d)!=ND_CONTROL_OK||!bounds(s,&d)||origin(s,&d,side)!=want){puts("FAIL released placement");goto stop;}
   ++starts;
  }
  printf("PASS side=%d magnify=%d live pixel movement, one-pixel adjustment, both screen clamps and saved offsets\n",side,mag);
 }
 cold=saved.position_offset;
 if(nd_control_send(ND_CONTROL_QUIT)!=ND_CONTROL_OK)goto stop;
 for(i=0;i<400&&control_present();++i)Delay(1);
 if(control_present()||!launch_test_dock())goto stop;
 for(i=0;i<400;++i){Delay(2);if(nd_control_diagnostics(&d)==ND_CONTROL_OK&&bounds(s,&d)&&origin(s,&d,side-1)==cold)break;}
 if(i==400){puts("FAIL cold-start position");goto stop;}
 if(!grip_point(&x,&y))goto stop;pointer(s,x,y);Delay(8);button(IECODE_RBUTTON);Delay(8);button(IECODE_RBUTTON|IECODE_UP_PREFIX);Delay(8);
 if(!main_menu_open(s)){puts("FAIL grip main menu");goto stop;}
 printf("PASS continuous grip: %d live drags, all four edges, magnification off/on, bounded windows, persistent cold restart and main menu (top=%d)\n",starts,qa_top);ok=1;
 stop:if(!ok){printf("FAIL stage side=%d mag=%d target=%d cfg=%d\n",side,mag,target,nd_load_effective(&saved));SystemTags("Copy ENV:NovaDock/novadock.cfg QATests:failed.cfg",TAG_DONE);capture_failure(s);}nd_control_send(ND_CONTROL_QUIT);Delay(20);
 done:if(s)UnlockPubScreen(NULL,s);if(GfxBase)CloseLibrary((struct Library *)GfxBase);if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);return ok?0:10;
}
