/* Two-file profile update: stage both, keep originals until both renames
 * succeed, and roll back the first if the second fails. Never overwrite an
 * unknown pending transaction. Persistent files contain the derived PSK. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "nd_prism.h"
#include "../warp/nd_warp_control.h"
#define PROFILE_LIMIT 65536U
#ifndef ND_PRISM_RENAME
#define ND_PRISM_RENAME rename
#endif
struct ProfileFile {char path[260],temp[280],backup[280];char *old,*next;unsigned int size;int existed,staged,moved,committed;};
static int absent(const char *path)
{FILE *f=fopen(path,"rb");if(f){fclose(f);return 0;}return errno==ENOENT;}
static int read_profile(struct ProfileFile *p,const char *path)
{
 FILE *f;long bytes;int extra;
 if(!path || !*path || strlen(path)>=sizeof(p->path))return 0;
 strcpy(p->path,path);strcpy(p->temp,path);strcat(p->temp,".novadock-new");strcpy(p->backup,path);strcat(p->backup,".novadock-backup");
 if(!absent(p->temp) || !absent(p->backup))return 0;
 f=fopen(path,"rb");p->existed=f!=NULL;
 if(!f && errno!=ENOENT)return 0;
 if(!f)bytes=0;
 else{if(fseek(f,0,SEEK_END) || (bytes=ftell(f))<0 || bytes>PROFILE_LIMIT || fseek(f,0,SEEK_SET)){fclose(f);return 0;}}
 p->size=(unsigned int)bytes;p->old=malloc(p->size+1);p->next=malloc(p->size+512);
 if(!p->old || !p->next){if(f)fclose(f);return 0;}
 if(f){extra=fread(p->old,1,p->size,f)!=p->size || fgetc(f)!=EOF || ferror(f);if(fclose(f))extra=1;if(extra)return 0;}
 p->old[p->size]=0;return strlen(p->old)==p->size;
}
static int still_matches(struct ProfileFile *p)
{
 FILE *f;unsigned int i;int c;
 if(!p->existed)return absent(p->path);
 f=fopen(p->path,"rb");if(!f)return 0;
 for(i=0;i<p->size;++i){c=fgetc(f);if(c==EOF || c!=(unsigned char)p->old[i]){fclose(f);return 0;}}
 c=fgetc(f);i=c==EOF && !ferror(f);if(fclose(f))i=0;return i!=0;
}
int nd_prism_save_paths(const char *paths[2],const char *ssid,const char *password,int open_network,void (*yield)(void))
{
 struct ProfileFile p[2];unsigned char key[32];FILE *f;unsigned int i,n;int result=ND_PRISM_PROFILE_FAILED,okay;
 memset(p,0,sizeof(p));memset(key,0,sizeof(key));
 if(!paths || !nd_prism_credentials(ssid,password,open_network))return ND_PRISM_INVALID;
 if(!open_network && !nd_prism_psk(ssid,password,key,yield))return ND_PRISM_INVALID;
 for(i=0;i<2;++i){
  if(!read_profile(&p[i],paths[i]) || !nd_prism_profile(p[i].old,ssid,key,open_network,p[i].next,p[i].size+512))goto finished;
  f=fopen(p[i].temp,"wb");if(!f)goto finished;p[i].staged=1;n=(unsigned int)strlen(p[i].next);
  okay=fwrite(p[i].next,1,n,f)==n;if(fflush(f))okay=0;if(fclose(f))okay=0;if(!okay)goto finished;
 }
 for(i=0;i<2;++i)if(!still_matches(&p[i]))goto finished;
 for(i=0;i<2;++i){
  if(p[i].existed){if(ND_PRISM_RENAME(p[i].path,p[i].backup))goto rollback;p[i].moved=1;}
  if(ND_PRISM_RENAME(p[i].temp,p[i].path))goto rollback;
  p[i].committed=1;p[i].staged=0;
 }
 result=ND_PRISM_OK;
 for(i=0;i<2;++i)if(p[i].moved){if(remove(p[i].backup))result=ND_PRISM_PARTIAL;else p[i].moved=0;}
 goto finished;
rollback:
 for(i=0;i<2;++i){
  if(p[i].committed && remove(p[i].path)){result=ND_PRISM_PARTIAL;continue;}
  if(p[i].moved){if(ND_PRISM_RENAME(p[i].backup,p[i].path))result=ND_PRISM_PARTIAL;else p[i].moved=0;}
 }
finished:
 nd_warp_erase(key,sizeof(key));
 for(i=0;i<2;++i){
  if(p[i].staged)remove(p[i].temp);
  if(p[i].old){nd_warp_erase(p[i].old,p[i].size+1);free(p[i].old);}
  if(p[i].next){nd_warp_erase(p[i].next,p[i].size+512);free(p[i].next);}
 }
 return result;
}
