#ifndef ND_SAVE_H
#define ND_SAVE_H
#define ND_SAVE_IDLE_TICKS 10 /* five housekeeping ticks per second: 2s */
#define ND_SAVE_RETRY_TICKS 50
struct NDSaveState {
    unsigned long revision,saved,active_revision;
    unsigned long started,completed,failed,coalesced;
    int quiet,active,last_failed;
};
void nd_save_changed(struct NDSaveState *s);
int nd_save_tick(struct NDSaveState *s,int ticks,int interacting);
void nd_save_begin(struct NDSaveState *s);
void nd_save_complete(struct NDSaveState *s,int success);
/* Quitting with unsaved settings, WAITED ticks (1/50 s) into the flush.
 * The first quit that cannot save is refused so the user sees why; the
 * next one leaves without the change, so a read-only ENVARC: or a missing
 * SaveConfig can never trap the dock. */
#define ND_SAVE_QUIT_WAIT 0    /* a save is running: keep waiting */
#define ND_SAVE_QUIT_LEAVE 1   /* saved, or quitting a second time */
#define ND_SAVE_QUIT_REFUSE 2  /* stay running and say why */
#define ND_SAVE_QUIT_LIMIT 1000UL        /* first quit: up to 20 s */
#define ND_SAVE_QUIT_SECOND_LIMIT 100UL  /* after a refusal: 2 s */
int nd_save_quit_step(const struct NDSaveState *s,unsigned long waited,
                      int refused_before);
#if defined(__amigaos__) || defined(AMIGA) || defined(__VBCC__)
#include <exec/types.h>
#include <exec/ports.h>
struct NDConfig;
#define ND_SAVE_MAGIC 0x4e445357UL
#define ND_SAVE_REQUEST 1
#define ND_SAVE_COMPLETE 2
/* Current SaveConfig: reads the dock's snapshot in place and never frees
 * it.  ND_SAVE_REQUEST (older SaveConfig) receives a copy it frees. */
#define ND_SAVE_BORROW 3
struct NDSaveMessage {
    struct Message msg;
    ULONG magic,revision;
    UWORD kind,env_ok,envarc_ok;
    ULONG config_size,elapsed,env_ticks,envarc_ticks;
    struct NDConfig *config;
};
#endif
#endif
