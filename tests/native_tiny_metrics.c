/* Native Tiny metric artwork, real plugin values, isolated offline drives. */
#include "native_qa_ui.h"
#include "nd_prefs_layout.h"
#include "nd_config.h"
struct Library *NDGlassCyberGfxBase;
#include "nd_rtg_pixels.h"
long __stack=65536;
static struct Window *dock_window(struct Screen *s)
{struct Window *w,*found=NULL;ULONG key=LockIBase(0);for(w=s->FirstWindow;w;w=w->NextWindow)if((w->Flags&WFLG_BORDERLESS)&&w->Width>150&&w->TopEdge>s->Height/2){found=w;break;}UnlockIBase(key);return found;}
static struct Window *sample_window;
static int line(struct Screen *s,UBYTE *rgb,int x,int y,int width)
{ULONG colors[256*3];int i,depth=s->RastPort.BitMap->Depth;struct RastPort raw=s->RastPort;raw.Layer=NULL;if(NDGlassCyberGfxBase&&nd_glass_bitmap(s->RastPort.BitMap))return __nd_glass_read(NDGlassCyberGfxBase,rgb,0,0,width*3,&s->RastPort,x,y,width,1,0)==width;
 if(depth>8)return 0;GetRGB32(s->ViewPort.ColorMap,0,1UL<<depth,colors);for(i=0;i<width;++i){LONG p=ReadPixel(&raw,x+i,y);if(p<0||p>=(1L<<depth))return 0;rgb[i*3]=colors[p*3]>>24;rgb[i*3+1]=colors[p*3+1]>>24;rgb[i*3+2]=colors[p*3+2]>>24;}return 1;}
