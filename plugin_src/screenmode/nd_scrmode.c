/* Portable ScreenMode preferences parser (see nd_scrmode.h). */
#include "nd_scrmode.h"
#include <stdio.h>
#include <string.h>

static unsigned long be32(const unsigned char *p)
{
    return ((unsigned long)p[0]<<24)|((unsigned long)p[1]<<16)|
           ((unsigned long)p[2]<<8)|(unsigned long)p[3];
}
static unsigned int be16(const unsigned char *p)
{
    return ((unsigned int)p[0]<<8)|p[1];
}
/* struct ScreenModePrefs: Reserved[4] (16 bytes), DisplayID (4), Width,
 * Height, Depth, Control (2 each) = 28 bytes. */
int nd_scrmode_parse(const unsigned char *b,unsigned long n,
                     struct NDScreenModePreset *p)
{
    unsigned long pos=12,form,size;
    if (!b || !p || n<12 || memcmp(b,"FORM",4) || memcmp(b+8,"PREF",4))
        return 0;
    form=be32(b+4);
    if (form<4) return 0;
    if (form<n-8) n=form+8;
    while (pos+8<=n) {
        size=be32(b+pos+4);
        if (size>n-pos-8) return 0;
        if (!memcmp(b+pos,"SCRM",4)) {
            const unsigned char *c=b+pos+8;
            if (size<28) return 0;
            p->display_id=be32(c+16);
            p->width=be16(c+20);p->height=be16(c+22);
            p->depth=be16(c+24);p->control=be16(c+26);
            return p->depth>=1 && p->depth<=32;
        }
        pos+=8+size+(size&1UL);
    }
    return 0;
}
int nd_scrmode_text(unsigned int width,unsigned int height,unsigned int depth,
                    char *out,unsigned int size)
{
    char w[8],h[8],text[32];
    if (!out || !size || depth>99) return 0;
    if (width==ND_SCRMODE_DEFAULT || !width) strcpy(w,"auto");
    else sprintf(w,"%u",width%100000U);
    if (height==ND_SCRMODE_DEFAULT || !height) strcpy(h,"auto");
    else sprintf(h,"%u",height%100000U);
    sprintf(text,"%sx%sx%u",w,h,depth);
    if (strlen(text)>=size) return 0;
    strcpy(out,text);
    return 1;
}
