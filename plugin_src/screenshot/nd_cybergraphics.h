#ifndef ND_CYBERGRAPHICS_H
#define ND_CYBERGRAPHICS_H
/* Small CyberGraphX-compatible interface, also supplied by Picasso96.
 * GetCyberMapAttr -96 (a0,d0); ReadPixelArray -120
 * (a0,d0,d1,d2,a1,d3,d4,d5,d6,d7). No separate RTG SDK is required. */
#include <exec/types.h>
#include <graphics/rastport.h>
extern struct Library *CyberGfxBase;
#if defined(__VBCC__)
ULONG __nd_cgx_attr(__reg("a6") struct Library *,__reg("a0") struct BitMap *,__reg("d0") ULONG)="\tjsr\t-96(a6)";
ULONG __nd_cgx_read(__reg("a6") struct Library *,__reg("a0") APTR,
    __reg("d0") UWORD,__reg("d1") UWORD,__reg("d2") UWORD,
    __reg("a1") struct RastPort *,__reg("d3") UWORD,__reg("d4") UWORD,
    __reg("d5") UWORD,__reg("d6") UWORD,__reg("d7") UBYTE)="\tjsr\t-120(a6)";
#define nd_cgx_bitmap(bitmap) __nd_cgx_attr(CyberGfxBase,(bitmap),0x80000008UL)
#define nd_cgx_rgb_line(out,width,rp,y) __nd_cgx_read(CyberGfxBase,(out),0,0,(UWORD)((width)*3U),(rp),0,(UWORD)(y),(UWORD)(width),1,0)
#elif defined(__GNUC__) && defined(__mc68000__) && !defined(__linux__)
static __inline ULONG nd_cgx_bitmap(struct BitMap *bitmap)
{
    register struct Library *base __asm("a6")=CyberGfxBase;
    register struct BitMap *a0 __asm("a0")=bitmap;
    register ULONG result __asm("d0")=0x80000008UL;
    __asm volatile("jsr -96(a6)":"+r"(result),"+r"(a0):"r"(base):"d1","a1","cc","memory");return result;
}
static __inline ULONG nd_cgx_rgb_line(APTR out,unsigned int width,struct RastPort *rp,unsigned int y)
{
    register struct Library *base __asm("a6")=CyberGfxBase;
    register APTR a0 __asm("a0")=out;register struct RastPort *a1 __asm("a1")=rp;
    register ULONG d0 __asm("d0")=0,d1 __asm("d1")=0,d2 __asm("d2")=width*3U;
    register ULONG d3 __asm("d3")=0,d4 __asm("d4")=y,d5 __asm("d5")=width;
    register ULONG d6 __asm("d6")=1,d7 __asm("d7")=0;
    __asm volatile("jsr -120(a6)":"+r"(d0),"+r"(d1),"+r"(d2),"+r"(d3),"+r"(d4),"+r"(d5),"+r"(d6),"+r"(d7),"+r"(a0),"+r"(a1):"r"(base):"cc","memory");return d0;
}
#else
ULONG nd_cgx_bitmap(struct BitMap *bitmap);
ULONG nd_cgx_rgb_line(APTR out,unsigned int width,struct RastPort *rp,unsigned int y);
#endif
#endif
