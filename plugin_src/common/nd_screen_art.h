#ifndef ND_SCREEN_ART_H
#define ND_SCREEN_ART_H
#include <string.h>
/* Palette roles: background, metal, paper, accent, success. */
typedef void (*NDArtRect)(void *,int,int,int,int,int);
#define ND_SHOT_IDLE 0
#define ND_SHOT_COUNTDOWN 1
#define ND_SHOT_CAPTURE 2
#define ND_SHOT_SAVED 3
#define ND_SHOT_ERROR 4
#define ND_SHOT_CANCELLED 5
static int nd_shot_art_state(const char *value,int online)
{
    if(!online || !strcmp(value,"READY"))return ND_SHOT_IDLE;
    if(!strncmp(value,"IN ",3))return ND_SHOT_COUNTDOWN;
    if(!strcmp(value,"CAPTURING") || !strncmp(value,"SAVING",6))return ND_SHOT_CAPTURE;
    if(!strncmp(value,"SAVED",5))return ND_SHOT_SAVED;
    if(!strcmp(value,"CANCELLED"))return ND_SHOT_CANCELLED;
    return ND_SHOT_ERROR;
}
static void nd_camera_art(void *ctx,NDArtRect box,int state)
{
    int i;
    box(ctx,0,3,9,28,21);box(ctx,1,1,7,28,21);
    box(ctx,2,2,8,26,3);box(ctx,3,2,12,3,14);
    box(ctx,0,8,3,13,5);box(ctx,2,9,4,11,3);
    box(ctx,2,24,5,4,2);box(ctx,0,3,14,24,11);
    box(ctx,2,11,11,9,15);box(ctx,2,8,14,15,9);
    box(ctx,1,11,13,9,11);box(ctx,1,10,14,11,9);
    box(ctx,0,12,15,7,7);box(ctx,3,13,16,5,5);
    box(ctx,2,13,16,2,2);
    if(state==ND_SHOT_COUNTDOWN){
        box(ctx,0,21,0,11,11);box(ctx,2,23,2,7,7);
        box(ctx,3,26,3,1,4);box(ctx,3,26,6,3,1);
    }else if(state==ND_SHOT_CAPTURE){
        for(i=0;i<5;++i)box(ctx,4,22-i,1+i,8,1);
        for(i=0;i<5;++i)box(ctx,4,23+i,6+i,8-2*i,1);
        box(ctx,4,14,17,3,3);
    }else if(state==ND_SHOT_SAVED){
        box(ctx,0,19,19,13,13);
        for(i=0;i<4;++i)box(ctx,4,20+i,24+i,2,2);
        for(i=0;i<7;++i)box(ctx,4,23+i,27-i,2,2);
    }else if(state==ND_SHOT_ERROR || state==ND_SHOT_CANCELLED){
        box(ctx,0,21,20,11,12);
        for(i=0;i<8;++i){box(ctx,3,23+i,22+i,1,1);box(ctx,3,30-i,22+i,1,1);}
    }
}
static void nd_modes_art(void *ctx,NDArtRect box)
{
    int i;
    box(ctx,0,3,4,28,21);box(ctx,2,1,2,28,21);
    box(ctx,1,2,3,26,19);box(ctx,0,4,5,22,15);
    box(ctx,3,5,6,20,3);
    for(i=0;i<3;++i){box(ctx,2,7,11+i*3,2,1);box(ctx,1,11,11+i*3,11,1);}
    box(ctx,4,6,13,18,3);box(ctx,0,8,14,2,1);
    box(ctx,0,12,23,9,6);box(ctx,1,13,23,7,5);
    box(ctx,2,8,29,17,2);box(ctx,3,9,30,15,1);
}
#endif
