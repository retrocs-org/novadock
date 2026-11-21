#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_colour.h"
static int fail;
int qa_colour_rename(const char *from,const char *to)
{if(fail&&strstr(from,".new")){if(fail==1)fail=0;return -1;}if(fail==2&&strstr(from,".bak"))return -1;return rename(from,to);}
int main(void)
{struct NDColourPalette p,check;FILE *f;memset(&p,0,sizeof(p));assert(nd_colour_add(&p,"Accent",0x123456));assert(nd_colour_save("palette",&p));p.colours[0].rgb=0xabcdef;fail=1;assert(!nd_colour_save("palette",&p));assert(nd_colour_read("palette",&check)==1&&check.colours[0].rgb==0x123456);assert(!fopen("palette.new","rb"));fail=2;assert(!nd_colour_save("palette",&p));assert(strstr(nd_colour_save_problem(),".bak"));assert(nd_colour_read("palette.bak",&check)==1&&check.colours[0].rgb==0x123456);assert(nd_colour_read("palette",&check)==-1);assert(!fopen("palette.new","rb"));fail=0;assert(!rename("palette.bak","palette"));assert(nd_colour_save("palette",&p));assert(nd_colour_read("palette.bak",&check)==1&&check.colours[0].rgb==0x123456);f=fopen("palette","wb");assert(f);fwrite("NDCOLOUR1\n\0hidden",1,17,f);fclose(f);assert(!nd_colour_save("palette",&p));f=fopen("palette","rb");assert(f);{char b[32];assert(fread(b,1,17,f)==17&&!memcmp(b,"NDCOLOUR1\n\0hidden",17));}fclose(f);puts("COLOUR rollback, retained backup after rollback failure and malformed-file preservation passed.");return 0;}
