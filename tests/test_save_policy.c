#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_save.h"
#include "nd_indicator.h"
/* Quitting: a save that keeps failing refuses one quit, then lets go. */
static void test_quit(void)
{
    struct NDSaveState s;
    memset(&s,0,sizeof(s));
    assert(nd_save_quit_step(&s,0,0)==ND_SAVE_QUIT_LEAVE);       /* nothing unsaved */
    nd_save_changed(&s);nd_save_begin(&s);
    assert(nd_save_quit_step(&s,10,0)==ND_SAVE_QUIT_WAIT);
    assert(nd_save_quit_step(&s,ND_SAVE_QUIT_LIMIT+1,0)==ND_SAVE_QUIT_REFUSE);
    assert(nd_save_quit_step(&s,ND_SAVE_QUIT_SECOND_LIMIT,1)==ND_SAVE_QUIT_WAIT);
    assert(nd_save_quit_step(&s,ND_SAVE_QUIT_SECOND_LIMIT+1,1)==ND_SAVE_QUIT_LEAVE);
    nd_save_complete(&s,0);                                     /* read-only ENVARC: */
    assert(nd_save_quit_step(&s,0,0)==ND_SAVE_QUIT_REFUSE);
    assert(nd_save_quit_step(&s,0,1)==ND_SAVE_QUIT_LEAVE);
    nd_save_begin(&s);nd_save_complete(&s,1);
    assert(nd_save_quit_step(&s,0,0)==ND_SAVE_QUIT_LEAVE);
}
int main(void)
{
    test_quit();
    struct NDSaveState s;int i;
    memset(&s,0,sizeof(s));assert(!nd_save_tick(&s,5,0));
    for(i=0;i<20;++i)nd_save_changed(&s);
    assert(s.revision==20 && s.coalesced==19 && s.started==0);
    assert(!nd_save_tick(&s,5,0));assert(!nd_save_tick(&s,5,1));
    assert(!nd_save_tick(&s,5,0));assert(nd_save_tick(&s,5,0));
    nd_save_begin(&s);assert(s.active_revision==20 && !nd_save_tick(&s,10,0));
    nd_save_changed(&s);nd_save_complete(&s,1);
    assert(s.saved==20 && s.revision==21 && s.completed==1);
    assert(!nd_save_tick(&s,5,0));assert(nd_save_tick(&s,5,0));
    nd_save_begin(&s);nd_save_complete(&s,0);
    assert(s.saved==20 && s.last_failed && s.failed==1 && s.quiet==50);
    assert(!nd_save_tick(&s,49,0));assert(nd_save_tick(&s,1,0));
    nd_save_begin(&s);nd_save_complete(&s,1);
    assert(s.saved==s.revision && !s.last_failed && !nd_save_tick(&s,100,0));
    assert(nd_indicator_mode("AMIRC",1,1,0,"")==ND_INDICATOR_NONE);
    assert(nd_indicator_mode("DOPUS4",1,1,0,"")==ND_INDICATOR_NONE);
    assert(nd_indicator_mode("ROADSHOW",1,1,0,"")==ND_INDICATOR_NONE);
    assert(nd_indicator_mode("NET",1,1,0,"")==ND_INDICATOR_NONE);
    assert(nd_indicator_mode("THIRDPARTY",1,1,0,"")==ND_INDICATOR_NONE);
    assert(nd_indicator_mode("AMIRC",1,1,0,"*")==ND_INDICATOR_DOT);
    assert(nd_indicator_mode("DOPUS4",1,0,0,"*")==ND_INDICATOR_NONE);
    assert(nd_indicator_mode("ROADSHOW",0,1,0,"*")==ND_INDICATOR_NONE);
    assert(nd_indicator_mode("MUSIC",1,1,7,"")==ND_INDICATOR_DOT);
    assert(nd_indicator_mode("MUSIC",1,1,0,"")==ND_INDICATOR_NONE);
    assert(nd_indicator_mode("TRASH",1,0,0,"3")==ND_INDICATOR_BADGE);
    assert(nd_indicator_mode("HELLO",1,1,0,"*")==ND_INDICATOR_BADGE);
    puts("Idle save batching, revisions, retry safety and application indicators passed.");
    return 0;
}
