/* Host tests for include/nd_input_policy.h: pointer-event staleness, music
 * action folding, deferred button replay, orphaned presses, layer overlap,
 * desktop snapshot sizing and the live-reload gate. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_input_policy.h"
#include "nd_hover_dirty.h"

/* Values from include/nd_protocol.h (checked by run_host_tests.sh). */
#define PREVIOUS 8U
#define PLAY_PAUSE 16U
#define NEXT 32U
#define VOLUME 64U

static void pointer_staleness(void)
{
    /* Messages generated at or before the epoch use the old geometry. */
    assert(nd_pointer_event_stale(10,500,10,500));
    assert(nd_pointer_event_stale(10,499,10,500));
    assert(nd_pointer_event_stale(9,999999,10,0));
    assert(!nd_pointer_event_stale(10,501,10,500));
    assert(!nd_pointer_event_stale(11,0,10,999999));
    /* Synthetic messages without a timestamp fall back to the live pointer
     * (the native stale-activation fixture posts zeroed messages). */
    assert(nd_pointer_event_stale(0,0,0,0));
    assert(nd_pointer_event_stale(0,0,5,0));
    /* A late click after activation keeps its own position: processing
     * delay alone must never move the click to the current pointer. */
    assert(!nd_pointer_event_stale(20,1,20,0));
    /* The epoch advances; a late message never moves it back, however late;
     * a clock that was set backwards (now before the epoch) resets it. */
    assert(nd_pointer_epoch_update(10,0,10,1,10,2));
    assert(!nd_pointer_epoch_update(10,500,10,500,11,0));
    assert(!nd_pointer_epoch_update(10,500,9,0,11,0));
    assert(!nd_pointer_epoch_update(100,0,30,0,101,0));     /* 70 s late, clock fine */
    assert(nd_pointer_epoch_update(800000000UL,0,3600,0,3601,0));  /* clock set back */
    assert(nd_pointer_epoch_update(100,500,99,0,100,499));
}

static void media_folding(void)
{
    unsigned int m=0;
    /* Next followed by Play/Pause: both reach the plugin. */
    m=nd_media_fold(m,NEXT,PLAY_PAUSE);
    m=nd_media_fold(m,PLAY_PAUSE,PLAY_PAUSE);
    assert(m==(NEXT|PLAY_PAUSE));
    /* A double click on Play/Pause cancels out, as do the tile toggles. */
    m=nd_media_fold(m,PLAY_PAUSE,PLAY_PAUSE);
    assert(m==NEXT);
    /* Three clicks toggle once. */
    m=0;
    m=nd_media_fold(m,PLAY_PAUSE,PLAY_PAUSE);
    m=nd_media_fold(m,PLAY_PAUSE,PLAY_PAUSE);
    m=nd_media_fold(m,PLAY_PAUSE,PLAY_PAUSE);
    assert(m==PLAY_PAUSE);
    /* Volume drags and track buttons accumulate without loss. */
    m=nd_media_fold(PREVIOUS,VOLUME,PLAY_PAUSE);
    m=nd_media_fold(m,VOLUME,PLAY_PAUSE);
    assert(m==(PREVIOUS|VOLUME));
    /* Twin tiles: toggles from two views cancel, other bits union. */
    assert(nd_media_merge(PLAY_PAUSE|NEXT,PLAY_PAUSE,PLAY_PAUSE)==NEXT);
    assert(nd_media_merge(PLAY_PAUSE,VOLUME,PLAY_PAUSE)==(PLAY_PAUSE|VOLUME));
    assert(nd_media_merge(0,0,PLAY_PAUSE)==0);
}

