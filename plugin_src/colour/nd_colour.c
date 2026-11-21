#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#if defined(__amigaos__) || defined(AMIGA) || defined(__VBCC__)
#include <dos/dos.h>
#include <proto/dos.h>
#endif
#include "nd_colour.h"
#ifndef ND_COLOUR_RENAME
#define ND_COLOUR_RENAME rename
#endif
int nd_colour_hex(const char *s,unsigned long *out)
{unsigned int i;unsigned long value=0;if(!s||!out)return 0;if(*s=='#')++s;if(strlen(s)!=6)return 0;for(i=0;i<6;++i){unsigned int c=(unsigned char)s[i],n;if(c>='0'&&c<='9')n=c-'0';else if(c>='a'&&c<='f')n=c-'a'+10;else if(c>='A'&&c<='F')n=c-'A'+10;else return 0;value=(value<<4)|n;}*out=value;return 1;}
void nd_colour_text(unsigned long rgb,char hex[8],char decimal[32])
{sprintf(hex,"#%06lX",rgb&0xffffffUL);sprintf(decimal,"%lu, %lu, %lu",(rgb>>16)&255,(rgb>>8)&255,rgb&255);}
static int name_valid(const char *s)
{unsigned int i;if(!s||!*s||strlen(s)>31)return 0;for(i=0;s[i];++i)if((unsigned char)s[i]<32 || s[i]=='|')return 0;return 1;}
int nd_colour_add(struct NDColourPalette *p,const char *name,unsigned long rgb)
{unsigned int i;if(!p||!name_valid(name)||rgb>0xffffffUL||p->count>ND_COLOUR_MAX)return 0;for(i=0;i<p->count;++i)if(!strcmp(p->colours[i].name,name))break;if(i==ND_COLOUR_MAX)return 0;if(i==p->count)++p->count;strcpy(p->colours[i].name,name);p->colours[i].rgb=rgb;return 1;}
int nd_colour_remove(struct NDColourPalette *p,unsigned int index)
{unsigned int i;if(!p||index>=p->count||p->count>ND_COLOUR_MAX)return 0;for(i=index+1;i<p->count;++i)p->colours[i-1]=p->colours[i];--p->count;memset(&p->colours[p->count],0,sizeof(p->colours[0]));return 1;}
int nd_colour_parse(const char *text,struct NDColourPalette *p)
{struct NDColourPalette copy;const char *next,*end;char hex[8],name[32];unsigned long rgb;unsigned int n,i;if(!text||!p||strncmp(text,"NDCOLOUR1\n",10)||strlen(text)>2047)return 0;memset(&copy,0,sizeof(copy));next=text+10;while(*next){end=strchr(next,'\n');if(!end || end-next<8 || end-next>38 || next[6]!='|')return 0;memcpy(hex,next,6);hex[6]=0;n=(unsigned int)(end-next)-7;if(n>31)return 0;memcpy(name,next+7,n);name[n]=0;if(!nd_colour_hex(hex,&rgb)||!name_valid(name)||copy.count>=ND_COLOUR_MAX)return 0;for(i=0;i<copy.count;++i)if(!strcmp(copy.colours[i].name,name))return 0;nd_colour_add(&copy,name,rgb);next=end+1;}*p=copy;return 1;}
unsigned int nd_colour_format(const struct NDColourPalette *p,char out[2048])
{unsigned int i,used=10;if(!p||!out||p->count>ND_COLOUR_MAX)return 0;strcpy(out,"NDCOLOUR1\n");for(i=0;i<p->count;++i){if(!name_valid(p->colours[i].name)||p->colours[i].rgb>0xffffffUL)return 0;used+=(unsigned int)sprintf(out+used,"%06lX|%s\n",p->colours[i].rgb,p->colours[i].name);}return used;}
static int missing(const char *path)
{
#if defined(__amigaos__) || defined(AMIGA) || defined(__VBCC__)
 BPTR lock=Lock((STRPTR)path,ACCESS_READ);LONG error;if(lock){UnLock(lock);return 0;}error=IoErr();return error==ERROR_OBJECT_NOT_FOUND||error==ERROR_DIR_NOT_FOUND;
#else
 (void)path;return errno==ENOENT;
#endif
}
int nd_colour_read(const char *path,struct NDColourPalette *p)
{FILE *f;char text[2048];size_t n;int ok;errno=0;f=fopen(path,"rb");if(!f)return missing(path) ? -1:0;n=fread(text,1,sizeof(text)-1,f);text[n]=0;ok=!ferror(f)&&fgetc(f)==EOF&&!memchr(text,0,n);if(fclose(f))ok=0;return ok&&nd_colour_parse(text,p);}
static const char *save_problem="Palette could not be saved; previous file retained";
const char *nd_colour_save_problem(void){return save_problem;}
static void save_failure(const char *fallback)
{
 save_problem=fallback;
#if defined(__amigaos__) || defined(AMIGA) || defined(__VBCC__)
 {LONG error=IoErr();if(error==ERROR_DISK_WRITE_PROTECTED||error==ERROR_WRITE_PROTECTED||error==ERROR_DELETE_PROTECTED)save_problem="Palette destination is write-protected; edits retained";else if(error==ERROR_DISK_FULL)save_problem="Palette destination is full; edits retained";}
#else
 if(errno==EACCES||errno==EPERM||errno==EROFS)save_problem="Palette destination is write-protected; edits retained";
 else if(errno==ENOSPC)save_problem="Palette destination is full; edits retained";
#endif
}
int nd_colour_save(const char *path,const struct NDColourPalette *p)
{
 char temp[320],backup[320],text[2048];struct NDColourPalette old,check;FILE *f;unsigned int n,i;int existing,ok;
 save_problem="Invalid palette or destination; edits retained";
 if(!path||strlen(path)>280||!(n=nd_colour_format(p,text)))return 0;
 existing=nd_colour_read(path,&old);if(existing==0){save_problem="Existing palette is unreadable or damaged; file retained";return 0;}
 sprintf(temp,"%s.new",path);sprintf(backup,"%s.bak",path);errno=0;f=fopen(temp,"wb");
 if(!f){save_failure("Cannot create palette file; check destination");return 0;}
 ok=fwrite(text,1,n,f)==n;if(!ok)save_failure("Palette write failed; previous file retained");
 if(fclose(f)){if(ok)save_failure("Palette close failed; previous file retained");ok=0;}
 if(!ok){remove(temp);return 0;}
 if(nd_colour_read(temp,&check)!=1||check.count!=p->count){save_problem="Palette verification failed; previous file retained";remove(temp);return 0;}
 for(i=0;i<p->count;++i)if(check.colours[i].rgb!=p->colours[i].rgb||strcmp(check.colours[i].name,p->colours[i].name)){save_problem="Palette verification failed; previous file retained";remove(temp);return 0;}
 if(existing==1){remove(backup);if(ND_COLOUR_RENAME(path,backup)){save_failure("Cannot replace palette; previous file retained");remove(temp);return 0;}}
 if(ND_COLOUR_RENAME(temp,path)){save_failure("Palette replacement failed; previous file retained");if(existing==1&&ND_COLOUR_RENAME(backup,path))save_problem="Save failed; original palette is preserved in .bak";remove(temp);return 0;}
 save_problem="Palette saved";return 1;
}
