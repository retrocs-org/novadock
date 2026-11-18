#include "nd_warp_control.h"
#include <string.h>
void nd_warp_erase(void *data,unsigned int size)
{volatile unsigned char *p=(volatile unsigned char *)data;while(size--)*p++=0;}
int nd_warp_supported(unsigned int version,unsigned int revision)
{return version==30 && revision==74;}
int nd_warp_1260(const unsigned char *d,unsigned int size)
{
 static const char model[]="warp1260";unsigned int i;unsigned char c;
 if(!d || size!=ND_WARP_BUFFER_SIZE)return 0;
 for(i=0;i<sizeof(model)-1;++i){c=d[i];if(c>='A' && c<='Z')c+='a'-'A';if(c!=model[i])return 0;}
 return d[i]==0;
}
static int bounded_text(const char *s,unsigned int min,unsigned int max)
{unsigned int n;if(!s)return 0;for(n=0;n<=max && s[n];++n)if((unsigned char)s[n]<32 || (unsigned char)s[n]>126)return 0;return n>=min && n<=max;}
int nd_warp_wifi_valid(const char *ssid,const char *password)
{return bounded_text(ssid,2,32) && bounded_text(password,1,63);}
int nd_warp_turbo_packet(int level,unsigned char packet[10])
{
 /* IEEE binary32 MHz values. No host floating-point ABI or FPU needed. */
 static const unsigned long presets[]={0x42480000UL,0x42853333UL,0x42960000UL,0x42aa0000UL,
  0x42b40000UL,0x42be0000UL,0x42c80000UL,0x42d20000UL};unsigned long v;
 if(!packet)return 0;memset(packet,0,10);if(level<0 || level>7)return 0;
 v=presets[level];packet[0]=(unsigned char)(v>>24);packet[1]=(unsigned char)(v>>16);
 packet[2]=(unsigned char)(v>>8);packet[3]=(unsigned char)v;packet[9]=1;return 1;
}
unsigned long nd_warp_frequency_khz(const unsigned char *d,unsigned int size)
{
 unsigned long v,n;unsigned int exponent;int shift;
 if(!d || size!=ND_WARP_BUFFER_SIZE)return 0;
 d+=382;v=((unsigned long)d[0]<<24)|((unsigned long)d[1]<<16)|((unsigned long)d[2]<<8)|d[3];
 exponent=(unsigned int)((v>>23)&255UL);
 if((v&0x80000000UL) || exponent<132 || exponent>133)return 0;
 shift=147-(int)exponent;n=((v&0x7fffffUL)|0x800000UL)*125UL;
 n=(n>>shift)+((n>>(shift-1))&1UL);return n>=33000UL && n<=120000UL ? n:0;
}
int nd_warp_password_edit(char secret[64],char *work,int oldpos,int newpos,
                          int oldlen,int newlen,unsigned int operation)
{
 int at,count,i;char c;
 if(!secret || !work || oldlen<0 || oldlen>63 || newlen<0 || newlen>63 ||
    oldpos<0 || oldpos>oldlen || newpos<0 || newpos>newlen || (int)strlen(secret)!=oldlen)return 0;
 switch(operation){
 case 8: /* insert */
  if(newlen!=oldlen+1 || newpos!=oldpos+1)return 0;
  c=work[oldpos];if((unsigned char)c<32 || (unsigned char)c>126)return 0;
  memmove(secret+oldpos+1,secret+oldpos,(unsigned int)(oldlen-oldpos+1));secret[oldpos]=c;break;
 case 7: /* replace */
  if(oldpos>=newlen || newlen!=(oldpos==oldlen ? oldlen+1:oldlen))return 0;
  c=work[oldpos];if((unsigned char)c<32 || (unsigned char)c>126)return 0;
  secret[oldpos]=c;secret[newlen]=0;break;
 case 2:case 3: /* backspace/delete, including word deletion */
  count=oldlen-newlen;at=operation==2 ? newpos:oldpos;
  if(count<0 || at<0 || at+count>oldlen || (operation==2 && at+count!=oldpos) ||
     (operation==3 && newpos!=oldpos))return 0;
  memmove(secret+at,secret+at+count,(unsigned int)(oldlen-at-count+1));
  for(i=newlen+1;i<64;++i)secret[i]=0;break;
 case 12:case 6: /* clear/undo: discard rather than recover masked text */
  nd_warp_erase(secret,64);newlen=0;break;
 case 1:case 4:case 5:if(newlen!=oldlen)return 0;break;
 default:return 0;
 }
 for(i=0;i<newlen;++i)work[i]='*';work[newlen]=0;return 1;
}