static void deferred_buttons(void)
{
    struct NDDeferredButtons q;
    int i;
    nd_deferred_clear(&q);
    assert(nd_deferred_add(&q,ND_DEFERRED_DOWN,10,20,0));
    assert(nd_deferred_add(&q,ND_DEFERRED_UP,11,21,0));
    assert(q.count==2 && q.event[0].code==ND_DEFERRED_DOWN && q.event[1].x==11);
    /* Fill it: further presses are ignored, but a release is always kept,
     * so a replayed queue can never end with the dock pressed. */
    assert(nd_deferred_add(&q,ND_DEFERRED_DOWN,30,30,0));
    assert(nd_deferred_add(&q,ND_DEFERRED_UP,31,31,0));
    assert(q.count==ND_DEFERRED_MAX);
    assert(!nd_deferred_add(&q,ND_DEFERRED_DOWN,40,40,0));
    assert(nd_deferred_add(&q,ND_DEFERRED_UP,41,41,0));
    assert(q.count==ND_DEFERRED_MAX && q.event[q.count-1].code==ND_DEFERRED_UP &&
           q.event[q.count-1].x==41);
    /* Queue ending in DOWN, full: a release replaces that dangling press. */
    nd_deferred_clear(&q);
    for(i=0;i<ND_DEFERRED_MAX-1;++i)
        assert(nd_deferred_add(&q,i&1 ? ND_DEFERRED_UP:ND_DEFERRED_DOWN,i,i,0));
    assert(nd_deferred_add(&q,ND_DEFERRED_DOWN,90,90,0));
    assert(q.count==ND_DEFERRED_MAX);
    assert(nd_deferred_add(&q,ND_DEFERRED_UP,91,91,0));
    assert(q.event[q.count-1].code==ND_DEFERRED_UP);
    {int downs=0,ups=0;for(i=0;i<q.count;++i){if(q.event[i].code==ND_DEFERRED_DOWN)++downs;else ++ups;}
     assert(ups>=downs);}
}

static void orphaned_press(void)
{
    int ticks=0;
    /* Held button, queued input or no press: never cancelled. */
    assert(!nd_press_orphaned(1,1,0,&ticks,3) && ticks==0);
    assert(!nd_press_orphaned(1,0,1,&ticks,3) && ticks==0);
    assert(!nd_press_orphaned(0,0,0,&ticks,3) && ticks==0);
    /* Released with nothing queued: cancelled on the third tick only. */
    assert(!nd_press_orphaned(1,0,0,&ticks,3));
    assert(!nd_press_orphaned(1,0,0,&ticks,3));
    assert(nd_press_orphaned(1,0,0,&ticks,3) && ticks==0);
    /* A release arriving in between resets the count. */
    assert(!nd_press_orphaned(1,0,0,&ticks,3));
    assert(!nd_press_orphaned(1,0,1,&ticks,3) && ticks==0);
}

static void overlap(void)
{
    /* Inclusive layer bounds: touching edges overlap, a one-pixel gap does not. */
    assert(nd_rect_overlap(0,0,99,49,99,49,200,200));
    assert(!nd_rect_overlap(0,0,99,49,100,0,200,49));
    assert(!nd_rect_overlap(0,0,99,49,0,50,99,99));
    assert(nd_rect_overlap(10,10,20,20,0,0,639,255));
}

static void snapshot_and_live_reload(void)
{
    assert(nd_snapshot_needs_alloc(0,0,0,640,80));
    assert(!nd_snapshot_needs_alloc(1,640,80,640,80));
    assert(nd_snapshot_needs_alloc(1,600,80,640,80));
    assert(nd_snapshot_needs_alloc(1,640,80,640,96));
    /* The reported overflow: theme artwork on, theme without a backdrop
     * image (snapshot only), dock widened by a live change. */
    assert(!nd_live_reload_allowed(1,0,1,0,0,0));
    /* Same rectangle: live update is fine. */
    assert(nd_live_reload_allowed(1,0,1,0,0,1));
    /* Nothing size-dependent cached: a resize can stay live. */
    assert(nd_live_reload_allowed(1,0,0,0,0,0));
    /* Backdrop or transparency alone still require the same rectangle. */
    assert(!nd_live_reload_allowed(1,0,0,1,0,0));
    assert(!nd_live_reload_allowed(1,0,0,0,40,0));
    /* Magnification headroom or incompatible settings never reload live. */
    assert(!nd_live_reload_allowed(1,64,0,0,0,1));
    assert(!nd_live_reload_allowed(0,0,0,0,0,1));
}

