/* PET: a small animated companion on a dock tile - a cat, Gerald the
 * grumpy stapler, a dog, a rabbit or Bob the rubber ball.  It idles, falls
 * asleep when left alone, and reacts when clicked (Gerald and Bob also
 * offer a line of dubious wisdom).  Frames
 * come from the plugin's artwork bank; the plugin only picks the state,
 * so the dock repaints one tile and no window is opened.  Sleeping halves
 * the frame rate, and a throttled (covered/hidden) dock slows it further. */
#include <exec/types.h>
#include <proto/exec.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_plugin_versions.h"
#include "nd_client.h"
#include "nd_option_runtime.h"
#include "nd_pet.h"
const char nd_version[]=ND_PLUGIN_VER_STRING(PET,"Pet");
long __stack=8192;
int main(int argc,char **argv)
{
    struct NDClient client;
    struct NDOptionString options[4];
    struct NDPet pet;
    char name[32],speed[8],sleep_text[8],character[12],value[48];
    unsigned int event,elapsed=0,clicks=0,delay,line=ND_PET_LINES-1;
    unsigned long interval=25UL,sleep_after=6000UL;
    int reread=1,state,who=ND_PET_CAT;
    if(argc<2||!nd_client_init(&client,argv[1]))return 20;
    nd_pet_init(&pet);
    for(;;){
        if(reread){
            options[0].key="NAME";options[0].fallback="Whiskers";options[0].out=name;options[0].limit=sizeof(name);
            options[1].key="SPEED";options[1].fallback="Normal";options[1].out=speed;options[1].limit=sizeof(speed);
            options[2].key="SLEEP";options[2].fallback="2";options[2].out=sleep_text;options[2].limit=sizeof(sleep_text);
            options[3].key="CHARACTER";options[3].fallback="Cat";options[3].out=character;options[3].limit=sizeof(character);
            nd_plugin_option_strings(argv[1],options,4);
            who=nd_pet_character(character);
            interval=!strcmp(speed,"Slow") ? 40UL:!strcmp(speed,"Fast") ? 15UL:25UL;
            sleep_after=(unsigned long)atoi(sleep_text);
            if(sleep_after<1UL)sleep_after=1UL;if(sleep_after>30UL)sleep_after=30UL;
            sleep_after*=60UL*50UL;
            /* An unchanged default name follows the character. */
            if(!name[0] || !strcmp(name,"Whiskers"))strcpy(name,nd_pet_default_names[who]);
            reread=0;
        }
        state=nd_pet_step(&pet,elapsed,clicks,sleep_after,interval);
        if(clicks)line=(line+1)%ND_PET_LINES;  /* each click: the next line */
        clicks=0;
        nd_pet_value(value,who,name,pet.mood,line);
        event=nd_client_update_art_quiet(&client,"PET",value,"pet",
                                         (unsigned int)(who*ND_PET_STATES+state),elapsed,500U);
        if(event&ND_EVENT_QUIT)break;
        if(event&ND_EVENT_SETTINGS)reread=1;
        if(event&ND_EVENT_CLICK)++clicks;
        /* Sleep until the next frame (or a click/settings change wakes us). */
        delay=client.throttled ? 500U:(unsigned int)nd_pet_wait(&pet,interval);
        if(clicks)delay=0;
        elapsed=delay ? nd_client_wait(&client,delay):0;
    }
    nd_client_close(&client);
    return 0;
}
