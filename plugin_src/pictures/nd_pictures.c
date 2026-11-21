#include <string.h>
#include <ctype.h>
#include "nd_pictures.h"
int nd_picture_name(const char *name)
{char ext[8];const char *dot;unsigned int i;if(!name||!*name||strlen(name)>95||strchr(name,':')||strchr(name,'/')||strchr(name,'\\'))return 0;for(i=0;name[i];++i)if((unsigned char)name[i]<32||name[i]==127)return 0;dot=strrchr(name,'.');if(!dot||strlen(dot)>6)return 0;for(i=0;dot[i];++i)ext[i]=(char)tolower((unsigned char)dot[i]);ext[i]=0;return !strcmp(ext,".iff")||!strcmp(ext,".ilbm")||!strcmp(ext,".lbm")||!strcmp(ext,".png")||!strcmp(ext,".jpg")||!strcmp(ext,".jpeg")||!strcmp(ext,".gif")||!strcmp(ext,".bmp");}
void nd_picture_palette(unsigned char rgb[16][3])
{static const unsigned char p[16][3]={{0,0,0},{32,32,32},{80,80,80},{128,128,128},{180,180,180},{224,224,224},{255,255,255},{128,32,32},{224,64,64},{32,128,32},{96,208,96},{32,64,160},{96,160,240},{224,192,64},{192,96,192},{128,88,48}};memcpy(rgb,p,sizeof(p));}
unsigned int nd_picture_color(const unsigned char rgb[3],const unsigned char p[16][3])
{unsigned int i,k,best=0;unsigned long score,lowest=0xffffffffUL;for(i=0;i<16;++i){score=0;for(k=0;k<3;++k){long d=(long)rgb[k]-p[i][k];score+=(unsigned long)(d*d);}if(score<lowest){lowest=score;best=i;}}return best;}
void nd_picture_thumbnail(const struct NDPictureImage *image,struct NDThumbnail *thumb)
{unsigned int x,y;memset(thumb,0,sizeof(*thumb));thumb->width=thumb->height=32;memcpy(thumb->rgb,image->rgb,sizeof(thumb->rgb));for(y=0;y<24;++y)for(x=0;x<32;++x)thumb->pixels[(y+4)*32+x]=image->pixels[y*4*128+x*4];}
static unsigned long be32(const unsigned char *p){return ((unsigned long)p[0]<<24)|((unsigned long)p[1]<<16)|((unsigned long)p[2]<<8)|p[3];}
static unsigned long le32(const unsigned char *p){return ((unsigned long)p[3]<<24)|((unsigned long)p[2]<<16)|((unsigned long)p[1]<<8)|p[0];}
int nd_picture_probe(FILE *f,unsigned int *width,unsigned int *height)
{unsigned char b[32];unsigned long w=0,h=0,pos=0,n;int marker,c;unsigned int chunks=0;if(!f||!width||!height)return 0;*width=*height=0;if(fseek(f,0,SEEK_SET)||fread(b,1,32,f)!=32)return 0;
 if(!memcmp(b,"\x89PNG\r\n\x1a\n",8)&&!memcmp(b+12,"IHDR",4)&&be32(b+8)==13){w=be32(b+16);h=be32(b+20);}
 else if(!memcmp(b,"GIF87a",6)||!memcmp(b,"GIF89a",6)){w=b[6]|((unsigned int)b[7]<<8);h=b[8]|((unsigned int)b[9]<<8);}
 else if(!memcmp(b,"BM",2)&&le32(b+14)>=40){w=le32(b+18);h=le32(b+22);if(h&0x80000000UL)h=(~h+1)&0xffffffffUL;}
 else if(!memcmp(b,"FORM",4)&&!memcmp(b+8,"ILBM",4)){pos=12;while(pos<65536&&++chunks<=64){if(fseek(f,(long)pos,SEEK_SET)||fread(b,1,8,f)!=8)return 0;n=be32(b+4);if(!memcmp(b,"BMHD",4)){if(n<20||fread(b,1,20,f)!=20)return 0;w=((unsigned int)b[0]<<8)|b[1];h=((unsigned int)b[2]<<8)|b[3];break;}if(n>65536-pos-8)return 0;pos+=8+n+(n&1);}}
 else if(b[0]==0xff&&b[1]==0xd8){if(fseek(f,2,SEEK_SET))return 0;while(ftell(f)<65536){if(fgetc(f)!=0xff)return 0;do{marker=fgetc(f);}while(marker==0xff);if(marker==EOF||marker==0xda||marker==0xd9)return 0;if(marker==0x01||(marker>=0xd0&&marker<=0xd7))continue;c=fgetc(f);if(c==EOF)return 0;n=(unsigned int)c<<8;c=fgetc(f);if(c==EOF)return 0;n|=(unsigned int)c;if(n<2||n>65536)return 0;
  if(marker>=0xc0&&marker<=0xcf&&marker!=0xc4&&marker!=0xc8&&marker!=0xcc){if(n<7||fread(b,1,5,f)!=5)return 0;h=((unsigned int)b[1]<<8)|b[2];w=((unsigned int)b[3]<<8)|b[4];break;}if(fseek(f,(long)n-2,SEEK_CUR))return 0;}}
 if(w<1||h<1||w>1024||h>1024)return 0;*width=(unsigned int)w;*height=(unsigned int)h;return 1;}
