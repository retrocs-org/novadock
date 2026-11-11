#ifndef ND_SCREENNOTIFY_H
#define ND_SCREENNOTIFY_H
/* Minimal, self-contained interface to the optional third-party
 * screennotify.library (Aminet: util/libs/ScreenNotify.lha), so NovaDock can
 * release the Workbench screen when ScreenMode/Font preferences close it and
 * reopen afterwards.  No header from that package is required to build.
 *
 * Library vector offsets (bias 30):
 *   AddCloseScreenClient -30, RemCloseScreenClient -36,
 *   AddPubScreenClient   -42, RemPubScreenClient   -48,
 *   AddWorkbenchClient   -54, RemWorkbenchClient   -60
 */
#include <exec/types.h>
#include <exec/ports.h>

#define SCREENNOTIFY_NAME "screennotify.library"
#define SCREENNOTIFY_VERSION 1
#define SCREENNOTIFY_TYPE_CLOSESCREEN 0
#define SCREENNOTIFY_TYPE_PUBLICSCREEN 1
#define SCREENNOTIFY_TYPE_PRIVATESCREEN 2
#define SCREENNOTIFY_TYPE_WORKBENCH 3

struct ScreenNotifyMessage {
    struct Message snm_Message;
    ULONG snm_Type;   /* SCREENNOTIFY_TYPE_* */
    APTR snm_Value;   /* WORKBENCH: FALSE = about to close, TRUE = opened */
};

extern struct Library *ScreenNotifyBase;

#if defined(__VBCC__)
APTR __nd_AddWorkbenchClient(__reg("a6") struct Library *,
                             __reg("a0") struct MsgPort *,
                             __reg("d0") LONG) = "\tjsr\t-54(a6)";
BOOL __nd_RemWorkbenchClient(__reg("a6") struct Library *,
                             __reg("a0") APTR) = "\tjsr\t-60(a6)";
#define ND_AddWorkbenchClient(port,pri) \
    __nd_AddWorkbenchClient(ScreenNotifyBase,(port),(pri))
#define ND_RemWorkbenchClient(handle) \
    __nd_RemWorkbenchClient(ScreenNotifyBase,(handle))
#elif defined(__GNUC__) && defined(__mc68000__) && !defined(__linux__)
static __inline APTR ND_AddWorkbenchClient(struct MsgPort *port,LONG pri)
{
    register APTR result __asm("d0");
    register struct Library *base __asm("a6")=ScreenNotifyBase;
    register struct MsgPort *a0 __asm("a0")=port;
    register LONG d0 __asm("d0")=pri;
    __asm volatile ("jsr -54(a6)" : "=r"(result) : "r"(base),"r"(a0),"r"(d0)
                    : "d1","a0","a1","cc","memory");
    return result;
}
static __inline BOOL ND_RemWorkbenchClient(APTR handle)
{
    register LONG result __asm("d0");
    register struct Library *base __asm("a6")=ScreenNotifyBase;
    register APTR a0 __asm("a0")=handle;
    __asm volatile ("jsr -60(a6)" : "=r"(result) : "r"(base),"r"(a0)
                    : "d1","a0","a1","cc","memory");
    return (BOOL)result;
}
#else
/* Host syntax checks only. */
APTR ND_AddWorkbenchClient(struct MsgPort *port,LONG pri);
BOOL ND_RemWorkbenchClient(APTR handle);
#endif

#endif
