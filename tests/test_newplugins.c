/* Portable cores of the Mixer, Screenshot and Screen-mode plugins. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_ahiprefs.h"
#include "nd_shotfile.h"
#include "nd_scrmode.h"
#include "nd_ilbm.h"

static void be32(unsigned char *p,unsigned long v)
{p[0]=(unsigned char)(v>>24);p[1]=(unsigned char)(v>>16);p[2]=(unsigned char)(v>>8);p[3]=(unsigned char)v;}
static unsigned long rd32(const unsigned char *p)
{return ((unsigned long)p[0]<<24)|((unsigned long)p[1]<<16)|((unsigned long)p[2]<<8)|p[3];}

/* An AHI prefs file: PRHD, AHIG, then AHIU chunks for units 0 and 255. */
static unsigned long ahi_file(unsigned char *b,unsigned long v0,unsigned long v255)
{
    unsigned long n=12;
    memcpy(b,"FORM",4);memcpy(b+8,"PREF",4);
    memcpy(b+n,"PRHD",4);be32(b+n+4,6);memset(b+n+8,0,6);n+=14;
    memcpy(b+n,"AHIG",4);be32(b+n+4,22);memset(b+n+8,0,22);n+=30;
    memcpy(b+n,"AHIU",4);be32(b+n+4,32);memset(b+n+8,0,32);
    b[n+8]=0;be32(b+n+8+20,v0);n+=40;
    memcpy(b+n,"AHIU",4);be32(b+n+4,32);memset(b+n+8,0,32);
    b[n+8]=255;be32(b+n+8+20,v255);n+=40;
    be32(b+4,n-8);
    return n;
}
static void test_ahi(void)
{
    unsigned char b[256];
    unsigned long n=ahi_file(b,0x8000UL,0xC000UL),v=0;
    assert(nd_ahi_prefs_volume(b,n,&v) && v==0xC000UL); /* low-level unit wins */
    assert(nd_ahi_percent(v)==75 && nd_ahi_percent(0)==0 &&
           nd_ahi_percent(0x10000UL)==100 && nd_ahi_percent(0x20000UL)==100);
    assert(nd_ahi_fixed(50)==0x8000UL && nd_ahi_fixed(100)==0x10000UL &&
           nd_ahi_fixed(0)==0 && nd_ahi_fixed(250)==0x10000UL);
    assert(nd_ahi_prefs_set_volume(b,n,0x4000UL)==2);
    assert(nd_ahi_prefs_volume(b,n,&v) && v==0x4000UL);
    assert(rd32(b+12+14+30+8+20)==0x4000UL);              /* unit 0 changed too */
    /* Truncated, wrong type, oversized chunk and huge FORM are rejected. */
    assert(!nd_ahi_prefs_volume(b,20,&v));
    memcpy(b+8,"ILBM",4);assert(!nd_ahi_prefs_volume(b,n,&v));memcpy(b+8,"PREF",4);
    be32(b+4,0xFFFFFFF0UL);assert(!nd_ahi_prefs_volume(b,n,&v));be32(b+4,n-8);
    be32(b+12+14+30+4,5000);assert(!nd_ahi_prefs_set_volume(b,n,1));
}
/* Decode ByteRun1 for the round trip. */
static unsigned int unpack(const unsigned char *s,unsigned int n,unsigned char *d)
{
    unsigned int i=0,o=0;
    while (i<n) {
        signed char c=(signed char)s[i++];
        if (c>=0) {memcpy(d+o,s+i,(size_t)c+1);o+=(unsigned int)c+1;i+=(unsigned int)c+1;}
        else if (c!=-128) {memset(d+o,s[i++],(size_t)(1-c));o+=(unsigned int)(1-c);}
    }
    return o;
}
static void test_byterun1(void)
{
    unsigned char src[700],packed[720],out[700];
    unsigned int t,n,i,len;
    srand(7);
    for (t=0;t<2000;++t) {
        len=(unsigned int)(rand()%700)+1;
        for (i=0;i<len;++i) src[i]=(unsigned char)(rand()%4 ? rand()%3 : rand());
        n=nd_byterun1(src,len,packed);
        assert(n<=len+len/128+2);
        assert(unpack(packed,n,out)==len && !memcmp(src,out,len));
    }
}
static void test_shot(void)
{
    struct NDShotWriter w;
    struct NDILBM *icon=(struct NDILBM *)malloc(sizeof(*icon));
    unsigned char palette[48],row[64];
    const char *path="tests/nd_shot_test.iff";
    unsigned int x,y,c;
    char name[96];
    for (c=0;c<16;++c) {palette[c*3]=(unsigned char)(c*16);palette[c*3+1]=(unsigned char)(255-c*16);palette[c*3+2]=(unsigned char)c;}
    assert(nd_shot_begin(&w,path,45,21,4,palette,16,0x29004UL));
    for (y=0;y<21;++y) {
        for (x=0;x<45;++x) row[x]=(unsigned char)((x/3+y)%16);
        assert(nd_shot_row(&w,row));
    }
    assert(nd_shot_end(&w,path));
    /* The dock's own ILBM reader accepts it, pixel for pixel. */
    assert(icon && nd_ilbm_load(path,icon));
    assert(icon->width==45 && icon->height==21 && icon->colors==16);
    for (c=0;c<16;++c) assert(!memcmp(icon->palette[c],palette+c*3,3));
    for (y=0;y<21;++y) for (x=0;x<45;++x)
        assert(icon->pixels[y*ND_ICON_MAX_DIM+x]==(x/3+y)%16);
    remove(path);
    /* An unfinished image is not left behind. */
    assert(nd_shot_begin(&w,path,10,10,2,palette,4,0));
    assert(nd_shot_row(&w,row));
    assert(!nd_shot_end(&w,path));
    assert(!fopen(path,"rb"));
    assert(!nd_shot_begin(&w,path,10,10,9,palette,4,0));
    /* 1 Jan 1978 00:00:00 and 29 Feb 2024 23:59:59 (16860 days later). */
    assert(nd_shot_name(name,sizeof(name),"RAM:",0,0,0,0) &&
           !strcmp(name,"RAM:Screenshot-19780101-000000.iff"));
    assert(nd_shot_name(name,sizeof(name),"Work:Pics",16860,1439,59*50,2) &&
           !strcmp(name,"Work:Pics/Screenshot-20240229-235959-2.iff"));
    assert(!nd_shot_name(name,20,"RAM:",0,0,0,0));
    free(icon);
}
static void test_scrmode(void)
{
    unsigned char b[128];
    struct NDScreenModePreset p;
    char text[16];
    unsigned long n=12;
    memcpy(b,"FORM",4);memcpy(b+8,"PREF",4);
    memcpy(b+n,"PRHD",4);be32(b+n+4,6);memset(b+n+8,0,6);n+=14;
    memcpy(b+n,"SCRM",4);be32(b+n+4,28);memset(b+n+8,0,28);
    be32(b+n+8+16,0x50031004UL);
    b[n+28]=0x03;b[n+29]=0x20;b[n+30]=0x02;b[n+31]=0x58;b[n+32]=0;b[n+33]=8;
    n+=36;be32(b+4,n-8);
    assert(nd_scrmode_parse(b,n,&p) && p.display_id==0x50031004UL &&
           p.width==800 && p.height==600 && p.depth==8);
    assert(nd_scrmode_text(p.width,p.height,p.depth,text,sizeof(text)) &&
           !strcmp(text,"800x600x8"));
    assert(nd_scrmode_text(0xFFFF,0xFFFF,4,text,sizeof(text)) &&
           !strcmp(text,"autoxautox4"));
    assert(!nd_scrmode_text(800,600,8,text,5));
    b[n-4]=0;b[n-3]=0;assert(!nd_scrmode_parse(b,n,&p));      /* depth 0 */
    assert(!nd_scrmode_parse(b,30,&p));
    memcpy(b+12+14,"SCRX",4);assert(!nd_scrmode_parse(b,n,&p));
}
int main(void)
{
    test_ahi();
    test_byterun1();
    test_shot();
    test_scrmode();
    printf("Mixer AHI prefs, screenshot ILBM writer (ByteRun1 round trip) and ScreenMode presets passed.\n");
    return 0;
}
