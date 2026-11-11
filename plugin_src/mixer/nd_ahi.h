#ifndef ND_AHI_H
#define ND_AHI_H
/* The few ahi.device definitions the Mixer needs, so building NovaDock does
 * not require the AHI developer archive.  Values are those of AHI 6's
 * <devices/ahi.h>; function offsets follow ahi_lib.sfd (bias 42):
 * AHI_AllocAudioA -42 (a1), AHI_FreeAudio -48 (a2),
 * AHI_GetAudioAttrsA -108 (d0,a2,a1). */
#include <exec/types.h>
#include <exec/io.h>
#include <utility/tagitem.h>

#define ND_AHI_NAME "ahi.device"
#define ND_AHI_NO_UNIT 255U
#define ND_AHI_TAGBASE TAG_USER
#define ND_AHIA_AudioID (ND_AHI_TAGBASE+1)
#define ND_AHIA_MixFreq (ND_AHI_TAGBASE+2)
#define ND_AHIA_Channels (ND_AHI_TAGBASE+3)
#define ND_AHIA_Sounds (ND_AHI_TAGBASE+4)
#define ND_AHIDB_MinOutputVolume (ND_AHI_TAGBASE+134)
#define ND_AHIDB_MaxOutputVolume (ND_AHI_TAGBASE+135)
#define ND_AHI_DEFAULT_ID 0x00000000UL
#define ND_AHI_INVALID_ID (~0UL)
#define ND_AHI_DEFAULT_FREQ 0UL

struct NDAHIAudioCtrl { APTR ahiac_UserData; };
/* struct AHIRequest (pack 2, as the 68k ABI lays it out). */
struct NDAHIRequest {
    struct IOStdReq ahir_Std;
    UWORD ahir_Version;
    UWORD ahir_Pad1;
    ULONG ahir_Private[2];
    ULONG ahir_Type;
    ULONG ahir_Frequency;
    LONG ahir_Volume;
    LONG ahir_Position;
    struct NDAHIRequest *ahir_Link;
};

extern struct Library *AHIBase;

#if defined(__VBCC__)
struct NDAHIAudioCtrl *__nd_AHI_AllocAudioA(__reg("a6") struct Library *,
    __reg("a1") struct TagItem *) = "\tjsr\t-42(a6)";
void __nd_AHI_FreeAudio(__reg("a6") struct Library *,
    __reg("a2") struct NDAHIAudioCtrl *) = "\tjsr\t-48(a6)";
BOOL __nd_AHI_GetAudioAttrsA(__reg("a6") struct Library *, __reg("d0") ULONG,
    __reg("a2") struct NDAHIAudioCtrl *, __reg("a1") struct TagItem *) =
    "\tjsr\t-108(a6)";
#define ND_AHI_AllocAudioA(tags) __nd_AHI_AllocAudioA(AHIBase,(tags))
#define ND_AHI_FreeAudio(ctrl) __nd_AHI_FreeAudio(AHIBase,(ctrl))
#define ND_AHI_GetAudioAttrsA(id,ctrl,tags) \
    __nd_AHI_GetAudioAttrsA(AHIBase,(id),(ctrl),(tags))
#elif defined(__GNUC__) && defined(__mc68000__) && !defined(__linux__)
static __inline struct NDAHIAudioCtrl *ND_AHI_AllocAudioA(struct TagItem *tags)
{
    register struct NDAHIAudioCtrl *result __asm("d0");
    register struct Library *base __asm("a6")=AHIBase;
    register struct TagItem *a1 __asm("a1")=tags;
    __asm volatile ("jsr -42(a6)" : "=r"(result) : "r"(base),"r"(a1)
                    : "d1","a0","cc","memory");
    return result;
}
static __inline void ND_AHI_FreeAudio(struct NDAHIAudioCtrl *ctrl)
{
    register struct Library *base __asm("a6")=AHIBase;
    register struct NDAHIAudioCtrl *a2 __asm("a2")=ctrl;
    __asm volatile ("jsr -48(a6)" : : "r"(base),"r"(a2)
                    : "d0","d1","a0","a1","cc","memory");
}
static __inline BOOL ND_AHI_GetAudioAttrsA(ULONG id,struct NDAHIAudioCtrl *ctrl,
                                           struct TagItem *tags)
{
    register LONG result __asm("d0")=(LONG)id;
    register struct Library *base __asm("a6")=AHIBase;
    register struct NDAHIAudioCtrl *a2 __asm("a2")=ctrl;
    register struct TagItem *a1 __asm("a1")=tags;
    __asm volatile ("jsr -108(a6)" : "+r"(result) : "r"(base),"r"(a2),"r"(a1)
                    : "d1","a0","cc","memory");
    return (BOOL)result;
}
#else
/* Host syntax checks only. */
struct NDAHIAudioCtrl *ND_AHI_AllocAudioA(struct TagItem *tags);
void ND_AHI_FreeAudio(struct NDAHIAudioCtrl *ctrl);
BOOL ND_AHI_GetAudioAttrsA(ULONG id,struct NDAHIAudioCtrl *ctrl,
                           struct TagItem *tags);
#endif
#endif
