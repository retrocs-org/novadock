#ifndef ND_CSWARP_PROBE_H
#define ND_CSWARP_PROBE_H
#include <exec/types.h>
#include <exec/libraries.h>
/* Provisional name: observed WarpTool call at text+0x80b0, library table
 * text+0x51c8 -> text+0x0808. A0=428-byte result, low D0=success. */
#if defined(__VBCC__)
UBYTE nd_cswarp_probe_diag(__reg("a6") struct Library *,__reg("a0") APTR)="\tjsr\t-42(a6)";
#elif defined(__GNUC__) && defined(__mc68000__) && !defined(__linux__)
static __inline UBYTE nd_cswarp_probe_diag(struct Library *library,APTR buffer)
{
 register struct Library *base __asm("a6")=library;
 register APTR a0 __asm("a0")=buffer;
 register ULONG d0 __asm("d0");
 __asm volatile("jsr -42(a6)":"=r"(d0),"+r"(a0):"r"(base):"d1","a1","cc","memory");
 return (UBYTE)d0;
}
#else
#error This experimental probe requires a supported classic Amiga compiler.
#endif
#endif
