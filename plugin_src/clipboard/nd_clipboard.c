#include "nd_clipboard.h"
#include <stdlib.h>
#include <string.h>
static unsigned long be32(const unsigned char *p)
{return ((unsigned long)p[0]<<24)|((unsigned long)p[1]<<16)|((unsigned long)p[2]<<8)|p[3];}
static void put32(unsigned char *p,unsigned long n)
{p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
void nd_clip_history_init(struct NDClipHistory *h,unsigned int limit)
{memset(h,0,sizeof(*h));h->limit=limit && limit<=ND_CLIP_MAX ? limit : 10;}
int nd_clip_history_delete(struct NDClipHistory *h,unsigned int i)
{
    if(i>=h->count)return 0;
    free(h->text[i]);--h->count;
    for(;i<h->count;++i){h->text[i]=h->text[i+1];h->pinned[i]=h->pinned[i+1];}
    h->text[h->count]=NULL;h->pinned[h->count]=0;return 1;
}
int nd_clip_history_pin(struct NDClipHistory *h,unsigned int i)
{if(!h || i>=h->count)return 0;h->pinned[i]=(unsigned char)!h->pinned[i];return 1;}
void nd_clip_history_clear(struct NDClipHistory *h)
{while(h->count)nd_clip_history_delete(h,h->count-1);}
void nd_clip_history_limit(struct NDClipHistory *h,unsigned int limit)
{
    if(!limit || limit>ND_CLIP_MAX)return;
    h->limit=limit;while(h->count>limit)nd_clip_history_delete(h,h->count-1);
}
int nd_clip_history_add(struct NDClipHistory *h,const char *text)
{
    unsigned int i,j;unsigned char pin;char *copy;size_t n;
    if(!text || !(n=strlen(text)) || n>ND_CLIP_TEXT_MAX)return 0;
    for(i=0;i<h->count;++i)if(!strcmp(h->text[i],text)){
        if(!i)return 0;
        copy=h->text[i];pin=h->pinned[i];
        for(j=i;j>0;--j){h->text[j]=h->text[j-1];h->pinned[j]=h->pinned[j-1];}
        h->text[0]=copy;h->pinned[0]=pin;return 1;
    }
    copy=malloc(n+1);if(!copy)return 0;memcpy(copy,text,n+1);
    if(h->count==h->limit){
        for(i=h->count;i>0 && h->pinned[i-1];--i){}
        if(!i){free(copy);return 0;}
        nd_clip_history_delete(h,i-1);
    }
    for(i=h->count;i>0;--i){h->text[i]=h->text[i-1];h->pinned[i]=h->pinned[i-1];}
    h->text[0]=copy;h->pinned[0]=0;++h->count;return 1;
}
int nd_clip_decode(const unsigned char *data,unsigned long size,char *text,unsigned int limit)
{
    unsigned long end,pos,n,pad;unsigned int used=0;int found=0,result=-1;
    if(!text || !limit)return -1;text[0]=0;
    if(size<12 || memcmp(data,"FORM",4) || memcmp(data+8,"FTXT",4))return 0;
    end=be32(data+4);if(end<4 || end>size-8)return -1;end+=8;
    for(pos=12;pos<end;){
        if(end-pos<8)goto bad;
        n=be32(data+pos+4);pad=n&1UL;
        if(n>end-pos-8 || pad>end-pos-8-n)goto bad;
        if(!memcmp(data+pos,"CHRS",4)){
            if(n>=limit-used){result=-2;goto bad;}
            if(memchr(data+pos+8,0,(size_t)n))goto bad;
            memcpy(text+used,data+pos+8,(size_t)n);used+=(unsigned int)n;found=1;
        }
        pos+=8+n+pad;
    }
    text[used]=0;return found && used ? 1 : 0;
bad:
    /* Never leave a half-copied, unterminated text behind. */
    text[0]=0;
    return result;
}
unsigned long nd_clip_encode(const char *text,unsigned char *data,unsigned long size)
{
    unsigned long n,total;
    if(!text || !(n=(unsigned long)strlen(text)) || n>ND_CLIP_TEXT_MAX)return 0;
    total=20+n+(n&1UL);if(total>size)return 0;
    memcpy(data,"FORM",4);put32(data+4,total-8);memcpy(data+8,"FTXTCHRS",8);
    put32(data+16,n);memcpy(data+20,text,(size_t)n);if(n&1UL)data[20+n]=0;
    return total;
}
void nd_clip_preview(char *out,unsigned int size,const char *text)
{
    unsigned int n=0;unsigned char c;
    if(!out || !size)return;
    while(text && *text && n+1<size){
        c=(unsigned char)*text++;
        if(c<32 || c==127)c=' ';
        out[n++]=(char)c;
    }
    if(text && *text && n>3){out[n-3]='.';out[n-2]='.';out[n-1]='.';}
    out[n]=0;
}