static int photo(struct Screen *s,struct Window *w,const char *name)
{UBYTE *rgb;FILE *f;int y,ok=1;rgb=AllocVec(w->Width*3UL,MEMF_PUBLIC);if(!rgb)return 0;f=fopen(name,"wb");if(!f){FreeVec(rgb);return 0;}fprintf(f,"P6\n%d %d\n255\n",w->Width,w->Height);WaitBlit();for(y=0;y<w->Height;++y)if(!line(s,rgb,w->LeftEdge,w->TopEdge+y,w->Width)||fwrite(rgb,3,w->Width,f)!=(size_t)w->Width){ok=0;break;}if(fclose(f))ok=0;FreeVec(rgb);return ok;}
static int artwork(struct Screen *s,struct Window *w,int slot,int kind)
{
 UBYTE rgb[43*3],background[3];int x,y,n=0,ink=0,columns[43],maximum=0,planar=!(NDGlassCyberGfxBase&&nd_glass_bitmap(s->RastPort.BitMap));
 memset(columns,0,sizeof(columns));if(!line(s,background,w->LeftEdge+8+slot*46,w->TopEdge+8,1))return 0;
 for(y=9;y<44;++y){
  if(!line(s,rgb,w->LeftEdge+7+slot*46,w->TopEdge+y,43))return 0;
  for(x=2;x<41;++x){int r=rgb[x*3],g=rgb[x*3+1],b=rgb[x*3+2];
   if(kind<2 ? g>r+12&&g>b+12:kind==2 ? r>g+20&&g>b+10:b>110&&b>r+20&&b>g+12)++n;
   if(r!=background[0]||g!=background[1]||b!=background[2]){++ink;++columns[x];}
  }
 }
 for(x=2;x<41;++x)if(columns[x]>maximum)maximum=columns[x];
 printf("Tiny %s: color=%d shape=%d vertical=%d planar=%d.\n",kind==0 ? "CPU":kind==1 ? "RAM":kind==2 ? "Clock":"Network",n,ink,maximum,planar);
 return planar ? ink>=180&&maximum>=15:n>=45;
}
/* Measure real painted bounds, including stale pixels after a size switch. */
static int preview_bounds(struct Screen *s,struct Window *w,int bounds[4])
{
 struct NDPrefsLayout layout;UBYTE bg[3],*rgb;int compact,fx=8,fy=8;
 int x,y,left,right,top,bottom,width;
 if(s->RastPort.Font){fx=s->RastPort.Font->tf_XSize;fy=s->RastPort.Font->tf_YSize;}
 compact=(s->Height-s->BarHeight)*8/fy<480 || s->Width*8/fx<720;
 nd_prefs_layout_init(&layout,fx,fy,s->Width,s->Height,s->BarHeight,compact ? 620:700,compact ? 239:468);
 left=nd_prefs_layout_x(&layout,140);right=nd_prefs_layout_x(&layout,layout.base_width-12);
 top=nd_prefs_layout_y(&layout,46);bottom=nd_prefs_layout_y(&layout,compact ? 86:138);width=right-left+1;
 bounds[0]=right+1;bounds[1]=bottom+1;bounds[2]=left-1;bounds[3]=top-1;
 if(!line(s,bg,w->LeftEdge+left-1,w->TopEdge+top-1,1))return 0;
 rgb=AllocVec(width*3UL,MEMF_PUBLIC);if(!rgb)return 0;
 for(y=top;y<=bottom;++y){
  if(!line(s,rgb,w->LeftEdge+left,w->TopEdge+y,width)){FreeVec(rgb);return 0;}
  for(x=0;x<width;++x)if(memcmp(rgb+x*3,bg,3)){
   if(left+x<bounds[0])bounds[0]=left+x;if(left+x>bounds[2])bounds[2]=left+x;
   if(y<bounds[1])bounds[1]=y;if(y>bounds[3])bounds[3]=y;
  }
 }
 FreeVec(rgb);return bounds[2]>=bounds[0]&&bounds[3]>=bounds[1];
}
static int preview_cycle(struct Window *w,UWORD id,ULONG value)
{
 struct Gadget *g=qa_gadget(w,id);ULONG active=~0UL;
 if(!g||GT_GetGadgetAttrs(g,w,NULL,GTCY_Active,(ULONG)&active,TAG_DONE)!=1)return 0;
 /* GadTools advances cycle gadgets while filtering the event. Do not post
  * another press when the requested choice is already selected. */
 if(active!=value){
  if(!qa_post(w,IDCMP_GADGETUP,(UWORD)value,id))return 0;
  Delay(12);active=~0UL;
  if(GT_GetGadgetAttrs(g,w,NULL,GTCY_Active,(ULONG)&active,TAG_DONE)!=1)return 0;
 }
 return active==value;
}
static int preview_sizes(struct Screen *s,struct Window *w)
{
 int background,normal[4],tiny[4],restored[4];
 for(background=0;background<2;++background){
  if(!preview_cycle(w,128,background)||!preview_cycle(w,124,ND_ITEM_SIZE_NORMAL))return 0;
  Delay(12);if(!preview_bounds(s,w,normal))return 0;
  if(!photo(s,w,background ? "QATests:preview-normal-artwork.ppm":"QATests:preview-normal-solid.ppm"))return 0;
  if(!preview_cycle(w,124,ND_ITEM_SIZE_TINY))return 0;
  Delay(12);if(!preview_bounds(s,w,tiny))return 0;
  printf("Preview %s Normal=%d,%d %dx%d Tiny=%d,%d %dx%d.\n",background ? "artwork":"solid",normal[0],normal[1],normal[2]-normal[0]+1,normal[3]-normal[1]+1,tiny[0],tiny[1],tiny[2]-tiny[0]+1,tiny[3]-tiny[1]+1);
  if(tiny[2]-tiny[0]>=normal[2]-normal[0] || tiny[3]-tiny[1]>=normal[3]-normal[1])return 0;
  if(tiny[0]<normal[0]||tiny[1]<normal[1]||tiny[2]>normal[2]||tiny[3]>normal[3])return 0;
  if(!photo(s,w,background ? "QATests:preview-tiny-artwork.ppm":"QATests:preview-tiny-solid.ppm"))return 0;
  if(!preview_cycle(w,124,ND_ITEM_SIZE_NORMAL))return 0;
  Delay(12);if(!preview_bounds(s,w,restored)||memcmp(normal,restored,sizeof(normal)))return 0;
 }
 if(!preview_cycle(w,124,ND_ITEM_SIZE_TINY))return 0;
 Delay(12);puts("PASS: native Normal/Tiny/Normal preview shelf sizes, restored bounds and no stale edges (solid/artwork)");return 1;
}
int main(void)
{struct Screen *s=NULL;struct Window *w=NULL;FILE *f;int i,idle_height,ok=0;if(!qa_init())return 20;NDGlassCyberGfxBase=OpenLibrary("cybergraphics.library",41);s=LockPubScreen("Workbench");if(!s)goto done;
 if(qa_port(ND_CONTROL_PORT)){puts("FAIL: isolated fixture must have no existing dock");goto done;}
 if(SystemTags("MakeDir RAM:TinyEnv RAM:TinyEnv/NovaDock",TAG_DONE)||SystemTags("Assign ENV: RAM:TinyEnv",TAG_DONE)||SystemTags("Assign NovaDock: QATests:Dock",TAG_DONE))goto done;
 f=fopen("ENV:NovaDock/novadock.cfg","w");if(!f)goto done;
 fputs("SIDE=BOTTOM\nPOSITION=CENTER\nDRAG_BAR=NO\nITEM_SIZE=TINY\nTILE_WIDTH=86\nTILE_HEIGHT=76\nGAP=5\nTHEME=Nord\nDOCK_BACKGROUND=SOLID\nICON_COLOURS=COLOURFUL\nALWAYS_ON_TOP=YES\nAUTO_HIDE=NO\nTOOLTIPS=NO\nREMOVABLE_MEDIA=NO\nHOVER_MAGNIFICATION=100\nHOVER_ANIMATION=NO\nPLUGIN=CPU|Plugins/Activity|CPU|METER|ON\nPLUGIN=RAM|Plugins/Memory|RAM|METER|ON\nPLUGIN=Clock|Plugins/Clock|CLOCK|METER|ON\nPLUGIN=Network|Plugins/Network|NET|METER|ON\n",f);fclose(f);
 qa_pointer(s,10,30);if(!qa_start("QATests:Dock/NovaDock"))goto done;for(i=0;i<400;++i){w=dock_window(s);if(w&&qa_port(ND_CONTROL_PORT))break;Delay(2);}if(i==400)goto stop;Delay(150);w=dock_window(s);if(!w||w->Height<38||w->Height>60||!qa_bounds(w))goto stop;
 sample_window=w;printf("Depth=%u actual=%lu map=%u screenpen=%ld windowpen=%ld.\n",s->RastPort.BitMap->Depth,GetBitMapAttr(s->RastPort.BitMap,BMA_DEPTH),s->ViewPort.ColorMap->Count,ReadPixel(&s->RastPort,w->LeftEdge+10,w->TopEdge+10),ReadPixel(w->RPort,10,10));
 printf("Tiny dock native bounds %d,%d %dx%d.\n",w->LeftEdge,w->TopEdge,w->Width,w->Height);
 if(!photo(s,w,"QATests:tiny-icons.ppm"))goto stop;
 for(i=0;i<4;++i)if(!artwork(s,w,i,i))goto stop;
 idle_height=w->Height;qa_pointer(s,w->LeftEdge+25,w->TopEdge+20);Delay(30);w=dock_window(s);if(!w||w->Height<=idle_height||!qa_bounds(w)||!photo(s,w,"QATests:tiny-hover.ppm"))goto stop;
 qa_pointer(s,10,30);Delay(40);
 if(!qa_start("QATests:Dock/NovaDockPrefs PAGE=APPEARANCE"))goto stop;
 w=qa_wait_window("NovaDock Preferences");
 if(!w||!qa_bounds(w))goto stop;
 Delay(50);sample_window=w;
 if(!preview_sizes(s,w)||!photo(s,w,"QATests:tiny-appearance.ppm")||!qa_close("NovaDock Preferences"))goto stop;
 sample_window=NULL;
 puts("PASS: native Tiny CPU/RAM/digital-clock/network artwork, live plugin values, magnified window bounds and Appearance preview.");ok=1;
 stop:if(!qa_stop())ok=0;
 done:if(s)UnlockPubScreen(NULL,s);if(NDGlassCyberGfxBase)CloseLibrary(NDGlassCyberGfxBase);qa_finish();if(!ok)puts("FAIL: native Tiny icon rendering");return ok ? 0:10;}
