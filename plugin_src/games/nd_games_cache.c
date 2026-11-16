#include "nd_games_cache.h"
#include <stdio.h>
#include <string.h>
static const unsigned char magic[8]={'N','D','W','H','D',1,13,10};
struct CacheIO {FILE *file;unsigned long checksum;int ok;};
static void digest(struct CacheIO *io,int byte)
{io->checksum=((io->checksum^(unsigned char)byte)*16777619UL)&0xffffffffUL;}
static int get_byte(struct CacheIO *io)
{int c=fgetc(io->file);if(c==EOF){io->ok=0;return 0;}digest(io,c);return c;}
static void put_byte(struct CacheIO *io,int c)
{if(fputc(c,io->file)==EOF)io->ok=0;digest(io,c);}
static unsigned long get_number(struct CacheIO *io,int bytes)
{unsigned long value=0;while(bytes--)value=(value<<8)|(unsigned int)get_byte(io);return value;}
static void put_number(struct CacheIO *io,unsigned long value,int bytes)
{while(bytes--)put_byte(io,(int)((value>>(bytes*8))&255UL));}
static int get_string(struct CacheIO *io,char *out,unsigned int size)
{unsigned long n=get_number(io,2),i;if(!io->ok || !n || n>=size)return 0;for(i=0;i<n;++i){out[i]=(char)get_byte(io);if(!out[i])return 0;}out[n]=0;return io->ok;}
static int put_string(struct CacheIO *io,const char *text,unsigned int size)
{unsigned int n=(unsigned int)strlen(text),i;if(!n || n>=size)return 0;put_number(io,n,2);for(i=0;i<n;++i)put_byte(io,(unsigned char)text[i]);return io->ok;}
static int inside(const char *root,const char *project)
{unsigned int n=(unsigned int)strlen(root);return !strncmp(root,project,n) && (!project[n] || root[n-1]==':' || root[n-1]=='/' || project[n]=='/');}
static int read_pass(FILE *file,const char *expected,unsigned long options,NDGamesCacheAdd add,void *context)
{
    struct CacheIO io;unsigned char header[8];unsigned long count,i,stored=0;int c,j;
    char root[ND_GAMES_SCAN_PATH],name[ND_GAMES_SCAN_NAME],project[ND_GAMES_SCAN_PATH];
    if(fread(header,1,8,file)!=8 || memcmp(header,magic,5) || header[6]!=13 || header[7]!=10 || (header[5]!=1 && header[5]!=2))return 0;
    io.file=file;io.ok=1;io.checksum=2166136261UL;
    if(!get_string(&io,root,sizeof(root)) || strcmp(root,expected))return 0;
    if(header[5]==2){if(get_number(&io,4)!=options)return 0;}else if(options)return 0;
    count=get_number(&io,4);if(!io.ok || count>ND_GAMES_CACHE_MAX)return 0;
    for(i=0;i<count;++i){
        if(!get_string(&io,name,sizeof(name)) || !get_string(&io,project,sizeof(project)) || !inside(root,project))return 0;
        if(add && !add(context,name,project))return 0;
    }
    for(j=0;j<4;++j){c=fgetc(file);if(c==EOF)return 0;stored=(stored<<8)|(unsigned int)c;}
    return io.ok && stored==io.checksum && fgetc(file)==EOF && !ferror(file);
}
int nd_games_cache_read_options(const char *file,const char *root,unsigned long options,NDGamesCacheAdd add,void *context)
{
    FILE *f;int ok;
    if(!file || !root || !*root || !add)return 0;
    f=fopen(file,"rb");if(!f)return 0;
    ok=read_pass(f,root,options,NULL,NULL) && fseek(f,0,SEEK_SET)==0 && read_pass(f,root,options,add,context);
    if(fclose(f)!=0)ok=0;return ok;
}
int nd_games_cache_write_options(const char *file,const char *root,unsigned long options,unsigned long count,
                          NDGamesCacheNext next,void *context)
{
    char temporary[400],backup[400],name[ND_GAMES_SCAN_NAME],project[ND_GAMES_SCAN_PATH];
    struct CacheIO io;unsigned char header[8];unsigned long i,checksum;FILE *old;int ok,j,had_old;
    if(!file || !root || !*root || !next || strlen(file)+5>=sizeof(temporary) || count>ND_GAMES_CACHE_MAX)return 0;
    sprintf(temporary,"%s.tmp",file);sprintf(backup,"%s.bak",file);
    io.file=fopen(temporary,"wb");if(!io.file)return 0;
    memcpy(header,magic,8);if(options)header[5]=2;
    io.ok=fwrite(header,1,8,io.file)==8;io.checksum=2166136261UL;
    ok=put_string(&io,root,ND_GAMES_SCAN_PATH);if(options)put_number(&io,options,4);put_number(&io,count,4);
    for(i=0;ok && i<count;++i){
        name[0]=project[0]=0;
        ok=next(context,name,sizeof(name),project,sizeof(project));
        name[sizeof(name)-1]=project[sizeof(project)-1]=0;
        if(ok)ok=inside(root,project) && put_string(&io,name,sizeof(name)) && put_string(&io,project,sizeof(project));
    }
    if(ok)ok=!next(context,name,sizeof(name),project,sizeof(project));
    checksum=io.checksum;
    for(j=3;j>=0;--j)if(fputc((int)((checksum>>(j*8))&255UL),io.file)==EOF)io.ok=0;
    ok=ok && io.ok && !ferror(io.file);if(fclose(io.file)!=0)ok=0;
    if(!ok){remove(temporary);return 0;}
    /* POSIX replaces atomically. AmigaDOS needs the old destination moved
     * aside first; retain it until the completed new file has been renamed. */
    if(rename(temporary,file)==0)return 1;
    old=fopen(file,"rb");had_old=old!=NULL;if(old)fclose(old);
    if(had_old){remove(backup);if(rename(file,backup)!=0){remove(temporary);return 0;}}
    if(rename(temporary,file)!=0){if(had_old)(void)rename(backup,file);remove(temporary);return 0;}
    if(had_old)remove(backup);return 1;
}

int nd_games_cache_read(const char *file,const char *root,NDGamesCacheAdd add,void *context)
{return nd_games_cache_read_options(file,root,0,add,context);}
int nd_games_cache_write(const char *file,const char *root,unsigned long count,NDGamesCacheNext next,void *context)
{return nd_games_cache_write_options(file,root,0,count,next,context);}