static void hover_dirty_span(void)
{
    struct NDRect pbox[3],cbox[3],pext[3],cext[3],shelf={10,0,300,60},shelf2;
    unsigned char changed[3]={0,0,0},visible[3]={1,1,1},draw[3];
    int i,lo,hi;
    for(i=0;i<3;++i){
        pbox[i].left=36+i*80;pbox[i].top=10;pbox[i].width=pbox[i].height=48;cbox[i]=pbox[i];
        /* the label and dot span the 80-pixel tile, wider than the artwork */
        pext[i].left=20+i*80;pext[i].top=8;pext[i].width=80;pext[i].height=70;cext[i]=pext[i];
    }
    /* Nothing changed: nothing to recompose. */
    assert(!nd_hover_dirty_span(pbox,cbox,pext,cext,changed,visible,3,0,&shelf,&shelf,400,&lo,&hi));
    /* A moved tile: the union of its old and new decoration extent, not
     * just the artwork, so no label or dot is left behind. */
    cbox[1].left=126;cext[1].left=110;
    assert(nd_hover_dirty_span(pbox,cbox,pext,cext,changed,visible,3,0,&shelf,&shelf,400,&lo,&hi) && lo==100 && hi==190);
    assert(lo<pbox[1].left && hi>cbox[1].left+cbox[1].width);
    cbox[1]=pbox[1];cext[1]=pext[1];
    /* Growing artwork with an unchanged extent along the axis still counts. */
    cbox[0].height=60;
    assert(nd_hover_dirty_span(pbox,cbox,pext,cext,changed,visible,3,0,&shelf,&shelf,400,&lo,&hi) && lo==20 && hi==100);
    cbox[0]=pbox[0];
    /* A repainted tile counts in place; vertical docks use top/height. */
    changed[2]=1;
    assert(nd_hover_dirty_span(pbox,cbox,pext,cext,changed,visible,3,1,&shelf,&shelf,400,&lo,&hi) && lo==8 && hi==78);
    changed[2]=0;
    /* A changed shelf outline joins the span; the span is clipped to the dock. */
    shelf2=shelf;shelf2.left=-5;shelf2.width=500;
    assert(nd_hover_dirty_span(pbox,cbox,pext,cext,changed,visible,3,0,&shelf,&shelf2,400,&lo,&hi) && lo==0 && hi==400);
    /* Tiles are skipped only when their extent misses the span entirely. */
    assert(nd_span_touches(&pext[1],0,150,160) && !nd_span_touches(&pext[0],0,100,190));
    assert(nd_span_touches(&pext[0],0,99,190) && !nd_span_touches(&pext[2],0,100,180));
    /* A tile scrolled out of view never widens the span. */
    cext[2].left=300;visible[2]=0;
    assert(!nd_hover_dirty_span(pbox,cbox,pext,cext,changed,visible,3,0,&shelf,&shelf,400,&lo,&hi));
    cext[2]=pext[2];visible[2]=1;
    /* Neighbours whose margins merely touch (gap 0) are not pulled in. */
    for(i=0;i<3;++i){pext[i].left=i*76-2;pext[i].width=80;}
    assert(nd_hover_draw_set(pext,visible,3,0,80,90,-1,draw)==1 && draw[1] && !draw[0] && !draw[2]);
    /* A real overlap pulls in only tiles drawn later: the hovered tile 0 is
     * drawn last, tile 2 after tile 1. */
    pext[1].width=120;
    assert(nd_hover_draw_set(pext,visible,3,0,80,90,-1,draw)==2 && draw[1] && draw[2] && !draw[0]);
    /* Only tile 2 reaches the span: tile 1 (overlapping it) is drawn before
     * it and stays, unless tile 1 is hovered and so drawn after it. */
    assert(nd_hover_draw_set(pext,visible,3,0,200,210,-1,draw)==1 && draw[2]);
    assert(nd_hover_draw_set(pext,visible,3,0,200,210,1,draw)==2 && draw[1] && draw[2] && !draw[0]);
    /* Hidden tiles are never drawn. */
    visible[1]=0;assert(nd_hover_draw_set(pext,visible,3,0,80,90,-1,draw)==0);
}

static void autohide_timer(void)
{
    /* Disabled or invalid: never forces the fast timer. */
    assert(!nd_autohide_needs_fast_timer(0,100,2,5));
    assert(!nd_autohide_needs_fast_timer(1,100,0,5));
    /* Two-second delay (10 ticks): slow until the last idle quantum. */
    assert(!nd_autohide_needs_fast_timer(1,0,2,5));
    assert(!nd_autohide_needs_fast_timer(1,4,2,5));
    assert(nd_autohide_needs_fast_timer(1,5,2,5));
    assert(nd_autohide_needs_fast_timer(1,9,2,5));
    /* One-second delay: fast from the start, as before. */
    assert(nd_autohide_needs_fast_timer(1,0,1,5));
}

