/* Real cached shelf, ABI frame changes and magnification; isolated offline QA. */
#define main old_tiny_main
#include "native_tiny_metrics.c"
#undef main
#include "nd_artwork.h"
#ifndef ND_ICON_SIDE
#define ND_ICON_SIDE "BOTTOM"
#endif
#ifndef ND_ICON_HEIGHT
#define ND_ICON_HEIGHT 76
#endif
static struct Window *icon_dock(struct Screen *s)
{struct NDDiagnosticsSnapshot d;struct Window *w,*found=NULL;ULONG key;
 if(nd_control_diagnostics(&d)!=ND_CONTROL_OK || d.viewport_pending)return NULL;
 key=LockIBase(0);for(w=s->FirstWindow;w;w=w->NextWindow)if((w->Flags&WFLG_BORDERLESS)&&w->LeftEdge==d.viewport_left&&w->TopEdge==d.viewport_top&&w->Width==d.viewport_width&&w->Height==d.viewport_height){found=w;break;}UnlockIBase(key);return found;}
static struct Window *settled_icon_dock(struct Screen *s,int large,int hover)
{struct Window *w=NULL;int i;for(i=0;i<10;++i){w=icon_dock(s);if(w && (hover ? w->Height>(large ? 90:50):w->Height<=(large ? 90:50))){Delay(4);return icon_dock(s);}Delay(2);}return w;}
static unsigned int icon_flags;
static int icon_pixels(struct Screen *s,struct Window *w,int large,int hover)
{UBYTE rgb[256*3],bg[3],desktop[3];int y,x,n=0,rows=0,wide=0,width=w->Width-8,origin=hover ? 0:w->Height-(large ? 84:46),height;
 if(origin<0)origin=0;
 height=(icon_flags&ND_ARTWORK_VALUE) ? (hover ? w->Height/2:origin+(large ? ((icon_flags&ND_ARTWORK_HEADING) ? 36:48):32)):w->Height-8;if(width>256)width=256;
 sample_window=w;if(!line(s,bg,w->LeftEdge+w->Width/2,w->TopEdge+w->Height-3,1))return 0;
 /* Planar Workbench white is also a valid artwork ink. Do not remove a
  * desktop colour from the icon merely because the shared palette uses it. */
 (void)desktop;
 for(y=origin+4;y<height;++y){int count=0;if(!line(s,rgb,w->LeftEdge+4,w->TopEdge+y,width))return 0;
  for(x=8;x<width-8;++x)if(memcmp(rgb+x*3,bg,3)){++count;++n;}
  if(count>=3)++rows;if(count>wide)wide=count;
 }
 if(n<25||rows<4||wide<4)return 0;return n;}
static struct Window *wait_icon_art(struct Screen *s,int large,int hover)
{struct Window *w=NULL;int i;for(i=0;i<40;++i){w=icon_dock(s);if(w && qa_bounds(w) && icon_pixels(s,w,large,hover))return w;Delay(5);}printf("FAIL visible icon size=%d hover=%d\n",large,hover);return NULL;}
static int select_frame(const char *name,unsigned int state,unsigned int serial)
{FILE *f;unsigned int i,ack;for(i=0;i<150;++i){f=fopen("QATests:icon-state","w");if(f)break;Delay(1);}if(!f)return 0;fprintf(f,"%s %u %u\n",name,state,serial);if(fclose(f))return 0;
 for(i=0;i<150;++i){ack=0;f=fopen("QATests:icon-ack","r");if(f){(void)fscanf(f,"%u",&ack);fclose(f);}if(ack==serial){Delay(3);return 1;}Delay(2);}return 0;}
