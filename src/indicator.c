#include "nd_indicator.h"
#include <string.h>
int nd_indicator_mode(const char *id,int online,int marks,
                      unsigned int media,const char *badge)
{
    int state_dot;
    if(!id || !online)return ND_INDICATOR_NONE;
    if(!badge)badge="";
    state_dot=!strcmp(id,"AMIRC") || !strcmp(id,"DOPUS4") || !strcmp(id,"ROADSHOW") || !strcmp(id,"NET");
    if(state_dot && !strcmp(badge,"*"))return marks ? ND_INDICATOR_DOT : ND_INDICATOR_NONE;
    if(*badge)return ND_INDICATOR_BADGE; /* explicit notifications, never IPC liveness */
    if(!strcmp(id,"MUSIC") && marks && media)return ND_INDICATOR_DOT;
    return ND_INDICATOR_NONE;
}
