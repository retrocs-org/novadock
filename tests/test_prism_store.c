#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#ifdef TEST_RENAME_FAILURE
static int fail_rename=0;
int qa_prism_rename(const char *from,const char *to)
{if(fail_rename && strstr(from,"runtime.prefs.novadock-new")){fail_rename=0;errno=EIO;return -1;}return rename(from,to);}
#endif
#include "nd_prism.h"
static void write_file(const char *path,const char *text)
{FILE *f=fopen(path,"wb");assert(f);assert(fwrite(text,1,strlen(text),f)==strlen(text));assert(!fclose(f));}
static void read_file(const char *path,char out[4096])
{FILE *f=fopen(path,"rb");size_t n;assert(f);n=fread(out,1,4095,f);out[n]=0;assert(!fclose(f));}
int main(void)
{
 const char *paths[]={"permanent.prefs","runtime.prefs"};char text[4096];FILE *f;
 write_file(paths[0],"# permanent existing settings\n");write_file(paths[1],"# runtime existing settings\n");
#ifdef TEST_RENAME_FAILURE
 fail_rename=1;
 assert(nd_prism_save_paths(paths,"wifi","password",0,NULL)==ND_PRISM_PROFILE_FAILED);
 read_file(paths[0],text);assert(!strcmp(text,"# permanent existing settings\n"));
 read_file(paths[1],text);assert(!strcmp(text,"# runtime existing settings\n"));
 f=fopen("permanent.prefs.novadock-backup","rb");assert(!f);
 f=fopen("runtime.prefs.novadock-backup","rb");assert(!f);
#endif
 assert(nd_prism_save_paths(paths,"wifi","password",0,NULL)==ND_PRISM_OK);
 read_file(paths[0],text);assert(strstr(text,"# permanent existing settings\n"));assert(!strstr(text,"password"));
 read_file(paths[1],text);assert(strstr(text,"# runtime existing settings\n"));
 assert(nd_prism_save_paths(paths,"other","",1,NULL)==ND_PRISM_OK);
 read_file(paths[0],text);assert(strstr(text,"key_mgmt=NONE"));assert(!strstr(text,"ssid=77696669"));
 write_file("runtime.prefs.novadock-backup","unknown pending transaction");
 assert(nd_prism_save_paths(paths,"wifi","password",0,NULL)==ND_PRISM_PROFILE_FAILED);
 read_file(paths[0],text);assert(strstr(text,"key_mgmt=NONE"));
 read_file("runtime.prefs.novadock-backup",text);assert(!strcmp(text,"unknown pending transaction"));
 assert(!remove("runtime.prefs.novadock-backup"));
 write_file(paths[1],"# NovaDock Wi-Fi profile begin\nmalformed\n");
 assert(nd_prism_save_paths(paths,"wifi","password",0,NULL)==ND_PRISM_PROFILE_FAILED);
 read_file(paths[0],text);assert(strstr(text,"key_mgmt=NONE"));
 f=fopen("permanent.prefs.novadock-new","rb");assert(!f);
 return 0;
}
