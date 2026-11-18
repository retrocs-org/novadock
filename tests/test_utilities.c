#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_calc.h"
#include "nd_tooltypes.h"
#include "nd_monitor.h"
static struct NDToolTypes original,edited;
static struct NDCalcResult result;
static void decimal(const char *expression,const char *expected)
{assert(nd_calc_eval(expression,&result));if(strcmp(result.decimal,expected)){fprintf(stderr,"%s: %s != %s\n",expression,result.decimal,expected);assert(0);}}
int main(void)
{
 unsigned long stack,h;char path[240],long_text[220];unsigned int i;
 decimal("1.5 + 2.25","3.75");decimal("0.1+0.2","0.3");decimal("2+3*4","14");decimal("(2+3)*4","20");decimal("-5 % 3","-2");decimal("1/8","0.125");decimal("0xff << 4","4080");assert(!strcmp(result.hex,"0x00000ff0"));decimal("0xffffffff","4294967295");assert(strlen(result.binary)==34);decimal("~0","4294967295");decimal("0xffffffff >> 31","1");decimal("-2147483648","-2147483648");assert(!strcmp(result.hex,"0x80000000"));decimal("0b1010 ^ 3","9");decimal("3 & 2 | 4","6");
 assert(!nd_calc_eval("1/0",&result));assert(!nd_calc_eval("1%0",&result));assert(!nd_calc_eval("1.5 & 2",&result));assert(!nd_calc_eval("1 << 32",&result));assert(!nd_calc_eval("1 << -1",&result));assert(!nd_calc_eval("0x100000000",&result));assert(!nd_calc_eval("4294967295+1",&result));assert(!nd_calc_eval("(2+3",&result));assert(!nd_calc_eval("Execute quit",&result));memset(long_text,'(',sizeof(long_text));long_text[sizeof(long_text)-1]=0;assert(!nd_calc_eval(long_text,&result));memset(long_text,'-',40);long_text[40]='1';long_text[41]=0;assert(!nd_calc_eval(long_text,&result));
 assert(nd_tt_path("Work:Tools/App.info",path) && !strcmp(path,"Work:Tools/App"));assert(nd_tt_path("Work:Tools/App.INFO",path) && !strcmp(path,"Work:Tools/App"));assert(!nd_tt_path("relative.info",path));assert(!nd_tt_path("Work:bad\n.info",path));assert(nd_tt_stack("4096",&stack)&&stack==4096);assert(nd_tt_stack("0",&stack)&&!stack);assert(!nd_tt_stack("4095",&stack));assert(!nd_tt_stack("99999999999999",&stack));assert(nd_tt_line("(DISABLED=YES)"));assert(!nd_tt_line("X=1\nY=2"));
 strcpy(original.lines[0],"MODE=SAFE");strcpy(original.lines[1],"IM1=encoded-image");strcpy(original.lines[2],"IM2=selected-image");original.count=3;edited=original;strcpy(edited.lines[0],"MODE=FAST");assert(nd_tt_preserves_art(&original,&edited));strcpy(edited.lines[1],"IM1=changed");assert(!nd_tt_preserves_art(&original,&edited));edited=original;edited.count=1;assert(!nd_tt_preserves_art(&original,&edited));h=nd_tt_hash(2166136261UL,(const unsigned char *)"original",8);assert(h!=nd_tt_hash(2166136261UL,(const unsigned char *)"changed",7));
 assert(nd_watch_name("Demo.info","#?","#?.info" )==0);assert(nd_watch_name("Demo.INFO","#?","#?.info" )==0);assert(nd_watch_name("build.log","*.log;*.txt","*.tmp"));assert(!nd_watch_name("build.o","*.log;*.txt",""));assert(nd_watch_name("one.txt","???.txt",""));assert(!nd_watch_name("ones.txt","???.txt",""));assert(!nd_watch_name("tmp-one","#?","tmp*;*.info"));
 for(i=0;i<1000;++i){char expr[80],expected[48];sprintf(expr,"%u*3+2",i);sprintf(expected,"%lu",(unsigned long)i*3+2);decimal(expr,expected);}
 puts("CALC precedence/conversions/bounds, protected ToolTypes and WATCH pattern tests passed.");return 0;
}
