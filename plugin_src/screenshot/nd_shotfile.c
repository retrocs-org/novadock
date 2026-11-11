/* Portable ILBM screenshot writer (see nd_shotfile.h). */
#include "nd_shotfile.h"
#include <stdlib.h>
#include <string.h>

static void be32(unsigned char *p,unsigned long v)
{
    p[0]=(unsigned char)(v>>24);p[1]=(unsigned char)(v>>16);
    p[2]=(unsigned char)(v>>8);p[3]=(unsigned char)v;
}
static void be16(unsigned char *p,unsigned int v)
{
    p[0]=(unsigned char)(v>>8);p[1]=(unsigned char)v;
}
unsigned int nd_byterun1(const unsigned char *src,unsigned int n,
                         unsigned char *dst)
{
    unsigned int in=0,out=0,run,lit;
    while (in<n) {
        /* A repeat of three or more is worth a run. */
        run=1;
        while (in+run<n && run<128 && src[in+run]==src[in]) ++run;
        if (run>=3) {
            dst[out++]=(unsigned char)(257-run);
            dst[out++]=src[in];
            in+=run;
            continue;
        }
        /* Literal: until the next run of three, or 128 bytes. */
        lit=0;
        while (in+lit<n && lit<128) {
            if (in+lit+2<n && src[in+lit]==src[in+lit+1] &&
                src[in+lit]==src[in+lit+2]) break;
            ++lit;
        }
        dst[out++]=(unsigned char)(lit-1);
        memcpy(dst+out,src+in,lit);
        out+=lit;in+=lit;
    }
    return out;
}
static int put(struct NDShotWriter *w,const void *data,unsigned long n)
{
    if (!w->failed && n && fwrite(data,1,(size_t)n,w->file)!=(size_t)n)
        w->failed=1;
    return !w->failed;
}
static int chunk(struct NDShotWriter *w,const char *id,unsigned long size)
{
    unsigned char h[8];
    memcpy(h,id,4);be32(h+4,size);
    return put(w,h,8);
}
int nd_shot_begin(struct NDShotWriter *w,const char *path,unsigned int width,
                  unsigned int height,unsigned int depth,
                  const unsigned char *palette,unsigned int colors,
                  unsigned long mode_id)
{
    unsigned char b[20],zero=0;
    unsigned long cmap;
    memset(w,0,sizeof(*w));
    if (!path || !width || !height || width>ND_SHOT_MAX_WIDTH || height>32767 ||
        depth<1 || depth>8 || colors>256 || (colors && !palette)) return 0;
    w->width=width;w->height=height;w->depth=depth;
    w->plane_bytes=((width+15U)/16U)*2U;
    w->planar=(unsigned char *)malloc(w->plane_bytes);
    w->packed=(unsigned char *)malloc(w->plane_bytes+w->plane_bytes/128U+2U);
    if (!w->planar || !w->packed) {w->failed=1;return 0;}
    w->file=fopen(path,"wb");
    if (!w->file) {w->failed=1;return 0;}
    put(w,"FORM\0\0\0\0ILBM",12);
    w->form_at=4;
    chunk(w,"BMHD",20);
    memset(b,0,sizeof(b));
    be16(b,width);be16(b+2,height);          /* w, h; x=y=0 */
    b[8]=(unsigned char)depth;               /* nPlanes */
    b[9]=0;                                  /* masking: none */
    b[10]=1;                                 /* ByteRun1 */
    b[14]=1;b[15]=1;                         /* x/y aspect 1:1 */
    be16(b+16,width);be16(b+18,height);      /* page size */
    put(w,b,20);
    cmap=(unsigned long)colors*3UL;
    if (cmap) {
        chunk(w,"CMAP",cmap);
        put(w,palette,cmap);
        if (cmap&1UL) put(w,&zero,1);
    }
    chunk(w,"CAMG",4);
    be32(b,mode_id);put(w,b,4);
    w->body_at=ftell(w->file);
    chunk(w,"BODY",0);                       /* size patched at the end */
    return !w->failed;
}
int nd_shot_row(struct NDShotWriter *w,const unsigned char *pens)
{
    unsigned int plane,x,n;
    if (!w || w->failed || !pens || w->row>=w->height) return 0;
    for (plane=0;plane<w->depth;++plane) {
        unsigned char bit=(unsigned char)(1U<<plane);
        memset(w->planar,0,w->plane_bytes);
        for (x=0;x<w->width;++x)
            if (pens[x]&bit) w->planar[x>>3]|=(unsigned char)(0x80U>>(x&7U));
        n=nd_byterun1(w->planar,w->plane_bytes,w->packed);
        if (!put(w,w->packed,n)) return 0;
    }
    ++w->row;
    return 1;
}
int nd_shot_end(struct NDShotWriter *w,const char *path)
{
    unsigned char b[4],zero=0;
    long end,body;
    int ok=w && !w->failed && w->file && w->row==w->height;
    if (ok) {
        end=ftell(w->file);
        body=end-w->body_at-8;
        if (body&1L) {put(w,&zero,1);++end;}
        be32(b,(unsigned long)body);
        if (fseek(w->file,w->body_at+4,SEEK_SET) || !put(w,b,4)) ok=0;
        be32(b,(unsigned long)(end-8));
        if (ok && (fseek(w->file,w->form_at,SEEK_SET) || !put(w,b,4))) ok=0;
        ok=ok && !w->failed;
    }
    if (w && w->file && fclose(w->file)) ok=0;
    if (w) {
        free(w->planar);free(w->packed);
        w->file=NULL;w->planar=w->packed=NULL;
    }
    if (!ok && path) remove(path);
    return ok;
}
int nd_shot_name(char *out,unsigned int size,const char *drawer,long days,
                 long minute,long tick,int serial)
{
    static const int month_days[12]={31,28,31,30,31,30,31,31,30,31,30,31};
    long year=1978,left=days;
    int month=0,leap,second;
    char name[80];
    size_t n;
    if (!out || !drawer || days<0 || days>2900000L || minute<0 ||
        minute>=1440L || tick<0) return 0;
    for (;;) {
        leap=(year%4==0 && year%100!=0) || year%400==0;
        if (left<365+leap) break;
        left-=365+leap;++year;
    }
    while (month<12) {
        int d=month_days[month]+(month==1 && leap);
        if (left<d) break;
        left-=d;++month;
    }
    second=(int)((tick/50L)%60L);
    sprintf(name,"Screenshot-%04ld%02d%02ld-%02ld%02ld%02d",year,month+1,
            left+1,(minute/60L)%24L,minute%60L,second);
    if (serial>0) sprintf(name+strlen(name),"-%d",serial);
    strcat(name,".iff");
    n=strlen(drawer);
    if (n+strlen(name)+2>size) return 0;
    strcpy(out,drawer);
    if (n && out[n-1]!=':' && out[n-1]!='/') strcat(out,"/");
    strcat(out,name);
    return 1;
}
