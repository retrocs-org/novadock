#ifndef AH_UTF8_DISPLAY_H
#define AH_UTF8_DISPLAY_H
/* Strict bounded decoding; display folding never changes transport identifiers. */
static unsigned long ah_utf8_next(const unsigned char **input)
{const unsigned char *p=*input;unsigned long c=*p++,v,min;unsigned int n,i;if(c<128){*input=p;return c;}if(c>=0xc2&&c<=0xdf){n=1;v=c&31;min=128;}else if(c>=0xe0&&c<=0xef){n=2;v=c&15;min=2048;}else if(c>=0xf0&&c<=0xf4){n=3;v=c&7;min=65536;}else{*input=p;return 0xfffd;}for(i=0;i<n;++i){if(!p[i]||(p[i]&0xc0)!=0x80){*input=p;return 0xfffd;}v=(v<<6)|(p[i]&63);}if(v<min||v>0x10ffff||(v>=0xd800&&v<=0xdfff)){*input=p;return 0xfffd;}*input=p+n;return v;}
static unsigned int ah_utf8_encode(unsigned long c,char out[5])
{unsigned int n=0;if(c<128)out[n++]=(char)c;else if(c<2048){out[n++]=(char)(0xc0|(c>>6));out[n++]=(char)(0x80|(c&63));}else if(c<65536){out[n++]=(char)(0xe0|(c>>12));out[n++]=(char)(0x80|((c>>6)&63));out[n++]=(char)(0x80|(c&63));}else{out[n++]=(char)(0xf0|(c>>18));out[n++]=(char)(0x80|((c>>12)&63));out[n++]=(char)(0x80|((c>>6)&63));out[n++]=(char)(0x80|(c&63));}out[n]=0;return n;}
static const char *ah_display_glyph(unsigned long c,char out[5])
{if(c==0x20ac)return "EUR";if(c==0x2026)return "...";if(c==0x2018||c==0x2019)return "'";if(c==0x201c||c==0x201d)return "\"";if((c>=0x2010&&c<=0x2015)||c==0x2212)return "-";if(c==0x3bc)c=0xb5;if(c==0xa0)c=' ';if(c<32||c==127)c=' ';if(c>255||(c>=128&&c<160))c='?';out[0]=(char)c;out[1]=0;return out;}
#endif
