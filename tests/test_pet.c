/* PET behaviour: idle sways and blinks, naps after the configured idle
 * time, a click makes it happy (and wakes it) for three seconds, frames
 * advance at the mood's rate, every state of all five characters exists in
 * the art, and the characters' lines fit a tile value. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_pet.h"
static int drawn;
static void box(void *ctx,int c,int x,int y,int w,int h)
{(void)ctx;assert(c>=0 && c<16 && x>=0 && y>=0 && w>0 && h>0 && x+w<=32 && y+h<=32);++drawn;}
int main(void)
{
    struct NDPet p;
    int state,i,seen[ND_PET_STATES];
    char value[64];
    for(i=0;i<ND_PET_STATES*ND_PET_CHARACTERS;++i){drawn=0;nd_pet_art(NULL,box,i);assert(drawn>10);}
    assert(ND_PET_STATES*ND_PET_CHARACTERS<=256);   /* artwork bank limit */
    /* Characters, default names and lines. */
    for(i=0;i<ND_PET_CHARACTERS;++i)assert(nd_pet_character(nd_pet_characters[i])==i);
    assert(nd_pet_character("Nobody")==ND_PET_CAT && nd_pet_character(NULL)==ND_PET_CAT);
    for(i=0;i<ND_PET_LINES;++i){assert(strlen(nd_gerald_tips[i])<32 && strlen(nd_bob_lines[i])<32);}
    nd_pet_value(value,ND_PET_GERALD,"Gerald",ND_PET_MOOD_HAPPY,3);assert(!strcmp(value,nd_gerald_tips[3]));
    nd_pet_value(value,ND_PET_BOB,"Bob",ND_PET_MOOD_HAPPY,12);assert(!strcmp(value,nd_bob_lines[2]));
    nd_pet_value(value,ND_PET_GERALD,"Gerald",ND_PET_MOOD_IDLE,0);assert(!strcmp(value,"Gerald Hmph."));
    nd_pet_value(value,ND_PET_DOG,"Biscuit",ND_PET_MOOD_SLEEP,0);assert(!strcmp(value,"Biscuit Asleep"));
    nd_pet_value(value,ND_PET_CAT,"An extremely long pet name indeed",ND_PET_MOOD_HAPPY,0);assert(strlen(value)<32);
    nd_pet_init(&p);memset(seen,0,sizeof(seen));
    /* Idle: 8 frames at the interval, cycling. */
    for(i=0;i<16;++i){state=nd_pet_step(&p,25,0,1000000UL,25);assert(state>=0 && state<ND_PET_IDLE_FRAMES);seen[state]=1;}
    for(i=0;i<ND_PET_IDLE_FRAMES;++i)assert(seen[i]);
    assert(p.mood==ND_PET_MOOD_IDLE && nd_pet_wait(&p,25)==25);
    /* Naps after SLEEP_AFTER idle ticks; asleep frames run at half rate. */
    nd_pet_init(&p);
    assert(nd_pet_step(&p,5999,0,6000,25)<ND_PET_SLEEP_FIRST);
    state=nd_pet_step(&p,1,0,6000,25);
    assert(p.mood==ND_PET_MOOD_SLEEP && state==ND_PET_SLEEP_FIRST && nd_pet_wait(&p,25)==50);
    assert(nd_pet_step(&p,25,0,6000,25)==ND_PET_SLEEP_FIRST);
    assert(nd_pet_step(&p,25,0,6000,25)==ND_PET_SLEEP_FIRST+1);
    /* A click wakes it happy; happy frames are twice as fast. */
    state=nd_pet_step(&p,0,1,6000,25);
    assert(p.mood==ND_PET_MOOD_HAPPY && state==ND_PET_HAPPY_FIRST && nd_pet_wait(&p,25)==12);
    assert(!strcmp(nd_pet_mood_text(p.mood),"Happy!"));
    /* ... for three seconds, then purrs again (and the idle timer restarted). */
    state=nd_pet_step(&p,149,0,6000,25);assert(p.mood==ND_PET_MOOD_HAPPY && state>=ND_PET_HAPPY_FIRST);
    state=nd_pet_step(&p,1,0,6000,25);assert(p.mood==ND_PET_MOOD_IDLE && state<ND_PET_IDLE_FRAMES);
    assert(nd_pet_step(&p,5000,0,6000,25)<ND_PET_SLEEP_FIRST);
    /* A long gap (throttled dock) lands on the right frame without looping. */
    nd_pet_init(&p);state=nd_pet_step(&p,25*13,0,1000000UL,25);assert(state==13%ND_PET_IDLE_FRAMES);
    puts("PET: five characters (80 artwork states), lines, idle sway and blink, naps, click-to-wake happiness and frame rates passed.");
    return 0;
}
