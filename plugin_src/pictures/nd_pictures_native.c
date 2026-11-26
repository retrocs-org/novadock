#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/pictureclass.h>
#include <graphics/gfxbase.h>
#include <graphics/rastport.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/datatypes.h>
#include <string.h>
#include <stdio.h>
#include "nd_pictures.h"
#include "../screenshot/nd_cybergraphics.h"
#if defined(ND_PLUGIN_RESOURCES)
#include "nd_plugin_resources.h"
#endif
struct Library *DataTypesBase=NULL,*CyberGfxBase=NULL;struct GfxBase *GfxBase=NULL;
static int alive(const char *parent){int ok;if(!parent)return 1;Forbid();ok=FindPort((STRPTR)parent)!=NULL;Permit();return ok;}
static int scan(struct NDPicturesJob *j,const char *parent)
{BPTR lock;struct FileInfoBlock *fib;unsigned int n=0,k,i;j->count=j->truncated=0;lock=Lock(j->drawer,ACCESS_READ);if(!lock)return 0;fib=AllocDosObject(DOS_FIB,NULL);if(!fib){UnLock(lock);return 0;}if(!Examine(lock,fib)||fib->fib_DirEntryType<0)goto fail;
 while(ExNext(lock,fib)){if(!alive(parent))goto fail;if(++n>4096){strcpy(j->problem,"Drawer exceeds 4096 entries; choose a smaller drawer");goto fail;}if(fib->fib_DirEntryType>=0||fib->fib_Size<1||fib->fib_Size>4L*1024L*1024L||!nd_picture_name(fib->fib_FileName))continue;
  for(i=0;i<j->count&&strcmp(j->names[i],fib->fib_FileName)<0;++i){}if(j->count==ND_PICTURES_MAX){j->truncated=1;if(i>=j->count)continue;}for(k=j->count<ND_PICTURES_MAX ? j->count:ND_PICTURES_MAX-1;k>i;--k)strcpy(j->names[k],j->names[k-1]);strcpy(j->names[i],fib->fib_FileName);if(j->count<ND_PICTURES_MAX)++j->count;
 }if(IoErr()!=ERROR_NO_MORE_ENTRIES)goto fail;FreeDosObject(DOS_FIB,fib);UnLock(lock);j->index=0;return 1;
 fail:FreeDosObject(DOS_FIB,fib);UnLock(lock);return 0;}
static int decode(struct NDPicturesJob *j,const char *parent)
{Object *object=NULL;struct BitMapHeader *h=NULL;struct BitMap *bitmap=NULL;struct ColorRegister *palette=NULL;struct RastPort rp;char path[340];ULONG colors=0;unsigned int w,hgt,x,y,sx,sy,left,top;LONG index;unsigned char pixel[3];int ok=0;FILE *file;unsigned int probew,probeh;struct FileInfoBlock *fib=NULL;BPTR lock;
 if(j->index>=j->count||!memchr(j->names[j->index],0,96)||!nd_picture_name(j->names[j->index]))return 0;strcpy(path,j->drawer);if(!AddPart(path,j->names[j->index],sizeof(path)))return 0;
 lock=Lock(path,ACCESS_READ);if(!lock)return 0;fib=AllocDosObject(DOS_FIB,NULL);if(!fib){UnLock(lock);return 0;}ok=Examine(lock,fib)&&fib->fib_DirEntryType<0&&fib->fib_Size>0&&fib->fib_Size<=4L*1024L*1024L;FreeDosObject(DOS_FIB,fib);UnLock(lock);if(!ok)return 0;file=fopen(path,"rb");if(!file)return 0;ok=nd_picture_probe(file,&probew,&probeh);fclose(file);if(!ok){strcpy(j->problem,"Unsupported or oversized image header (maximum 1024x1024)");return 0;}ok=0;
 DataTypesBase=OpenLibrary("datatypes.library",39);GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);if(!DataTypesBase||!GfxBase)goto done;
 object=NewDTObject(path,DTA_SourceType,DTST_FILE,DTA_GroupID,GID_PICTURE,PDTA_Remap,FALSE,TAG_DONE);if(!object)goto done;GetDTAttrs(object,PDTA_BitMapHeader,(ULONG)&h,TAG_DONE);if(!h||!h->bmh_Width||!h->bmh_Height||h->bmh_Width>1024||h->bmh_Height>1024)goto done;
 if(!DoDTMethod(object,NULL,NULL,DTM_PROCLAYOUT,NULL,TRUE))goto done;GetDTAttrs(object,PDTA_BitMap,(ULONG)&bitmap,PDTA_ColorRegisters,(ULONG)&palette,PDTA_NumColors,(ULONG)&colors,TAG_DONE);if(!bitmap)goto done;if(bitmap->Depth<=8&&(!palette||!colors||colors>256))goto done;if(bitmap->Depth>8){CyberGfxBase=OpenLibrary("cybergraphics.library",41);if(!CyberGfxBase||!nd_cgx_bitmap(bitmap))goto done;}
 memset(&j->image,0,sizeof(j->image));nd_picture_palette(j->image.rgb);j->image.source_width=h->bmh_Width;j->image.source_height=h->bmh_Height;w=128;hgt=(unsigned int)((unsigned long)h->bmh_Height*128UL/h->bmh_Width);if(hgt>96){hgt=96;w=(unsigned int)((unsigned long)h->bmh_Width*96UL/h->bmh_Height);}if(!w)w=1;if(!hgt)hgt=1;left=(128-w)/2;top=(96-hgt)/2;InitRastPort(&rp);rp.BitMap=bitmap;
 for(y=0;y<hgt;++y){if(!alive(parent))goto done;sy=(unsigned int)((unsigned long)y*h->bmh_Height/hgt);for(x=0;x<w;++x){sx=(unsigned int)((unsigned long)x*h->bmh_Width/w);if(bitmap->Depth>8){if(nd_cgx_rgb_pixel(pixel,&rp,sx,sy)!=1)goto done;}else{index=ReadPixel(&rp,sx,sy);if(index<0||(ULONG)index>=colors)goto done;pixel[0]=palette[index].red;pixel[1]=palette[index].green;pixel[2]=palette[index].blue;}j->image.pixels[(y+top)*128+x+left]=(unsigned char)nd_picture_color(pixel,j->image.rgb);}}
 ok=1;
 done:if(object)DisposeDTObject(object);if(CyberGfxBase)CloseLibrary(CyberGfxBase);if(DataTypesBase)CloseLibrary(DataTypesBase);if(GfxBase)CloseLibrary((struct Library *)GfxBase);CyberGfxBase=DataTypesBase=NULL;GfxBase=NULL;return ok;}
int nd_pictures_work(unsigned int action,void *data,unsigned int bytes,const char *parent)
{struct NDPicturesJob *j=data;int ok;if(bytes!=sizeof(*j)||!memchr(j->drawer,0,sizeof(j->drawer))||!j->drawer[0]||!strchr(j->drawer,':')||j->count>32)return 0;j->problem[0]=0;j->image.source_width=j->image.source_height=0;if(action==1){if(!scan(j,parent)){if(!j->problem[0])strcpy(j->problem,"Cannot read picture drawer");return 0;}if(!j->count){strcpy(j->problem,"No supported pictures found (maximum file size 4 MiB)");return 2;}}
 else if(action!=2)return 0;ok=decode(j,parent);if(!ok&&!j->problem[0])strcpy(j->problem,"Cannot decode: matching DataType required; maximum 1024x1024");return ok;}