static void rexx_tracking(void)
{
    struct NDRexxTrack t[ND_REXX_TRACK_MAX];
    int a,b,i;
    static char m[ND_REXX_TRACK_MAX+2];
    char longa[100],longb[100];
    memset(t,0,sizeof(t));
    /* Two requests per port; a third to the same port waits. */
    a=nd_rexx_admit(t,"HUNG",100);assert(a>=0);nd_rexx_track(t,a,&m[0],"HUNG",100);
    a=nd_rexx_admit(t,"HUNG",100);assert(a>=0);nd_rexx_track(t,a,&m[1],"HUNG",100);
    assert(nd_rexx_admit(t,"HUNG",101)<0);
    /* A hung port never blocks another target. */
    b=nd_rexx_admit(t,"OTHER",101);assert(b>=0);nd_rexx_track(t,b,&m[2],"OTHER",101);
    assert(nd_rexx_admit(t,"HUNGRY",101)>=0);
    /* A reply frees its slot. */
    nd_rexx_release(t,&m[0]);
    assert(nd_rexx_admit(t,"HUNG",102)>=0);
    nd_rexx_release(t,&m[0]);  /* late or duplicate release is harmless */
    /* Unanswered requests expire after ND_REXX_EXPIRE seconds. */
    memset(t,0,sizeof(t));
    a=nd_rexx_admit(t,"HUNG",102);nd_rexx_track(t,a,&m[3],"HUNG",102);
    a=nd_rexx_admit(t,"HUNG",102);nd_rexx_track(t,a,&m[4],"HUNG",102);
    /* Only unexpired requests are worth waiting for at shutdown. */
    assert(nd_rexx_live(t,102)==2 && nd_rexx_live(t,102+ND_REXX_EXPIRE-1)==2);
    assert(nd_rexx_live(t,102+ND_REXX_EXPIRE)==0 && nd_rexx_live(t,50)==0);
    assert(nd_rexx_admit(t,"HUNG",102+ND_REXX_EXPIRE-1)<0);
    assert(nd_rexx_admit(t,"HUNG",102+ND_REXX_EXPIRE)>=0);
    /* A clock that moves backwards expires everything. */
    memset(t,0,sizeof(t));
    a=nd_rexx_admit(t,"P",500);nd_rexx_track(t,a,&m[0],"P",500);
    a=nd_rexx_admit(t,"P",500);nd_rexx_track(t,a,&m[1],"P",500);
    assert(nd_rexx_admit(t,"P",10)>=0);
    /* A full table refuses new ports too. */
    memset(t,0,sizeof(t));
    for(i=0;i<ND_REXX_TRACK_MAX;++i){char port[8];sprintf(port,"P%d",i);
        a=nd_rexx_admit(t,port,1);assert(a>=0);nd_rexx_track(t,a,&m[i],port,1);}
    assert(nd_rexx_admit(t,"NEW",1)<0);
    /* Long names: the stored key is truncated, never overflowed. */
    memset(t,0,sizeof(t));memset(longa,'A',99);longa[99]=0;strcpy(longb,longa);longb[98]='B';
    a=nd_rexx_admit(t,longa,1);nd_rexx_track(t,a,&m[0],longa,1);
    assert(strlen(t[a].port)==ND_REXX_PORT_KEY-1);
    assert(nd_rexx_same_port(t[a].port,longb));
    assert(!nd_rexx_same_port(t[a].port,"AAA"));
}

int main(void)
{
    pointer_staleness();
    media_folding();
    deferred_buttons();
    orphaned_press();
    overlap();
    snapshot_and_live_reload();
    rexx_tracking();
    autohide_timer();
    hover_dirty_span();
    puts("Input policy: stale pointer events, music action folding, deferred buttons, orphaned presses, layer overlap, snapshot size, live-reload gate, per-port ARexx limits, the auto-hide timer rate and hover dirty spans passed.");
    return 0;
}
