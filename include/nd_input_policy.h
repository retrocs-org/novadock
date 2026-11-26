#ifndef ND_INPUT_POLICY_H
#define ND_INPUT_POLICY_H
/* Portable input policies shared by the dock and the host tests.
 * No AmigaOS headers: every rule here is exercised on the host. */
#if defined(__GNUC__)
#define ND_POLICY static __attribute__((unused))
#else
#define ND_POLICY static
#endif

/* Intuition stamps each pointer message with the time it was generated and
 * coordinates relative to the window at that moment.  Those coordinates are
 * only wrong for messages generated before the latest activation, window-box
 * change or layer scroll (the "epoch").  Only such messages are resolved at
 * the live pointer; every later message keeps the position where the user
 * actually clicked, even when the dock processes it late. */
ND_POLICY int nd_pointer_event_stale(unsigned long secs,unsigned long micros,
                                  unsigned long epoch_secs,unsigned long epoch_micros)
{
    return secs<epoch_secs || (secs==epoch_secs && micros<=epoch_micros);
}

/* The epoch only advances, except when the clock itself moved backwards (a
 * date change, a clock utility resync): if the current time NOW is earlier
 * than the epoch, a new stamp replaces it.  Otherwise every later event would
 * stay "stale" until the clock caught up and each click would be resolved at
 * the live pointer.  A message that is merely processed late never moves the
 * epoch back, however late. */
ND_POLICY int nd_pointer_epoch_update(unsigned long epoch_secs,unsigned long epoch_micros,
                                   unsigned long secs,unsigned long micros,
                                   unsigned long now_secs,unsigned long now_micros)
{
    if (!nd_pointer_event_stale(secs,micros,epoch_secs,epoch_micros)) return 1;
    return now_secs<epoch_secs || (now_secs==epoch_secs && now_micros<epoch_micros);
}

/* Media actions queued for the next plugin exchange.  Separate actions
 * accumulate instead of replacing each other.  Play/Pause is a toggle: a
 * second click before the exchange cancels the first, so the player and the
 * optimistic tile state stay in agreement. */
ND_POLICY unsigned int nd_media_fold(unsigned int pending,unsigned int action,
                                  unsigned int toggle)
{
    if (action==toggle) return pending^toggle;
    return pending|action;
}
/* Fold the actions of a repeated (twin) tile into its logical client. */
ND_POLICY unsigned int nd_media_merge(unsigned int a,unsigned int b,unsigned int toggle)
{
    return ((a|b)&~toggle) | ((a^b)&toggle);
}

/* Button events that arrive while an asynchronous window-box change is
 * pending cannot be hit-tested yet.  They are kept in screen coordinates and
 * replayed, in order, once Intuition acknowledges the new geometry.  A full
 * queue keeps its oldest press but always records the newest release, so a
 * pressed tile can never be left without its release. */
#define ND_DEFERRED_MAX 4
#define ND_DEFERRED_DOWN 1
#define ND_DEFERRED_UP 2
struct NDDeferredButton {int code,x,y;unsigned short qualifier;};
struct NDDeferredButtons {int count;struct NDDeferredButton event[ND_DEFERRED_MAX];};
ND_POLICY void nd_deferred_clear(struct NDDeferredButtons *q)
{
    q->count=0;
}
ND_POLICY int nd_deferred_add(struct NDDeferredButtons *q,int code,int x,int y,
                           unsigned short qualifier)
{
    struct NDDeferredButton *e;
    if (q->count<ND_DEFERRED_MAX) e=&q->event[q->count++];
    else if (code==ND_DEFERRED_UP) {
        /* Replace the newest event; keep the queue ending in a release. */
        e=&q->event[ND_DEFERRED_MAX-1];
        if (e->code==ND_DEFERRED_DOWN && ND_DEFERRED_MAX>1 &&
            q->event[ND_DEFERRED_MAX-2].code==ND_DEFERRED_UP)
            e=&q->event[ND_DEFERRED_MAX-2];
        q->count=(int)(e-q->event)+1;
    } else return 0;
    e->code=code;e->x=x;e->y=y;e->qualifier=qualifier;
    return 1;
}

/* A press whose release was lost (window recreated, message discarded) must
 * not leave the dock "pressed": that blocks auto-hide, snapshot refreshes and
 * idle timing.  Cancel it after STABLE consecutive housekeeping ticks in
 * which the button is up and no release is waiting in the window port. */
ND_POLICY int nd_press_orphaned(int pressed,int button_held,int messages_waiting,
                             int *ticks,int stable)
{
    if (!pressed || button_held || messages_waiting) {*ticks=0;return 0;}
    if (++*ticks<stable) return 0;
    *ticks=0;
    return 1;
}