int main(void)
{struct Screen *s=NULL;struct Window *w;struct NDArtwork art;FILE *list,*f;char program[48],path[160],caption[24],shot[160];int large,i,ok=0;unsigned int frame,n,serial=0,states=0,draws=0;const unsigned char *r;
 if(!qa_init())return 20;NDGlassCyberGfxBase=OpenLibrary("cybergraphics.library",41);s=LockPubScreen("Workbench");if(!s||qa_port(ND_CONTROL_PORT))goto done;
 if(SystemTags("MakeDir RAM:IconEnv RAM:IconEnv/NovaDock",TAG_DONE)||SystemTags("Assign ENV: RAM:IconEnv",TAG_DONE)||SystemTags("Assign NovaDock: QATests:Dock",TAG_DONE))goto done;
 for(large=0;large<2;++large){
 list=fopen("QATests:icon-banks","r");if(!list)goto done;
 while(fscanf(list,"%47s",program)==1){
  sprintf(path,"QATests:Dock/Plugins/%s.assets/Artwork.ndart",program);memset(&art,0,sizeof(art));if(!nd_artwork_load(&art,path)){fclose(list);goto done;}
  r=art.data+nd_artwork_header(art.data);strcpy(caption,(const char *)r);f=fopen("QATests:icon-state","w");if(!f){nd_artwork_close(&art);fclose(list);goto done;}fprintf(f,"%s %u %u\n",caption,r[24],++serial);fclose(f);
  f=fopen("ENV:NovaDock/novadock.cfg","w");if(!f){nd_artwork_close(&art);fclose(list);goto done;}
  fprintf(f,"SIDE=%s\nPOSITION=CENTER\nDRAG_BAR=NO\nITEM_SIZE=%s\nTILE_WIDTH=86\nTILE_HEIGHT=%d\nGAP=5\nTHEME=Nord\nDOCK_BACKGROUND=SOLID\nICON_COLOURS=COLOURFUL\nLABELS=HIDDEN\nALWAYS_ON_TOP=YES\nAUTO_HIDE=NO\nTOOLTIPS=NO\nREMOVABLE_MEDIA=NO\nHOVER_MAGNIFICATION=100\nHOVER_ANIMATION=NO\nPLUGIN=Icon|Plugins/%s|TEST|METER|ON\n",ND_ICON_SIDE,large ? "NORMAL":"TINY",ND_ICON_HEIGHT,program);fclose(f);
  qa_pointer(s,10,30);if(!qa_start("QATests:Dock/NovaDock")){nd_artwork_close(&art);fclose(list);goto done;}
  for(i=0;i<300;++i){w=icon_dock(s);if(w&&qa_port(ND_CONTROL_PORT))break;Delay(2);}if(i==300){nd_artwork_close(&art);fclose(list);goto stop;}
  n=nd_artwork_u16(art.data+6);
  for(frame=0;frame<n;++frame){r=art.data+nd_artwork_header(art.data)+frame*32;icon_flags=art.data[5]==2 ? r[25]:0;
   qa_pointer(s,10,30);Delay(6);if(!select_frame((const char *)r,r[24],++serial)){printf("FAIL frame ack %s %u serial %u\n",program,r[24],serial);nd_artwork_close(&art);fclose(list);goto stop;}
   w=settled_icon_dock(s,large,0);w=wait_icon_art(s,large,0);if(!w){printf("%s state %u idle %dx%d\n",program,r[24],w ? w->Width:0,w ? w->Height:0);if(w)photo(s,w,"QATests:failed-state.ppm");nd_artwork_close(&art);fclose(list);goto stop;}
   ++draws;qa_pointer(s,w->LeftEdge+w->Width/2,w->TopEdge+w->Height/2);Delay(5);w=settled_icon_dock(s,large,1);
   w=wait_icon_art(s,large,1);if(!w){struct Window *failed=icon_dock(s);if(failed)photo(s,failed,"QATests:failed-hover.ppm");printf("%s state %u hover\n",program,r[24]);nd_artwork_close(&art);fclose(list);goto stop;}
   ++draws;++states;
   if(frame==0||(!strcmp(program,"Weather")&&r[24]==7)){sprintf(shot,"QATests:%s-%s-%u.ppm",program,large ? "normal":"tiny",r[24]);if(!photo(s,w,shot)){printf("FAIL photo %s\n",shot);nd_artwork_close(&art);fclose(list);goto stop;}}
  }
  nd_artwork_close(&art);sample_window=NULL;if(!qa_stop()){printf("FAIL stop %s\n",program);fclose(list);goto done;}
  Delay(4);printf("PASS %s %s %u states\n",program,large ? "Normal":"Tiny",n);
 }
 fclose(list);
 }
 /* EYES uses the live pointer/blink renderer, independent of bank artwork. */
 f=fopen("ENV:NovaDock/novadock.cfg","w");if(!f)goto done;fputs("SIDE=" ND_ICON_SIDE "\nALWAYS_ON_TOP=YES\nITEM_SIZE=TINY\nTILE_WIDTH=86\nTILE_HEIGHT=76\nDRAG_BAR=NO\nHOVER_MAGNIFICATION=100\nHOVER_ANIMATION=NO\nREMOVABLE_MEDIA=NO\nAUTO_HIDE=NO\nTOOLTIPS=NO\nTHEME=Nord\nPLUGIN=Eyes|Plugins/Eyes|EYES|METER|ON\nOPTION.EYES.BLINK=NO\n",f);fclose(f);
 icon_flags=0;qa_pointer(s,10,30);if(!qa_start("QATests:Dock/NovaDock"))goto done;Delay(100);w=icon_dock(s);if(!w||!icon_pixels(s,w,0,0))goto stop;if(!photo(s,w,"QATests:EYES-tiny.ppm"))goto stop;
 printf("PASS: plugin banks %u frame/size cases, %u idle/magnified native paints, Tiny EYES live artwork.\n",states,draws);ok=1;
 stop:sample_window=NULL;if(!qa_stop())ok=0;
 done:if(s)UnlockPubScreen(NULL,s);if(NDGlassCyberGfxBase)CloseLibrary(NDGlassCyberGfxBase);qa_finish();if(!ok)puts("FAIL: native plugin state icons");return ok ? 0:10;}
