/* Portable AHI preferences helpers (see nd_ahiprefs.h).  No AmigaOS
 * headers, so the host tests exercise exactly this code. */
#include "nd_ahiprefs.h"
#include <string.h>

static unsigned long be32(const unsigned char *p)
{
    return ((unsigned long)p[0]<<24)|((unsigned long)p[1]<<16)|
           ((unsigned long)p[2]<<8)|(unsigned long)p[3];
}
static void put_be32(unsigned char *p,unsigned long v)
{
    p[0]=(unsigned char)(v>>24);p[1]=(unsigned char)(v>>16);
    p[2]=(unsigned char)(v>>8);p[3]=(unsigned char)v;
}
/* struct AHIUnitPrefs: Unit(1) Obsolete(1) Channels(2) AudioMode(4)
 * Frequency(4) MonitorVolume(4) InputGain(4) OutputVolume(4) Input(4)
 * Output(4) = 32 bytes; OutputVolume is at offset 20. */
#define AHIU_MIN 24UL
#define AHIU_VOLUME 20UL
/* Calls VISIT for each AHIU chunk.  Returns how many were visited. */
static int walk(const unsigned char *b,unsigned long n,
                void (*visit)(const unsigned char *chunk,void *ctx),void *ctx)
{
    unsigned long pos=12,form,size;
    int count=0;
    if (!b || n<12 || n>ND_AHI_PREFS_MAX || memcmp(b,"FORM",4) ||
        memcmp(b+8,"PREF",4)) return 0;
    form=be32(b+4);
    if (form<4 || form>n-8) return 0;
    n=form+8;
    while (pos+8<=n) {
        size=be32(b+pos+4);
        if (size>n-pos-8) return count;
        if (!memcmp(b+pos,"AHIU",4) && size>=AHIU_MIN) {
            ++count;
            visit(b+pos+8,ctx);
        }
        pos+=8+size+(size&1UL);
    }
    return count;
}
struct Pick { unsigned long volume; int quality; };
static void pick(const unsigned char *c,void *ctx)
{
    struct Pick *p=(struct Pick *)ctx;
    /* Low-level unit first (what players use), then unit 0, then any. */
    int quality=c[0]==ND_AHI_UNIT_LOWLEVEL ? 3 : c[0]==0 ? 2 : 1;
    if (quality>p->quality) {p->quality=quality;p->volume=be32(c+AHIU_VOLUME);}
}
int nd_ahi_prefs_volume(const unsigned char *buffer,unsigned long length,
                        unsigned long *fixed)
{
    struct Pick p;
    p.volume=0;p.quality=0;
    if (!walk(buffer,length,pick,&p) || !p.quality) return 0;
    if (fixed) *fixed=p.volume;
    return 1;
}
static void store(const unsigned char *c,void *ctx)
{
    put_be32((unsigned char *)c+AHIU_VOLUME,*(unsigned long *)ctx);
}
int nd_ahi_prefs_set_volume(unsigned char *buffer,unsigned long length,
                            unsigned long fixed)
{
    return walk(buffer,length,store,&fixed);
}
unsigned int nd_ahi_percent(unsigned long fixed)
{
    if (fixed>=ND_AHI_FIXED_ONE) return 100;
    return (unsigned int)((fixed*100UL+ND_AHI_FIXED_ONE/2UL)/ND_AHI_FIXED_ONE);
}
unsigned long nd_ahi_fixed(unsigned int percent)
{
    if (percent>=100) return ND_AHI_FIXED_ONE;
    return ((unsigned long)percent*ND_AHI_FIXED_ONE+50UL)/100UL;
}
