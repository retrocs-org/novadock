/* Independent implementations of SHA-1/HMAC/PBKDF2 (FIPS 180-4, RFC 2104,
 * RFC 8018) and WirelessManager profile serialization. No third-party code. */
#include <stdint.h>
#include <string.h>
#include "nd_prism.h"
#include "../warp/nd_warp_control.h"
struct SHA1 {uint32_t state[5];unsigned int used,bytes;unsigned char block[64];};
static uint32_t rol(uint32_t x,unsigned int n){return (x<<n)|(x>>(32-n));}
static void compress(struct SHA1 *s,const unsigned char *p)
{
 uint32_t w[80],a,b,c,d,e,f,k,t;unsigned int i;
 for(i=0;i<16;++i)w[i]=((uint32_t)p[i*4]<<24)|((uint32_t)p[i*4+1]<<16)|((uint32_t)p[i*4+2]<<8)|p[i*4+3];
 for(i=16;i<80;++i)w[i]=rol(w[i-3]^w[i-8]^w[i-14]^w[i-16],1);
 a=s->state[0];b=s->state[1];c=s->state[2];d=s->state[3];e=s->state[4];
 for(i=0;i<80;++i){
  if(i<20){f=(b&c)|(~b&d);k=0x5a827999;}
  else if(i<40){f=b^c^d;k=0x6ed9eba1;}
  else if(i<60){f=(b&c)|(b&d)|(c&d);k=0x8f1bbcdc;}
  else{f=b^c^d;k=0xca62c1d6;}
  t=rol(a,5)+f+e+k+w[i];e=d;d=c;c=rol(b,30);b=a;a=t;
 }
 s->state[0]+=a;s->state[1]+=b;s->state[2]+=c;s->state[3]+=d;s->state[4]+=e;
 nd_warp_erase(w,sizeof(w));
}
static void init(struct SHA1 *s)
{memset(s,0,sizeof(*s));s->state[0]=0x67452301;s->state[1]=0xefcdab89;s->state[2]=0x98badcfe;s->state[3]=0x10325476;s->state[4]=0xc3d2e1f0;}
static void update(struct SHA1 *s,const unsigned char *p,unsigned int n)
{while(n--){s->block[s->used++]=*p++;++s->bytes;if(s->used==64){compress(s,s->block);s->used=0;}}}
static void finish(struct SHA1 *s,unsigned char digest[20])
{
 uint32_t bits=(uint32_t)s->bytes*8;unsigned int i;unsigned char b=0x80,zero=0;
 update(s,&b,1);while(s->used!=56)update(s,&zero,1);
 memset(s->block+56,0,4);s->block[60]=(unsigned char)(bits>>24);s->block[61]=(unsigned char)(bits>>16);s->block[62]=(unsigned char)(bits>>8);s->block[63]=(unsigned char)bits;compress(s,s->block);
 for(i=0;i<20;++i)digest[i]=(unsigned char)(s->state[i/4]>>(24-(i%4)*8));nd_warp_erase(s,sizeof(*s));
}
static void mac(const struct SHA1 *inner,const struct SHA1 *outer,const unsigned char *data,unsigned int n,unsigned char digest[20])
{struct SHA1 s=*inner;unsigned char tmp[20];update(&s,data,n);finish(&s,tmp);s=*outer;update(&s,tmp,20);finish(&s,digest);nd_warp_erase(tmp,sizeof(tmp));}
static int text(const char *s,unsigned int min,unsigned int max)
{unsigned int n;if(!s)return 0;for(n=0;n<=max && s[n];++n)if((unsigned char)s[n]<32 || (unsigned char)s[n]>126)return 0;return n>=min && n<=max;}
int nd_prism_credentials(const char *ssid,const char *password,int open_network)
{return text(ssid,1,32) && (open_network || text(password,8,63));}
int nd_prism_psk(const char *ssid,const char *password,unsigned char key[32],void (*yield)(void))
{
 struct SHA1 inner,outer;unsigned char pad[64],salt[36],u[20],sum[20];unsigned int i,j,block,n;
 if(!key || !nd_prism_credentials(ssid,password,0))return 0;
 n=(unsigned int)strlen(password);memset(pad,0x36,64);for(i=0;i<n;++i)pad[i]^=(unsigned char)password[i];init(&inner);update(&inner,pad,64);
 memset(pad,0x5c,64);for(i=0;i<n;++i)pad[i]^=(unsigned char)password[i];init(&outer);update(&outer,pad,64);
 n=(unsigned int)strlen(ssid);memcpy(salt,ssid,n);memset(salt+n,0,4);
 for(block=1;block<=2;++block){
  salt[n+3]=(unsigned char)block;mac(&inner,&outer,salt,n+4,u);memcpy(sum,u,20);
  for(i=1;i<4096;++i){mac(&inner,&outer,u,20,u);for(j=0;j<20;++j)sum[j]^=u[j];if(yield && !(i&63))yield();}
  memcpy(key+(block-1)*20,sum,block==1 ? 20:12);
 }
 nd_warp_erase(&inner,sizeof(inner));nd_warp_erase(&outer,sizeof(outer));nd_warp_erase(pad,sizeof(pad));nd_warp_erase(salt,sizeof(salt));nd_warp_erase(u,sizeof(u));nd_warp_erase(sum,sizeof(sum));return 1;
}
#define BEGIN "# NovaDock Wi-Fi profile begin\n"
#define END "# NovaDock Wi-Fi profile end\n"
int nd_prism_profile(const char *old,const char *ssid,const unsigned char key[32],int open_network,char *out,unsigned int capacity)
{
 const char *p,*start,*end,*extra;unsigned int n=0,i,bytes;static const char hex[]="0123456789abcdef";
 if(!old || !out || !nd_prism_credentials(ssid,"",1) || (!open_network && !key))return 0;
 p=old;start=strstr(old,BEGIN);
 if(start && start>old && start[-1]!='\n')return 0;
 if(start){end=strstr(start+sizeof(BEGIN)-1,END);if(!end)return 0;extra=strstr(start+sizeof(BEGIN)-1,BEGIN);if(extra && extra<end)return 0;
  end+=sizeof(END)-1;if(strstr(end,BEGIN) || strstr(end,END))return 0;
  bytes=(unsigned int)(start-old);if(bytes>=capacity)return 0;memcpy(out,old,bytes);n=bytes;p=end;
 }else if(strstr(old,END))return 0;
 bytes=(unsigned int)strlen(p);
 if(n+bytes+sizeof(BEGIN)+sizeof(END)+220>=capacity)return 0;
 memcpy(out+n,p,bytes);n+=bytes;if(n && out[n-1]!='\n')out[n++]='\n';
 memcpy(out+n,BEGIN,sizeof(BEGIN)-1);n+=sizeof(BEGIN)-1;
 memcpy(out+n,"network={\n    ssid=",19);n+=19;
 for(i=0;ssid[i];++i){out[n++]=hex[(unsigned char)ssid[i]>>4];out[n++]=hex[(unsigned char)ssid[i]&15];}
 if(open_network){memcpy(out+n,"\n    key_mgmt=NONE\n",19);n+=19;}
 else{memcpy(out+n,"\n    psk=",9);n+=9;for(i=0;i<32;++i){out[n++]=hex[key[i]>>4];out[n++]=hex[key[i]&15];}out[n++]='\n';}
 memcpy(out+n,"    priority=1000\n}\n",20);n+=20;
 memcpy(out+n,END,sizeof(END)-1);n+=sizeof(END)-1;out[n]=0;return 1;
}
void nd_prism_add_network(struct NDPrismScan *s,const char *ssid,long signal,unsigned long channel)
{
 unsigned int i,j;struct NDPrismNetwork entry;
 if(!s || !text(ssid,1,32) || signal<-200 || signal>0 || channel>200)return;
 for(i=0;i<s->count;++i)if(!strcmp(s->networks[i].ssid,ssid)){if(s->networks[i].signal>=signal)return;break;}
 if(i==s->count){if(s->count>=ND_PRISM_MAX_NETWORKS){i=s->count-1;if(signal<=s->networks[i].signal)return;}else ++s->count;}
 strcpy(s->networks[i].ssid,ssid);s->networks[i].signal=signal;s->networks[i].channel=channel;
 for(i=1;i<s->count;++i){entry=s->networks[i];j=i;while(j && s->networks[j-1].signal<entry.signal){s->networks[j]=s->networks[j-1];--j;}s->networks[j]=entry;}
}
