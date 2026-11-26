/* All-side static-frame equality after delayed backdrop repaint and hover. */
#define main old_tiny_main
#include "native_tiny_metrics.c"
#undef main
static struct Window *side_dock(struct Screen *s)
{struct NDDiagnosticsSnapshot d;struct Window *w,*found=NULL;ULONG lock;if(nd_control_diagnostics(&d)!=ND_CONTROL_OK||d.viewport_pending)return NULL;lock=LockIBase(0);for(w=s->FirstWindow;w;w=w->NextWindow)if((w->Flags&WFLG_BORDERLESS)&&w->LeftEdge==d.viewport_left&&w->TopEdge==d.viewport_top&&w->Width==d.viewport_width&&w->Height==d.viewport_height){found=w;break;}UnlockIBase(lock);return found;}
/* Animated resize acknowledgements may land after the first valid viewport.
 * Require a settled rectangle before sampling; a transient pending viewport
 * is not a blank or off-screen dock. */
static struct Window *settled_side_dock(struct Screen *s)
{
 struct Window *w;int i,n=0,x=0,y=0,width=0,height=0;
 for(i=0;i<300;++i){
  w=side_dock(s);
  if(w){
   if(n&&w->LeftEdge==x&&w->TopEdge==y&&w->Width==width&&w->Height==height)++n;
   else{x=w->LeftEdge;y=w->TopEdge;width=w->Width;height=w->Height;n=1;}
   if(n>=10)return w;
  }else n=0;
  Delay(2);
 }
 return NULL;
}
static int capture(struct Screen *s,struct Window *w,UBYTE *data,int width,int height){int y;if(w->Width!=width||w->Height!=height)return 0;WaitBlit();for(y=0;y<height;++y)if(!line(s,data+(ULONG)y*width*3,w->LeftEdge,w->TopEdge+y,width))return 0;return 1;}
static unsigned int bg_count(void){FILE *f;unsigned int n=0;f=fopen("QATests:bg-count","r");if(f){(void)fscanf(f,"%u",&n);fclose(f);}return n;}
int main(int argc,char **argv){static struct NDConfig cfg;struct Screen *s=NULL;struct Window *w,*cover=NULL;struct MsgPort *bg;UBYTE *reference=NULL,*after=NULL;FILE *f;int side=0,anim=0,stage=0,i,j,width,height,px,py,rx,ry,ok=0,top=argc>1&&!strcmp(argv[1],"TOP"),alpha=argc>2?atoi(argv[2]):100;unsigned int before,seen=0;ULONG bytes,different;char path[120];static const char *const sides[]={"BOTTOM","TOP","LEFT","RIGHT"};static const char *const icons[]={"QAClock.info","Folder.iff","QAShell.info","QAMusic.info","Trash.iff","Settings.iff","Folder.iff","Settings.iff"};
 if(!qa_init())return 20;NDGlassCyberGfxBase=OpenLibrary("cybergraphics.library",41);s=LockPubScreen("Workbench");if(!s||!NDGlassCyberGfxBase)goto done;
 if(SystemTags("MakeDir RAM:RedrawEnv RAM:RedrawEnv/NovaDock",TAG_DONE)||SystemTags("Assign ENV: RAM:RedrawEnv",TAG_DONE)||SystemTags("Assign NovaDock: QATests:Dock",TAG_DONE))goto done;
 if(!qa_start("QATests:SlowDesktop"))goto done;for(i=0;i<200&&!qa_port("NovaDock.QA.SlowDesktop");++i)Delay(2);if(i==200)goto done;
 for(anim=0;anim<2;++anim)for(side=0;side<4;++side){stage=1;
  f=fopen("ENV:NovaDock/novadock.cfg","w");if(!f)goto stop;fprintf(f,"SIDE=%s\nPOSITION=START\nDRAG_BAR=NO\nITEM_SIZE=NORMAL\nTILE_WIDTH=86\nTILE_HEIGHT=76\nGAP=5\nLABELS=HIDDEN\nTHEME=Nord\nDOCK_BACKGROUND=SOLID\nBACKGROUND_TRANSPARENCY=%d\nICON_COLOURS=COLOURFUL\nALWAYS_ON_TOP=%s\nAUTO_HIDE=NO\nTOOLTIPS=NO\nREMOVABLE_MEDIA=NO\nRUNNING_APPS=NO\nHOVER_MAGNIFICATION=100\nHOVER_ANIMATION=%s\n",sides[side],alpha,top?"YES":"NO",anim?"YES":"NO");
  for(i=0;i<8;++i)fprintf(f,"LAUNCH=Tile%d|QATests:never-run-%d|NovaDock:Icons/%s|ON|AUTO||||NO|NO||||10|NO\n",i,i,icons[i]);fclose(f);if(nd_load_config(&cfg,"ENV:NovaDock/novadock.cfg")!=0||cfg.count<8||strcmp(cfg.items[0].label,"Tile0")){puts("FAIL invalid isolated test configuration");goto stop;}
  qa_pointer(s,s->Width/2,s->Height/2);if(!qa_start("QATests:Dock/NovaDock"))goto stop;
  stage=2;w=settled_side_dock(s);if(!w)goto stop;
  /* Focus the third tile; its neighbours expose cached background trails. */
  if(side<2){px=w->LeftEdge+7+2*(86+5)+43;py=side==0?s->Height-25:s->BarHeight+25;}else{px=side==2?25:s->Width-25;py=s->BarHeight+7+2*70+30;}
  stage=3;qa_pointer(s,px,py);Delay(100);w=settled_side_dock(s);if(!w||!qa_bounds(w))goto stop;stage=4;width=w->Width;height=w->Height;rx=w->LeftEdge;ry=w->TopEdge;bytes=(ULONG)width*height*3;reference=AllocVec(bytes,MEMF_PUBLIC);after=AllocVec(bytes,MEMF_PUBLIC);if(!reference||!after||!capture(s,w,reference,width,height))goto stop;
  sprintf(path,"QATests:reference-%s-%d.ppm",sides[side],anim);f=fopen(path,"wb");if(f){fprintf(f,"P6\n%d %d\n255\n",width,height);fwrite(reference,1,bytes,f);fclose(f);}
  stage=5;before=bg_count();
  cover=OpenWindowTags(NULL,WA_PubScreen,(ULONG)s,WA_Left,rx,WA_Top,ry,WA_Width,width,WA_Height,height,WA_Activate,FALSE,WA_Title,(ULONG)"Transient refresh QA",WA_Flags,WFLG_SMART_REFRESH,TAG_DONE);if(!cover)goto stop;Delay(90);CloseWindow(cover);cover=NULL;qa_pointer(s,s->Width/2,s->Height/2);Delay(250);
  seen+=bg_count()-before;
  /* Exercise rapid magnification before returning to the exact reference. */
  for(j=0;j<20;++j){qa_pointer(s,px+(side<2?((j%5)-2)*18:0),py+(side<2?0:((j%5)-2)*18));Delay(2);}qa_pointer(s,s->Width/2,s->Height/2);Delay(50);qa_pointer(s,px,py);Delay(100);stage=6;w=settled_side_dock(s);
  if(!w||w->LeftEdge!=rx||w->TopEdge!=ry||!capture(s,w,after,width,height)){printf("FAIL changed rest geometry side=%s anim=%d\n",sides[side],anim);goto stop;}
  different=0;for(i=0;(ULONG)i<bytes;++i)if(reference[i]!=after[i])++different;
  if(different){printf("FAIL ghost pixels side=%s anim=%d top=%d alpha=%d bytes=%lu refreshes=%u\n",sides[side],anim,top,alpha,different,bg_count()-before);sprintf(path,"QATests:redraw-%s-%d.ppm",sides[side],anim);photo(s,w,path);goto stop;}
  FreeVec(reference);FreeVec(after);reference=after=NULL;if(!qa_stop())goto stop;Delay(10);printf("PASS identical composed frame side=%s animate=%d top=%d alpha=%d refreshes=%u\n",sides[side],anim,top,alpha,bg_count()-before);
 }
 if(!seen){puts("FAIL no delayed SIMPLE_REFRESH repaint exercised");goto stop;}puts("PASS all-side delayed-desktop and hover redraw equality");ok=1;
 stop:if(!ok)printf("FAIL side=%s animate=%d top=%d alpha=%d stage=%d\n",sides[side<4 ? side:3],anim,top,alpha,stage);if(cover)CloseWindow(cover);if(qa_port(ND_CONTROL_PORT))(void)qa_stop();if(reference)FreeVec(reference);if(after)FreeVec(after);Forbid();bg=FindPort("NovaDock.QA.SlowDesktop");if(bg)Signal(bg->mp_SigTask,SIGBREAKF_CTRL_C);Permit();Delay(20);
 done:if(s)UnlockPubScreen(NULL,s);if(NDGlassCyberGfxBase)CloseLibrary(NDGlassCyberGfxBase);qa_finish();if(!ok)puts("FAIL side redraw regression");return ok?0:10;}
