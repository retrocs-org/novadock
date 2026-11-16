#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_games_cache.h"
#ifdef TEST_AMIGA_RENAME
#undef rename
extern int rename(const char *,const char *);
static int fail_final=0;
int qa_rename(const char *from,const char *to)
{FILE *existing;if(fail_final && strstr(from,".tmp"))return -1;existing=fopen(to,"rb");if(existing){fclose(existing);return -1;}return rename(from,to);}
#endif
struct Records {int at,count,fail;const char *names[3],*paths[3];char read_names[3][108],read_paths[3][300];};
static int next(void *ctx,char *name,unsigned int ns,char *path,unsigned int ps)
{struct Records *r=ctx;int i=r->at++;if(i>=r->count || (r->fail && i==1))return 0;assert(strlen(r->names[i])<ns && strlen(r->paths[i])<ps);strcpy(name,r->names[i]);strcpy(path,r->paths[i]);return 1;}
static int add(void *ctx,const char *name,const char *path)
{struct Records *r=ctx;assert(r->at<3);strcpy(r->read_names[r->at],name);strcpy(r->read_paths[r->at++],path);return 1;}
static int bytes(const char *file,unsigned char *data){FILE *f=fopen(file,"rb");int n;assert(f);n=(int)fread(data,1,4096,f);fclose(f);return n;}
int main(void){struct Records write,read;unsigned char original[4096],current[4096];int n,m;FILE *f;
 memset(&write,0,sizeof(write));write.count=2;write.names[0]="Game % One";write.names[1]="Demo \"two\"";write.paths[0]="Games:Action/A/One";write.paths[1]="Games:Demos/Two";
 assert(nd_games_cache_write("games.idx","Games:",2,next,&write));n=bytes("games.idx",original);
 memset(&read,0,sizeof(read));assert(nd_games_cache_read("games.idx","Games:",add,&read));assert(read.at==2 && !strcmp(read.read_names[1],write.names[1]) && !strcmp(read.read_paths[0],write.paths[0]));
 memset(&read,0,sizeof(read));assert(!nd_games_cache_read("games.idx","Demos:",add,&read));assert(read.at==0);
 write.at=0;write.fail=1;assert(!nd_games_cache_write("games.idx","Games:",2,next,&write));m=bytes("games.idx",current);assert(n==m && !memcmp(original,current,n));
 write.at=0;write.fail=0;write.paths[1]="Outside:App";assert(!nd_games_cache_write("games.idx","Games:",2,next,&write));m=bytes("games.idx",current);assert(n==m && !memcmp(original,current,n));write.paths[1]="Games:Demos/Two";
 f=fopen("corrupt.idx","wb");assert(f);current[n-1]^=1;assert(fwrite(current,1,n,f)==(unsigned int)n);fclose(f);memset(&read,0,sizeof(read));assert(!nd_games_cache_read("corrupt.idx","Games:",add,&read));assert(read.at==0);
 f=fopen("truncated.idx","wb");assert(f);fwrite(original,1,n-5,f);fclose(f);assert(!nd_games_cache_read("truncated.idx","Games:",add,&read));
 f=fopen("version.idx","wb");assert(f);original[5]=99;fwrite(original,1,n,f);fclose(f);assert(!nd_games_cache_read("version.idx","Games:",add,&read));
 write.at=0;assert(nd_games_cache_write_options("music.idx","Games:",0x80000040UL,2,next,&write));
 memset(&read,0,sizeof(read));assert(nd_games_cache_read_options("music.idx","Games:",0x80000040UL,add,&read) && read.at==2);
 memset(&read,0,sizeof(read));assert(!nd_games_cache_read_options("music.idx","Games:",0x80000001UL,add,&read) && !read.at);
 assert(!nd_games_cache_read("music.idx","Games:",add,&read) && !read.at);
 assert(!nd_games_cache_read_options("games.idx","Games:",0x80000040UL,add,&read) && !read.at);
 write.at=0;write.count=0;assert(nd_games_cache_write("empty.idx","Games:",0,next,&write));memset(&read,0,sizeof(read));assert(nd_games_cache_read("empty.idx","Games:",add,&read) && read.at==0);
 write.at=0;assert(!nd_games_cache_write("absent/cache.idx","Games:",0,next,&write));
#ifdef TEST_AMIGA_RENAME
 n=bytes("games.idx",original);write.at=0;write.count=2;fail_final=1;
 assert(!nd_games_cache_write("games.idx","Games:",2,next,&write));fail_final=0;
 m=bytes("games.idx",current);assert(n==m && !memcmp(original,current,n));
 memset(&read,0,sizeof(read));assert(nd_games_cache_read("games.idx","Games:",add,&read) && read.at==2);
#endif
 puts("WHDLoad persistent caches: round trip, folder isolation, corruption, truncation and failed replacement passed.");return 0;}
