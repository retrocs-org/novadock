#include "nd_tooltypes.h"
#include <string.h>
#include <ctype.h>
int nd_tt_path(const char *input,char out[240])
{unsigned int n,i;if(!input || !strchr(input,':') || (n=(unsigned int)strlen(input))>=220 || !n)return 0;for(i=0;i<n;++i)if((unsigned char)input[i]<32 || input[i]==127)return 0;strcpy(out,input);if(n>5 && out[n-5]=='.' && tolower((unsigned char)out[n-4])=='i' && tolower((unsigned char)out[n-3])=='n' && tolower((unsigned char)out[n-2])=='f' && tolower((unsigned char)out[n-1])=='o')out[n-5]=0;return out[0] && out[strlen(out)-1]!=':' && out[strlen(out)-1]!='/';}
int nd_tt_line(const char *line)
{const unsigned char *p=(const unsigned char *)line;if(!line || !*line || strlen(line)>=ND_TT_LINE)return 0;for(;*p;++p)if(*p<32 || *p==127)return 0;return 1;}
int nd_tt_protected(const char *line)
{return line && (!strncmp(line,"*** DONOT EDIT",14) || !strncmp(line,"IM1=",4) || !strncmp(line,"IM2=",4));}
int nd_tt_preserves_art(const struct NDToolTypes *a,const struct NDToolTypes *b)
{unsigned int i,j=0;if(a->count>ND_TT_MAX || b->count>ND_TT_MAX)return 0;for(i=0;i<a->count;++i)if(nd_tt_protected(a->lines[i])){while(j<b->count && !nd_tt_protected(b->lines[j]))++j;if(j>=b->count || strcmp(a->lines[i],b->lines[j++]))return 0;}while(j<b->count)if(nd_tt_protected(b->lines[j++]))return 0;return 1;}
int nd_tt_stack(const char *text,unsigned long *stack)
{unsigned long n=0;const char *p=text;if(!p || !*p)return 0;for(;*p;++p){if(*p<'0' || *p>'9' || n>1048576UL/10)return 0;n=n*10+*p-'0';if(n>1048576UL)return 0;}if(n && n<4096)return 0;*stack=n;return 1;}
unsigned long nd_tt_hash(unsigned long hash,const unsigned char *data,unsigned int length)
{unsigned int i;for(i=0;i<length;++i)hash=((hash^data[i])*16777619UL)&0xffffffffUL;return hash;}
