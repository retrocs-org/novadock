#include "nd_save.h"
void nd_save_changed(struct NDSaveState *s)
{
    if (s->revision!=s->saved) ++s->coalesced;
    ++s->revision;
    if (!s->revision) ++s->revision;
    s->quiet=ND_SAVE_IDLE_TICKS;
}
int nd_save_tick(struct NDSaveState *s,int ticks,int interacting)
{
    if (s->revision==s->saved || s->active) return 0;
    if (interacting) {s->quiet=ND_SAVE_IDLE_TICKS;return 0;}
    if (s->quiet>0) s->quiet-=ticks;
    return s->quiet<=0;
}
void nd_save_begin(struct NDSaveState *s)
{
    s->active=1;s->active_revision=s->revision;++s->started;
}
void nd_save_complete(struct NDSaveState *s,int success)
{
    s->active=0;s->last_failed=!success;
    if (success) {s->saved=s->active_revision;++s->completed;}
    else {++s->failed;s->quiet=ND_SAVE_RETRY_TICKS;}
}
int nd_save_quit_step(const struct NDSaveState *s,unsigned long waited,
                      int refused_before)
{
    unsigned long limit=refused_before ? ND_SAVE_QUIT_SECOND_LIMIT :
                                         ND_SAVE_QUIT_LIMIT;
    if (s->revision==s->saved) return ND_SAVE_QUIT_LEAVE;
    if (s->active && waited<=limit) return ND_SAVE_QUIT_WAIT;
    return refused_before ? ND_SAVE_QUIT_LEAVE : ND_SAVE_QUIT_REFUSE;
}
