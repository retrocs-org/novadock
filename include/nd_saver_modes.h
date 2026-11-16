#ifndef ND_SAVER_MODES_H
#define ND_SAVER_MODES_H
#include <string.h>
#define ND_SAVER_CHOICES "Starfield,Boing ball,Bouncing ball,Blank,Copper waves,Vector show,Fireworks,Maze,Demo shuffle"
#define ND_SAVER_SHUFFLE 8
#define ND_SAVER_SCENES 8
#define ND_SAVER_SHUFFLE_OPTIONS(X) \
 X(STARS,"Starfield",0,"YES") \
 X(BOING,"Boing ball",3,"YES") \
 X(BALL,"Bouncing ball",1,"YES") \
 X(BLANK,"Blank",2,"NO") \
 X(COPPER,"Copper waves",4,"YES") \
 X(VECTOR,"Vector show",5,"YES") \
 X(FIREWORKS,"Fireworks",6,"YES") \
 X(MAZE,"Workbench maze",7,"YES")
#ifndef ND_SAVER_METADATA_ONLY
static const char *nd_saver_name(int mode)
{static const char *names[]={"Starfield","Bouncing ball","Blank","Boing ball","Copper waves","Vector show","Fireworks","Workbench maze","Demo shuffle"};return mode>=0&&mode<=8 ? names[mode]:names[0];}
static int nd_saver_mode(const char *text)
{int i;if(!strcmp(text,"Maze"))return 7;for(i=0;i<=8;++i)if(!strcmp(text,nd_saver_name(i)))return i;return 0;}
static int nd_saver_pick(int mode,unsigned int mask,unsigned long ticks,unsigned long duration)
{unsigned int n=0,i,choice;if(mode!=8)return mode;if(!duration)duration=2000;for(i=0;i<8;++i)if(mask&(1U<<i))++n;if(!n)return 0;choice=(unsigned int)((ticks/duration)%n);for(i=0;i<8;++i)if(mask&(1U<<i)){if(!choice--)return (int)i;}return 0;}
#endif /* ND_SAVER_METADATA_ONLY */
#endif
