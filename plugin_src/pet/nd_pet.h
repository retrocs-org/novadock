#ifndef ND_PET_H
#define ND_PET_H
/* PET behaviour, portable and host-tested: a companion that idles, falls
 * asleep after a while without clicks, and is happy (or, for Gerald the
 * stapler, chomps) for a few seconds after a click - which also wakes it.
 * Times are in ticks (1/50 s).  Characters: cat, Gerald, dog, rabbit and
 * Bob the rubber ball; nd_pet_step() returns a state 0-15 that the plugin
 * offsets by character*ND_PET_STATES in the artwork bank. */
#include <stdio.h>
#include <string.h>
#include "nd_pet_art.h"
#define ND_PET_MOOD_IDLE 0
#define ND_PET_MOOD_SLEEP 1
#define ND_PET_MOOD_HAPPY 2
#define ND_PET_HAPPY_TICKS 150UL
struct NDPet {
    int mood,frame;
    unsigned long idle_ticks,mood_ticks,frame_ticks;
};
static void nd_pet_init(struct NDPet *p)
{
    p->mood=ND_PET_MOOD_IDLE;p->frame=0;p->idle_ticks=p->mood_ticks=p->frame_ticks=0;
}
/* Ticks per animation frame for the mood (INTERVAL is the idle rate). */
static unsigned long nd_pet_frame_ticks(int mood,unsigned long interval)
{
    if(mood==ND_PET_MOOD_SLEEP)return interval*2UL;
    if(mood==ND_PET_MOOD_HAPPY)return interval>2UL ? interval/2UL:1UL;
    return interval;
}
/* Advance by ELAPSED ticks; CLICKS since the last step.  SLEEP_AFTER ticks
 * without clicks send it to sleep.  Returns the bank state to show. */
static int nd_pet_step(struct NDPet *p,unsigned long elapsed,unsigned int clicks,
                       unsigned long sleep_after,unsigned long interval)
{
    unsigned long per;
    int count;
    if(clicks){
        p->mood=ND_PET_MOOD_HAPPY;p->mood_ticks=0;p->idle_ticks=0;p->frame=0;p->frame_ticks=0;
    }else{
        p->idle_ticks+=elapsed;p->mood_ticks+=elapsed;p->frame_ticks+=elapsed;
        if(p->mood==ND_PET_MOOD_HAPPY && p->mood_ticks>=ND_PET_HAPPY_TICKS){
            p->mood=ND_PET_MOOD_IDLE;p->mood_ticks=0;p->frame=0;p->frame_ticks=0;
        }
        if(p->mood==ND_PET_MOOD_IDLE && p->idle_ticks>=sleep_after){
            p->mood=ND_PET_MOOD_SLEEP;p->mood_ticks=0;p->frame=0;p->frame_ticks=0;
        }
    }
    count=p->mood==ND_PET_MOOD_SLEEP ? ND_PET_SLEEP_FRAMES :
          p->mood==ND_PET_MOOD_HAPPY ? ND_PET_HAPPY_FRAMES : ND_PET_IDLE_FRAMES;
    per=nd_pet_frame_ticks(p->mood,interval);
    if(p->frame_ticks>=per){p->frame=(int)((p->frame+p->frame_ticks/per)%(unsigned long)count);p->frame_ticks%=per;}
    return (p->mood==ND_PET_MOOD_SLEEP ? ND_PET_SLEEP_FIRST :
            p->mood==ND_PET_MOOD_HAPPY ? ND_PET_HAPPY_FIRST : ND_PET_IDLE_FIRST)+p->frame;
}
/* Ticks until the next frame is due (the plugin sleeps that long). */
static unsigned long nd_pet_wait(const struct NDPet *p,unsigned long interval)
{
    unsigned long per=nd_pet_frame_ticks(p->mood,interval);
    return p->frame_ticks<per ? per-p->frame_ticks : 1UL;
}
static const char *nd_pet_mood_text(int mood)
{
    return mood==ND_PET_MOOD_SLEEP ? "Asleep" : mood==ND_PET_MOOD_HAPPY ? "Happy!" : "Purring";
}
/* Settings: CHARACTER choice and the default name for each character. */
static const char *const nd_pet_characters[ND_PET_CHARACTERS]={"Cat","Gerald","Dog","Rabbit","Bob"};
static const char *const nd_pet_default_names[ND_PET_CHARACTERS]={"Whiskers","Gerald","Biscuit","Clover","Bob"};
static int nd_pet_character(const char *choice)
{
    int i;
    for(i=0;choice && i<ND_PET_CHARACTERS;++i)if(!strcmp(choice,nd_pet_characters[i]))return i;
    return ND_PET_CAT;
}
/* The idle mood in each character's own words. */
static const char *nd_pet_idle_text(int who)
{
    static const char *const idle[ND_PET_CHARACTERS]={"Purring","Hmph.","Wagging","Nibbling","Bouncing"};
    return who>=0 && who<ND_PET_CHARACTERS ? idle[who] : idle[0];
}
/* Gerald's unsolicited advice and Bob's chatter, one per click, each short
 * enough for a tile value (under 32 characters). */
#define ND_PET_LINES 10
static const char *const nd_gerald_tips[ND_PET_LINES]={
    "Tip: staples fix everything.","Have you tried more staples?","That's not a bug. Probably.",
    "Need help? I'm decorative.","I bite. Ask the last mouse.","Save often. I won't remind you.",
    "Paper jam? Not my department.","Your RAM looks lovely today.","Ow. That was my good side.",
    "Busy. Doing what? Stapling."};
static const char *const nd_bob_lines[ND_PET_LINES]={
    "Wheee!","I'm on a roll!","Bouncing back!","Again! Again!","I'm having a ball.",
    "Gravity is a suggestion.","Round of applause?","Dots are in this season.",
    "Spin me right round!","Floor? Never heard of it."};
/* The tile's value: name and mood, or a line after a click. */
static void nd_pet_value(char *out,int who,const char *name,int mood,unsigned int line)
{
    if(mood==ND_PET_MOOD_HAPPY && who==ND_PET_GERALD)strcpy(out,nd_gerald_tips[line%ND_PET_LINES]);
    else if(mood==ND_PET_MOOD_HAPPY && who==ND_PET_BOB)strcpy(out,nd_bob_lines[line%ND_PET_LINES]);
    else sprintf(out,"%.20s %s",name,mood==ND_PET_MOOD_IDLE ? nd_pet_idle_text(who):nd_pet_mood_text(mood));
}
#endif
