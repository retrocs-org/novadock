#include <string.h>
#include "nd_qr.h"
static int utf8(const unsigned char *s)
{unsigned int left=0;unsigned long value=0,min=0;for(;*s;++s){if(left){if((*s&192)!=128)return 0;value=(value<<6)|(*s&63);if(!--left && (value<min || value>0x10ffffUL || (value>=0xd800 && value<=0xdfff)))return 0;}else if(*s>=128){if(*s>=194 && *s<=223){left=1;value=*s&31;min=128;}else if(*s<=239 && *s>=224){left=2;value=*s&15;min=2048;}else if(*s>=240 && *s<=244){left=3;value=*s&7;min=65536;}else return 0;}}return !left;}
int nd_qr_generate(const char *text,struct NDQR *out)
{
 unsigned char data[ND_QR_TEXT_MAX],bits[ND_QR_TEXT_MAX],work[ND_QR_BYTES];struct qrcodegen_Segment segments[2];unsigned int i,n=0,len;int unicode;
 if(!text || !out || !(len=(unsigned int)strlen(text)) || len>ND_QR_TEXT_MAX)return 0;unicode=utf8((const unsigned char *)text);
 for(i=0;i<len;++i){unsigned char c=(unsigned char)text[i];if(!unicode && c>=128){if(n+2>sizeof(data))return 0;data[n++]=(unsigned char)(192|(c>>6));data[n++]=(unsigned char)(128|(c&63));}else {if(n==sizeof(data))return 0;data[n++]=c;}}
 memset(out,0,sizeof(*out));segments[1]=qrcodegen_makeBytes(data,n,bits);
 /* The ECI segment needs its own stable byte while encoding uses work. */
 {unsigned char eci[4];segments[0]=qrcodegen_makeEci(26,eci);
 if(!qrcodegen_encodeSegmentsAdvanced(segments,2,qrcodegen_Ecc_MEDIUM,1,ND_QR_VERSION,qrcodegen_Mask_AUTO,1,work,out->code))return 0;}
 out->size=(unsigned int)qrcodegen_getSize(out->code);return out->size>=21 && out->size<=89;
}
int nd_qr_pixel(const struct NDQR *qr,int x,int y)
{return qr && qr->size>=21 && qr->size<=89 && x>=4 && y>=4 && x<(int)qr->size+4 && y<(int)qr->size+4 && qrcodegen_getModule(qr->code,x-4,y-4);}
static void put32(unsigned char *p,unsigned long n){p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
static void put16(unsigned char *p,unsigned int n){p[0]=(unsigned char)(n>>8);p[1]=(unsigned char)n;}
unsigned int nd_qr_ilbm(const struct NDQR *q,unsigned char *out,unsigned int cap)
{
 unsigned int width,rowbytes,body,total,x,y,pos;if(!q || q->size<21 || q->size>89 || !out)return 0;
 width=(q->size+8)*2;rowbytes=((width+15)/16)*2;body=rowbytes*width;total=62+body;if(total>cap)return 0;memset(out,0,total);
 memcpy(out,"FORM",4);put32(out+4,total-8);memcpy(out+8,"ILBMBMHD",8);put32(out+16,20);put16(out+20,width);put16(out+22,width);out[28]=1;out[34]=out[35]=1;put16(out+36,width);put16(out+38,width);
 memcpy(out+40,"CMAP",4);put32(out+44,6);out[48]=out[49]=out[50]=255;memcpy(out+54,"BODY",4);put32(out+58,body);
 for(y=0;y<width;++y)for(x=0;x<width;++x)if(nd_qr_pixel(q,x/2,y/2)){pos=62+y*rowbytes+x/8;out[pos]|=(unsigned char)(128>>(x%8));}return total;
}
