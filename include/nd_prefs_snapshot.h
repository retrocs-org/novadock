#ifndef ND_PREFS_SNAPSHOT_H
#define ND_PREFS_SNAPSHOT_H
/* Private, same-process preview backup. NDConfig contains no pointers.
 * Permanent settings still use the validated, recoverable text writer. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_config.h"
static unsigned long nd_prefs_snapshot_hash(const unsigned char *p,size_t n)
{unsigned long sum=0;while(n--)sum=((sum<<5)-sum+*p++)&0xffffffffUL;return sum;}
static void nd_prefs_snapshot_u32(unsigned char *p,unsigned long n)
{p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
static unsigned long nd_prefs_snapshot_read32(const unsigned char *p)
{return ((unsigned long)p[0]<<24)|((unsigned long)p[1]<<16)|((unsigned long)p[2]<<8)|p[3];}
static int nd_prefs_snapshot_valid(const struct NDConfig *c)
{
 int i,j;unsigned int offset;
 if(c->count<0 || c->count>ND_MAX_ITEMS || c->option_count<0 || c->option_count>ND_MAX_OPTIONS ||
    c->def_count<0 || c->def_count>ND_MAX_OPTION_DEFS || c->hook_count<0 || c->hook_count>ND_MAX_HOOKS ||
    c->unknown_count<0 || c->unknown_count>ND_MAX_UNKNOWN || c->text_used>ND_TEXT_POOL_SIZE || c->text_pool[0])return 0;
 for(i=0;i<c->count;++i)for(j=0;j<ND_TEXT_COUNT;++j){offset=c->items[i].text[j];
  if(offset>=ND_TEXT_POOL_SIZE || (offset && offset>=c->text_used) || !memchr(c->text_pool+offset,0,ND_TEXT_POOL_SIZE-offset))return 0;}
 return 1;
}
static int nd_prefs_snapshot_save(const struct NDConfig *c,const char *path)
{
 unsigned char header[16];FILE *f;int ok;
 if(!c || !path || !nd_prefs_snapshot_valid(c))return 0;
 memcpy(header,"NDPS\0\1\0\0",8);nd_prefs_snapshot_u32(header+8,sizeof(*c));
 nd_prefs_snapshot_u32(header+12,nd_prefs_snapshot_hash((const unsigned char *)c,sizeof(*c)));
 f=fopen(path,"wb");if(!f)return 0;
 ok=fwrite(header,1,sizeof(header),f)==sizeof(header) && fwrite(c,1,sizeof(*c),f)==sizeof(*c);
 if(fclose(f))ok=0;if(!ok)remove(path);return ok;
}
static int nd_prefs_snapshot_load(struct NDConfig *c,const char *path)
{
 unsigned char header[16];struct NDConfig *saved;FILE *f;int ok=0;
 if(!c || !path)return 0;f=fopen(path,"rb");if(!f)return 0;
 saved=(struct NDConfig *)malloc(sizeof(*saved));if(!saved){fclose(f);return 0;}
 if(fread(header,1,sizeof(header),f)==sizeof(header) && !memcmp(header,"NDPS\0\1\0\0",8) &&
    nd_prefs_snapshot_read32(header+8)==sizeof(*saved) && fread(saved,1,sizeof(*saved),f)==sizeof(*saved) &&
    fgetc(f)==EOF && !ferror(f) && nd_prefs_snapshot_read32(header+12)==nd_prefs_snapshot_hash((const unsigned char *)saved,sizeof(*saved)) &&
    nd_prefs_snapshot_valid(saved))ok=1;
 if(fclose(f))ok=0;if(ok)*c=*saved;free(saved);return ok;
}
#endif