/* The always-on-top refresh raises the dock only when a foreign layer in
 * front of it really overlaps it.  Inclusive Amiga layer bounds. */
ND_POLICY int nd_rect_overlap(int ax0,int ay0,int ax1,int ay1,
                           int bx0,int by0,int bx1,int by1)
{
    return ax0<=bx1 && bx0<=ax1 && ay0<=by1 && by0<=ay1;
}

/* The desktop snapshot backs the whole dock window. It must be reallocated
 * whenever the dock's size differs from the size it was allocated for. */
ND_POLICY int nd_snapshot_needs_alloc(int exists,int have_width,int have_height,
                                   int width,int height)
{
    return !exists || have_width!=width || have_height!=height;
}

/* A live (in-place) reload keeps the window and every cached bitmap. It is
 * only safe while the dock rectangle is unchanged whenever a cached desktop
 * snapshot, backdrop strip or transparency buffer depends on that size. */
ND_POLICY int nd_live_reload_allowed(int compatible,int hover_headroom,int snapshot,
                                  int backdrop,int transparency,int same_box)
{
    if (!compatible || hover_headroom) return 0;
    if ((snapshot || backdrop || transparency) && !same_box) return 0;
    return 1;
}
/* ARexx launcher actions: at most ND_REXX_PER_PORT unanswered requests per
 * target port, so one hung application never blocks actions aimed at other
 * ports.  A request unanswered for ND_REXX_EXPIRE seconds stops counting
 * (its reply is still collected and freed whenever it arrives).  TIME is a
 * seconds clock; a clock that moves backwards expires every entry. */
#define ND_REXX_TRACK_MAX 8
#define ND_REXX_PER_PORT 2
#define ND_REXX_EXPIRE 10UL
#define ND_REXX_PORT_KEY 64
struct NDRexxTrack {const void *msg;unsigned long sent;char port[ND_REXX_PORT_KEY];};
/* Ports compare on their first ND_REXX_PORT_KEY-1 characters (the key kept). */
ND_POLICY int nd_rexx_same_port(const char *key,const char *port)
{
    int k;
    for (k=0;k<ND_REXX_PORT_KEY-1;++k) {
        if (key[k]!=port[k]) return 0;
        if (!key[k]) return 1;
    }
    return 1;
}
/* Slot for a new request to PORT, or -1 while that port (or the table) is busy. */
ND_POLICY int nd_rexx_admit(struct NDRexxTrack *t,const char *port,unsigned long now)
{
    int i,busy=0,slot=-1;
    for (i=0;i<ND_REXX_TRACK_MAX;++i) {
        if (t[i].msg && (now<t[i].sent || now-t[i].sent>=ND_REXX_EXPIRE)) t[i].msg=0;
        if (!t[i].msg) {if (slot<0) slot=i;continue;}
        if (nd_rexx_same_port(t[i].port,port)) ++busy;
    }
    return busy>=ND_REXX_PER_PORT ? -1:slot;
}
ND_POLICY void nd_rexx_track(struct NDRexxTrack *t,int slot,const void *msg,
                          const char *port,unsigned long now)
{
    int k=0;
    if (slot<0 || slot>=ND_REXX_TRACK_MAX) return;
    while (k<ND_REXX_PORT_KEY-1 && port[k]) {t[slot].port[k]=port[k];++k;}
    t[slot].port[k]=0;t[slot].sent=now;t[slot].msg=msg;
}
/* Requests still worth waiting for at shutdown: tracked and not expired. */
ND_POLICY int nd_rexx_live(const struct NDRexxTrack *t,unsigned long now)
{
    int i,live=0;
    for (i=0;i<ND_REXX_TRACK_MAX;++i)
        if (t[i].msg && now>=t[i].sent && now-t[i].sent<ND_REXX_EXPIRE) ++live;
    return live;
}
/* A reply arrived: forget its entry (it may already have expired). */
ND_POLICY void nd_rexx_release(struct NDRexxTrack *t,const void *msg)
{
    int i;
    for (i=0;i<ND_REXX_TRACK_MAX;++i) if (t[i].msg==msg) t[i].msg=0;
}
/* The dock timer runs at 1 Hz while nothing needs it (ND_TIMER_IDLE_QUANTA
 * five-Hz quanta per wake).  Auto-hide only needs the 5 Hz timer during the
 * final IDLE_QUANTA ticks before its delay (DELAY seconds, 5 ticks each)
 * expires, so the dock still hides on time. */
ND_POLICY int nd_autohide_needs_fast_timer(int enabled,int idle_ticks,int delay_seconds,int idle_quanta)
{
    if (!enabled || delay_seconds<1) return 0;
    return idle_ticks+idle_quanta>=delay_seconds*5;
}
#endif
