/* BitSkim 0.1 local STATUS protocol; no sockets or file payloads in the dock. */
#include "nd_bitskim.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
void nd_bitskim_defaults(struct NDBitSkimConfig *c)
{memset(c,0,sizeof(*c));strcpy(c->program,"SYS:Utilities/BitSkim/BitSkim");strcpy(c->port,"AUTO");c->poll_ticks=50;c->badge=1;}
static int clean(const char *v,unsigned int size)
{const unsigned char *p=(const unsigned char *)v;if(!v || strlen(v)>=size)return 0;for(;*p;++p)if(*p<32 || *p==127 || *p=='|')return 0;return 1;}
int nd_bitskim_port(const char *name)
{const char *p;if(!name)return 0;if(!strcmp(name,"BITSKIM"))return 1;if(strncmp(name,"BITSKIM.",8))return 0;p=name+8;if(!*p)return 0;for(;*p;++p)if(!isdigit((unsigned char)*p))return 0;return strlen(name)<64;}
int nd_bitskim_option(void *ctx,const char *key,const char *value)
{
    struct NDBitSkimConfig *c=ctx;unsigned long n;char *end;
    if(!strcmp(key,"PROGRAM")){if(!clean(value,sizeof(c->program)))return 0;strcpy(c->program,!strcmp(value,"Bin:BitSkim/BitSkim") ? "SYS:Utilities/BitSkim/BitSkim" : value);return 1;}
    if(!strcmp(key,"PORT")){if(strcmp(value,"AUTO") && !nd_bitskim_port(value))return 0;strcpy(c->port,value);return 1;}
    if(!strcmp(key,"POLL_TICKS")){n=strtoul(value,&end,10);if(!isdigit((unsigned char)*value) || *end || n<25 || n>3000)return 0;c->poll_ticks=(unsigned int)n;return 1;}
    if(!strcmp(key,"BADGE")){if(!strcmp(value,"PEERS"))c->badge=1;else if(!strcmp(value,"STATUS"))c->badge=2;else if(!strcmp(value,"OFF"))c->badge=0;else return 0;return 1;}
    return 0;
}
int nd_bitskim_parse(struct NDBitSkimStatus *s,const char *text)
{
    struct NDBitSkimStatus next;char key[32],number[16];const char *p=text,*start;unsigned long n;unsigned int have=0;int len;
    memset(&next,0,sizeof(next));if(!text)return 0;
    while(*p){
        while(isspace((unsigned char)*p))++p;if(!*p)break;len=0;
        while(*p && *p!='=' && !isspace((unsigned char)*p)){if(len>=31)return 0;key[len++]=*p++;}key[len]=0;
        if(*p++!='=' || !isdigit((unsigned char)*p))return 0;
        start=p;while(isdigit((unsigned char)*p))++p;
        if((*p && !isspace((unsigned char)*p)) || (size_t)(p-start)>=sizeof(number))return 0;
        memcpy(number,start,(size_t)(p-start));number[p-start]=0;if(!nd_bitskim_uint(number,&n))return 0;
#define FIELD(k,member,bit,max) if(!strcmp(key,k)){if(n>(max) || (have&(bit)))return 0;next.member=n;have|=(bit);}
        FIELD("ACTIVE",active,1,1)
        else FIELD("INCOMING",incoming,2,1)
        else FIELD("AWAITING",awaiting,4,1)
        else FIELD("ICONIFIED",iconified,8,1)
        else FIELD("RECEIVING",receiving,16,1)
        else FIELD("VISIBLE",visible,32,1)
        else FIELD("PEERS",peers,64,32)
        else FIELD("BYTES",bytes,128,2147483647UL)
        else FIELD("TOTAL",total,256,2147483647UL)
        else if(!strcmp(key,"BPS")){if(next.has_bps)return 0;next.bps=n;next.has_bps=1;}
#undef FIELD
    }
    if(have!=511 || next.bytes>next.total)return 0;
    *s=next;return 1;
}
unsigned int nd_bitskim_percent(unsigned long bytes,unsigned long total)
{
    unsigned int p;if(!total)return 0;if(bytes>=total)return 100;
    for(p=1;p<=100;++p)if(bytes<(total/100)*p+((total%100)*p+99)/100)return p-1;
    return 100;
}
void nd_bitskim_display(const struct NDBitSkimStatus *s,char *value,unsigned int *meter,char *badge,int mode)
{
    *meter=65535U;badge[0]=0;
    if(s->awaiting)strcpy(value,s->incoming ? "INCOMING FILE":"WAIT ACCEPT");
    else if(s->active){*meter=nd_bitskim_percent(s->bytes,s->total);if(!s->has_bps)sprintf(value,"%s %u%%",s->incoming ? "RECV":"SEND",*meter);else if(s->bps>=1024UL)sprintf(value,"%s %u%% %luK/s",s->incoming ? "RECV":"SEND",*meter,s->bps/1024UL);else sprintf(value,"%s %u%% %luB/s",s->incoming ? "RECV":"SEND",*meter,s->bps);}
    else if(!s->receiving)strcpy(value,"RECEIVING OFF");
    else sprintf(value,"%lu DEVICE%s",s->peers,s->peers==1 ? "":"S");
    if(mode){if(s->awaiting && s->incoming)strcpy(badge,"!");else if(mode==1 && s->peers)sprintf(badge,"%lu",s->peers);else if(s->receiving)strcpy(badge,"*");}
}
/* Bounded protocol-2 token readers. Unknown fields remain forward-compatible. */
int nd_bitskim_uint(const char *s,unsigned long *value)
{unsigned long n=0;unsigned int d;if(!s || !*s)return 0;for(;*s;++s){d=(unsigned int)(*s-'0');if(d>9 || n>(4294967295UL-d)/10)return 0;n=n*10+d;}*value=n;return 1;}
int nd_bitskim_field(const char *text,const char *key,char *out,unsigned int size)
{
    const char *p=text,*start,*eq;unsigned int n,k=(unsigned int)strlen(key);int found=0;
    if(!text || !out || !size)return 0;out[0]=0;
    while(*p){while(isspace((unsigned char)*p))++p;if(!*p)break;start=p;while(*p && !isspace((unsigned char)*p))++p;
        eq=memchr(start,'=',(size_t)(p-start));if(!eq)continue;
        if((unsigned int)(eq-start)==k && !memcmp(start,key,k)){
            n=(unsigned int)(p-eq-1);if(found || n>=size)return 0;memcpy(out,eq+1,n);out[n]=0;found=1;
        }
    }
    return found;
}
int nd_bitskim_number(const char *text,const char *key,unsigned long *n)
{char value[16];return nd_bitskim_field(text,key,value,sizeof(value)) && nd_bitskim_uint(value,n);}
static int hex_digit(char c)
{if(c>='0' && c<='9')return c-'0';if(c>='a' && c<='f')return c-'a'+10;if(c>='A' && c<='F')return c-'A'+10;return -1;}
int nd_bitskim_hex(const char *s,unsigned int length)
{unsigned int i;if(!s || strlen(s)!=length)return 0;for(i=0;i<length;++i)if(hex_digit(s[i])<0)return 0;return 1;}
int nd_bitskim_decode(const char *text,char *out,unsigned int size)
{
    unsigned int n=0;int a,b;unsigned char c;if(!size)return 0;
    while(*text){c=(unsigned char)*text++;if(c=='%'){
        if(!text[0] || !text[1] || (a=hex_digit(text[0]))<0 || (b=hex_digit(text[1]))<0)return 0;
        c=(unsigned char)(a*16+b);text+=2;if(!c)return 0;
    }if(n+1>=size)return 0;out[n++]=(char)c;}
    out[n]=0;return 1;
}
void nd_bitskim_local(char *out,unsigned int size,const char *text)
{
    unsigned int n=0;unsigned char c,d;const unsigned char *p=(const unsigned char *)text;if(!size)return;
    while(*p && n+1<size){c=*p++;if(c<32 || c==127)c=' ';
        else if(c>=128){
            if((c==0xc2 || c==0xc3) && (*p&0xc0)==0x80){d=*p++;c=(unsigned char)(((c&31)<<6)|(d&63));if(c<160)c='?';}
            else {while((*p&0xc0)==0x80)++p;c='?';}
        }out[n++]=(char)c;
    }out[n]=0;
}
static int detail_text(const char *text,const char *key,char *out,unsigned int size)
{char encoded[770];return nd_bitskim_field(text,key,encoded,sizeof(encoded)) && nd_bitskim_decode(encoded,out,size);}
int nd_bitskim_transfer(struct NDBitSkimTransfer *t,const char *text)
{
    struct NDBitSkimTransfer next;unsigned long n;memset(&next,0,sizeof(next));
    if(!nd_bitskim_number(text,"AVAILABLE",&n) || n>1)return 0;
    if(!n){*t=next;return 1;}next.available=1;
    if(!nd_bitskim_field(text,"SESSION",next.session,sizeof(next.session)) || !nd_bitskim_hex(next.session,32) ||
       !nd_bitskim_field(text,"ID",next.id,sizeof(next.id)) || strlen(next.id)!=41 || next.id[32]!='-')return 0;
    {char part[33];memcpy(part,next.id,32);part[32]=0;if(!nd_bitskim_hex(part,32) || !nd_bitskim_hex(next.id+33,8))return 0;}
    if(!nd_bitskim_number(text,"ACTIVE",&n) || n>1)return 0;next.active=(int)n;
    if(!nd_bitskim_number(text,"INCOMING",&n) || n>1)return 0;next.incoming=(int)n;
    if(!nd_bitskim_number(text,"RESULT",&n) || n>4)return 0;next.result=(int)n;
    if(!nd_bitskim_number(text,"BYTES",&next.bytes) || !nd_bitskim_number(text,"TOTAL",&next.total) ||
       next.total>2147483647UL || next.bytes>next.total)return 0;
    if(!detail_text(text,"FILENAME",next.filename,sizeof(next.filename)) || !detail_text(text,"PEER",next.peer,sizeof(next.peer)) || !detail_text(text,"ERROR",next.error,sizeof(next.error)))return 0;
    *t=next;return 1;
}
int nd_bitskim_events(struct NDBitSkimTracker *t,const char *text)
{
    char session[33];unsigned long cursor,lost;int changed;
    if(!nd_bitskim_field(text,"SESSION",session,sizeof(session)) || !nd_bitskim_hex(session,32) ||
       !nd_bitskim_number(text,"LATEST",&cursor) || !nd_bitskim_number(text,"LOST",&lost) || lost>1)return -1;
    changed=strcmp(session,t->session)!=0;
    if(changed){memset(t,0,sizeof(*t));strcpy(t->session,session);t->baseline=1;}
    changed=changed || lost || cursor!=t->cursor;t->cursor=cursor;return changed;
}
void nd_bitskim_track(struct NDBitSkimTracker *t,const struct NDBitSkimTransfer *last)
{
    if(last->available && last->result){
        if(strcmp(last->id,t->id)){
            t->last=*last;strcpy(t->id,last->id);if(!t->baseline)t->unread=1;
        }
    }
    t->baseline=0;
}
int nd_bitskim_send_command(char *out,unsigned int size,const char *id,const char *path)
{
    unsigned int n=0,plen=0,extra;unsigned char c;char prefix[48];
    if(!nd_bitskim_hex(id,32) || !path || !*path)return 0;
    sprintf(prefix,"SENDID %s \"",id);if(strlen(prefix)+2>=size)return 0;strcpy(out,prefix);n=(unsigned int)strlen(out);
    while((c=(unsigned char)*path++)!=0){if(++plen>1023 || c<32 || c==127)return 0;extra=c=='"' ? 2:1;if(n+extra+1>=size || n+extra+1>1199)return 0;if(c=='"')out[n++]='"';out[n++]=(char)c;}
    out[n++]='"';out[n]=0;return n<=1199;
}
const char *nd_bitskim_result(int result,int incoming)
{return result==1 ? (incoming ? "RECEIVED":"SENT"):result==2 ? "TRANSFER FAILED":result==3 ? "CANCELLED":result==4 ? "DECLINED":"IN PROGRESS";}
