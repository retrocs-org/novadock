#ifndef ND_METRIC_ART_H
#define ND_METRIC_ART_H
/* Theme-role artwork on a 48-pixel grid. The host adds live text/meters;
 * no picture datatype, palette allocation or per-frame decoding is needed.
 * Inks: 0 dark, 1 case, 2 light, 3 accent, 4 secondary accent. */
typedef void (*NDMetricBox)(void *,int,int,int,int,int);
static void nd_metric_art(void *ctx,NDMetricBox box,int kind)
{
    int i;
    if(kind==1){ /* Processor package and pins. */
        for(i=10;i<40;i+=6){
            box(ctx,1,i,2,3,5);box(ctx,1,i,41,3,5);
            box(ctx,1,2,i,5,3);box(ctx,1,41,i,5,3);
        }
        box(ctx,0,6,6,36,36);box(ctx,1,7,7,34,34);
        box(ctx,2,8,8,32,2);box(ctx,0,8,13,32,24);
        box(ctx,3,11,15,3,3);box(ctx,1,11,33,26,2);
    }else if(kind==2){ /* Two-bank memory module with gold contacts. */
        box(ctx,0,3,8,42,31);box(ctx,1,4,9,40,28);
        box(ctx,2,5,10,38,2);
        for(i=6;i<43;i+=4)box(ctx,3,i,37,2,5);
        box(ctx,0,6,15,36,7);box(ctx,0,6,26,36,7);
        box(ctx,2,4,22,2,3);box(ctx,2,42,22,2,3);
    }else if(kind==4){ /* Ethernet interface with two live traffic channels. */
        box(ctx,0,3,3,42,38);box(ctx,1,4,4,40,35);
        box(ctx,2,5,5,38,2);box(ctx,0,7,10,34,23);
        box(ctx,3,8,11,3,3);box(ctx,4,8,23,3,3);
        box(ctx,0,16,39,16,7);box(ctx,1,18,39,12,5);
        for(i=19;i<30;i+=3)box(ctx,2,i,40,1,3);
        box(ctx,3,7,35,4,2);box(ctx,4,37,35,4,2);
    }else{ /* Digital clock case and illuminated display. */
        box(ctx,0,2,3,44,42);box(ctx,1,3,4,42,39);
        box(ctx,2,4,5,40,3);box(ctx,3,7,9,8,2);
        box(ctx,0,4,13,40,22);
        box(ctx,4,31,38,4,2);box(ctx,2,37,38,4,2);
        box(ctx,0,8,45,7,2);box(ctx,0,33,45,7,2);
    }
}
/* Compact 3x5 readings stay sharp at Tiny tile sizes instead of trying to
 * squeeze the Workbench font into the display. Coordinates are output pixels. */
static const unsigned char *nd_metric_glyph(char ch)
{
 static const unsigned char rows[][5]={
  {7,5,5,5,7},{2,6,2,2,7},{7,1,7,4,7},{7,1,7,1,7},{5,5,7,1,1},
  {7,4,7,1,7},{7,4,7,5,7},{7,1,1,1,1},{7,5,7,5,7},{7,5,7,1,7},
  {0,1,0,1,0},{5,1,2,4,5},{0,0,7,0,0},{4,5,6,5,5},{5,7,7,5,5},
  {7,4,5,5,7},{2,5,7,5,5},{6,5,6,4,4}};
 if(ch>='0'&&ch<='9')return rows[ch-'0'];
 switch(ch){case ':':return rows[10];case '%':return rows[11];case '-':return rows[12];case 'k':return rows[13];case 'M':return rows[14];case 'G':return rows[15];case 'A':return rows[16];case 'P':return rows[17];default:return (const unsigned char *)0;}
}
static int nd_metric_text_width(const char *text)
{int n=0;for(;*text;++text){if(!nd_metric_glyph(*text))return 0;n+=*text==':' ? 2:4;}return n ? n-1:0;}
static void nd_metric_tiny_text(void *ctx,NDMetricBox box,int ink,const char *text,int x,int y,int width)
{
    const unsigned char *rows;int w=nd_metric_text_width(text),row,col,start,columns;
    if(!w || w>width)return;
    x+=(width-w)/2;
    for(;*text;++text){
        rows=nd_metric_glyph(*text);columns=*text==':' ? 1:3;
        for(row=0;row<5;++row)for(col=0;col<columns;col=start){
            start=col+1;
            if(!(rows[row]&(1<<(columns-1-col))))continue;
            while(start<columns && (rows[row]&(1<<(columns-1-start))))++start;
            box(ctx,ink,x+col,y+row,start-col,1);
        }
        x+=columns+1;
    }
}
/* Seconds and the full value stay in the tooltip; 12-hour clocks keep A/P. */
static void nd_metric_tiny_time(const char *value,char out[8])
{int i=0,n;const char *p=value;out[0]=0;while(i<2&&p[i]>='0'&&p[i]<='9')++i;
 if(!i||p[i]!=':'||p[i+1]<'0'||p[i+1]>'9'||p[i+2]<'0'||p[i+2]>'9'){out[0]='-';out[1]='-';out[2]=0;return;}
 n=i+3;for(i=0;i<n;++i)out[i]=p[i];out[n]=0;
 for(p=value+n;*p;++p)if((*p=='A'||*p=='P')&&p[1]=='M'){out[n++]=*p;out[n]=0;break;}
}
#endif
