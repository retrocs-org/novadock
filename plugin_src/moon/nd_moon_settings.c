#include <string.h>
#include "nd_moon.h"
int nd_moon_coordinate(const char *s,int limit,long *out)
{int negative=0,digits=0,fraction=0;long value=0,scale=1000;if(!s || !out || !*s)return 0;if(*s=='-' || *s=='+')negative=*s++=='-';while(*s>='0' && *s<='9'){if(++digits>3)return 0;value=value*10+*s++-'0';}if(!digits)return 0;value*=10000;if(*s=='.'){++s;while(*s>='0' && *s<='9'){if(++fraction>4)return 0;value+=(*s++-'0')*scale;scale/=10;}if(!fraction)return 0;}if(*s || value>(long)limit*10000)return 0;*out=negative ? -value:value;return 1;}
int nd_moon_offset(const char *s,int *minutes)
{int sign,h,m;if(!s || !minutes || strlen(s)!=6 || (s[0]!='+' && s[0]!='-') || s[3]!=':' || s[1]<'0'||s[1]>'9'||s[2]<'0'||s[2]>'9'||s[4]<'0'||s[4]>'9'||s[5]<'0'||s[5]>'9')return 0;sign=s[0]=='-' ? -1:1;h=(s[1]-'0')*10+s[2]-'0';m=(s[4]-'0')*10+s[5]-'0';if(h>14 || m>59 || (h==14 && m))return 0;*minutes=sign*(h*60+m);return 1;}
