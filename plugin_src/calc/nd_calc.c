/* Bounded recursive-descent calculator. No eval, shell or external service. */
#include "nd_calc.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
struct Parser {const char *p;unsigned int tokens,depth;const char *error;};
static double real_word(unsigned long n)
{return (double)(long)(n>>1)*2.0+(double)(long)(n&1);}
static double expression(struct Parser *p,int minimum);
static void space(struct Parser *p){while(*p->p==' ' || *p->p=='\t')++p->p;}
static double checked(struct Parser *p,double v)
{if(v!=v || v>4294967295.0 || v < -2147483648.0)p->error="Result outside 32-bit calculator range";return v;}
static int word(double v,unsigned long *out)
{if(v < -2147483648.0 || v>4294967295.0)return 0;if(v<0){long n=(long)v;if((double)n!=v)return 0;*out=((unsigned long)n)&0xffffffffUL;}else{*out=(unsigned long)v;if(real_word(*out)!=v)return 0;}return 1;}
static double primary(struct Parser *p)
{
 double v=0,scale=0.1;unsigned long n=0,w;unsigned int base=10,digits=0;int c,sign;
 space(p);if(++p->tokens>64 || ++p->depth>16){p->error="Expression is too complex";--p->depth;return 0;}
 if(*p->p=='+' || *p->p=='-' || *p->p=='~'){sign=*p->p++;v=primary(p);if(sign=='-')v=-v;else if(sign=='~'){if(!word(v,&w))p->error="Bitwise operations require integers";else v=real_word((~w)&0xffffffffUL);}--p->depth;return checked(p,v);}
 if(*p->p=='('){++p->p;v=expression(p,1);space(p);if(*p->p!=')')p->error="Missing closing parenthesis";else ++p->p;--p->depth;return v;}
 if(p->p[0]=='0' && (p->p[1]=='x' || p->p[1]=='X' || p->p[1]=='b' || p->p[1]=='B')){base=p->p[1]=='x'||p->p[1]=='X' ? 16:2;p->p+=2;}
 while((c=(unsigned char)*p->p)!=0){unsigned int d;if(c>='0' && c<='9')d=(unsigned int)(c-'0');else if(base==16 && tolower(c)>='a' && tolower(c)<='f')d=(unsigned int)(tolower(c)-'a'+10);else break;if(d>=base)break;if(n>(0xffffffffUL-d)/base){p->error="Number exceeds 32 bits";break;}n=n*base+d;++p->p;++digits;}
 v=real_word(n);
 if(base==10 && *p->p=='.'){++p->p;while(isdigit((unsigned char)*p->p)){v+=(*p->p++-'0')*scale;scale*=0.1;++digits;}}
 if(!digits)p->error="Expected a number";--p->depth;return checked(p,v);
}
static int op(struct Parser *p,int *width)
{space(p);*width=1;if(!strncmp(p->p,"<<",2)||!strncmp(p->p,">>",2)){*width=2;return 4;}switch(*p->p){case '|':return 1;case '^':return 2;case '&':return 3;case '+':case '-':return 5;case '*':case '/':case '%':return 6;}return 0;}
static double expression(struct Parser *p,int minimum)
{
 double left=primary(p),right,q;int precedence,width,c;unsigned long a,b;
 while(!p->error){precedence=op(p,&width);if(precedence<minimum || !precedence)break;c=*p->p;p->p+=width;if(++p->tokens>64){p->error="Expression is too complex";break;}right=expression(p,precedence+1);if(p->error)break;
  if(c=='+')left+=right;else if(c=='-')left-=right;else if(c=='*')left*=right;else if(c=='/'){if(right==0)p->error="Division by zero";else left/=right;}
  else if(c=='%'){if(!word(left,&a)||!word(right,&b))p->error="Remainder requires integers";else if(right==0)p->error="Division by zero";else{q=(left<0 ? -left:left)/(right<0 ? -right:right);left=(left<0 ? -1:1)*((left<0 ? -left:left)-real_word((unsigned long)q)*(right<0 ? -right:right));}}
  else if(!word(left,&a)||!word(right,&b))p->error="Bitwise operations require integers";
  else if(c=='<'||c=='>'){if(right<0 || right>31)p->error="Shift count must be 0 to 31";else left=real_word(c=='<' ? (a<<b)&0xffffffffUL:a>>b);}
  else left=real_word(c=='&' ? a&b:c=='|' ? a|b:a^b);
  checked(p,left);
 }
 return left;
}
int nd_calc_eval(const char *text,struct NDCalcResult *out)
{
 struct Parser p;unsigned long n,f,w;double value,absolute;char fraction[12];int i,at=0,started=0;
 memset(out,0,sizeof(*out));if(!text || strlen(text)>159){strcpy(out->error,"Expression is too long");return 0;}
 memset(&p,0,sizeof(p));p.p=text;value=expression(&p,1);space(&p);if(!p.error && *p.p)p.error="Unexpected character or missing operator";
 if(p.error){strcpy(out->error,p.error);return 0;}out->value=value;absolute=value<0 ? -value:value;n=(unsigned long)absolute;f=(unsigned long)((absolute-real_word(n))*1000000.0+0.5);if(f>=1000000UL){if(n==0xffffffffUL){strcpy(out->error,"Rounded result exceeds 32 bits");return 0;}++n;f=0;}
 sprintf(out->decimal,"%s%lu",value<0 && (n||f) ? "-":"",n);if(f){sprintf(fraction,".%06lu",f);for(i=(int)strlen(fraction)-1;i>0 && fraction[i]=='0';--i)fraction[i]=0;strcat(out->decimal,fraction);}
 if(!word(value,&w)){strcpy(out->hex,"Integer only");strcpy(out->binary,"Integer only");return 1;}
 sprintf(out->hex,"0x%08lx",w);out->binary[at++]='0';out->binary[at++]='b';for(i=31;i>=0;--i)if((w&(1UL<<i)) || started || !i){out->binary[at++]=(w&(1UL<<i)) ? '1':'0';started=1;}out->binary[at]=0;return 1;
}
