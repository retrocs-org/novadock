#ifndef ND_CSWARP_H
#define ND_CSWARP_H
#include <exec/types.h>
#include <exec/libraries.h>
/* Observed in cswarp.library and WarpTool 30.74; gated to that exact ABI.
 * A0 points to the argument; success is the low byte of D0. */
#if defined(__VBCC__)
UBYTE nd_cswarp_diag(__reg("a6") struct Library *,__reg("a0") APTR)="\tjsr\t-42(a6)";
UBYTE nd_cswarp_ssid(__reg("a6") struct Library *,__reg("a0") APTR)="\tjsr\t-66(a6)";
UBYTE nd_cswarp_password(__reg("a6") struct Library *,__reg("a0") APTR)="\tjsr\t-72(a6)";
UBYTE nd_cswarp_frequency(__reg("a6") struct Library *,__reg("a0") APTR)="\tjsr\t-282(a6)";
#else
#error Warp control currently requires the supported vbcc classic Amiga build.
#endif
#endif
