/* Offline estimates: mean lunar age from NASA's 2000-01-06 18:14 UT
 * new moon; NOAA fractional-year sunrise/sunset equations. See MOON_PLUGIN.md. */
#include <string.h>
#include <stdio.h>
#include "nd_moon.h"
#define PI 3.14159265358979323846
#define TAU (2.0*PI)
static double sine(double x)
{double term,sum;int i;x-=TAU*(long)(x/TAU);if(x>PI)x-=TAU;if(x < -PI)x+=TAU;term=sum=x;for(i=1;i<=12;++i){term*=-x*x/((double)(2*i)*(2*i+1));sum+=term;}return sum;}
static double cosine(double x){return sine(x+PI/2.0);}
static double acosine(double x)
{double lo=0,hi=PI,mid;int i;for(i=0;i<32;++i){mid=(lo+hi)/2;if(cosine(mid)>x)lo=mid;else hi=mid;}return (lo+hi)/2;}
static int leap(int y){return !(y%4) && ((y%100) || !(y%400));}
const char *nd_moon_phase(unsigned int f)
{static const char *names[]={"New moon","Waxing crescent","Waxing crescent","Waxing crescent","First quarter","Waxing gibbous","Waxing gibbous","Waxing gibbous","Full moon","Waning gibbous","Waning gibbous","Waning gibbous","Last quarter","Waning crescent","Waning crescent","Waning crescent"};return names[f%16];}
static int local_minute(double value,int *day)
{int n=(int)(value>=0 ? value+0.5:value-0.5);*day=0;while(n<0){n+=1440;--*day;}while(n>=1440){n-=1440;++*day;}return n;}
int nd_moon_calculate(long days,unsigned long seconds,int offset,long latitude,long longitude,struct NDMoon *out)
{
 static const int months[]={31,28,31,30,31,30,31,31,30,31,30,31};long remain;int y=1978,m=0,doy,n;double utc,age,phase,g,eq,dec,lat,lon,den,hour,ha,noon;
 if(!out || days<0 || days>44559 || seconds>=86400 || offset < -840 || offset>840 || latitude < -900000 || latitude>900000 || longitude < -1800000 || longitude>1800000)return 0;
 memset(out,0,sizeof(*out));remain=days;while(remain>=365+leap(y)){remain-=365+leap(y);++y;}doy=(int)remain+1;while(m<11 && remain>=(n=months[m]+(m==1 && leap(y)))){remain-=n;++m;}sprintf(out->date,"%04d-%02d-%02ld",y,m+1,remain+1);
 utc=(double)days+((double)(long)seconds-(double)offset*60.0)/86400.0;
 /* NASA reference new moon, expressed as days since 1978-01-01. */
 age=(utc-8040.7597222222)/29.530588853;phase=age-(long)age;if(phase<0)phase+=1;out->age100=(unsigned int)(phase*2953.0588853+0.5);out->frame=(unsigned int)(phase*16+0.5)%16;
 out->illumination=(unsigned int)((1.0-cosine(TAU*phase))*50.0+0.5);
 g=TAU*(doy-1)/(365.0+leap(y));eq=229.18*(0.000075+0.001868*cosine(g)-0.032077*sine(g)-0.014615*cosine(2*g)-0.040849*sine(2*g));
 dec=0.006918-0.399912*cosine(g)+0.070257*sine(g)-0.006758*cosine(2*g)+0.000907*sine(2*g)-0.002697*cosine(3*g)+0.00148*sine(3*g);
 lat=(double)latitude*PI/1800000.0;lon=(double)longitude/10000.0;den=cosine(lat)*cosine(dec);
 if(den<0.00000001){out->polar=sine(lat)*sine(dec)>cosine(90.833*PI/180.0) ? 1:-1;return 1;}
 ha=(cosine(90.833*PI/180.0)-sine(lat)*sine(dec))/den;
 if(ha < -1){out->polar=1;return 1;}if(ha>1){out->polar=-1;return 1;}
 hour=acosine(ha)*180.0/PI;noon=720.0-4.0*lon-eq+offset;
 out->sunrise=local_minute(noon-4.0*hour,&out->rise_day);out->sunset=local_minute(noon+4.0*hour,&out->set_day);return 1;
}
void nd_moon_time(int minute,int day,int polar,char out[48])
{if(polar)strcpy(out,polar>0 ? "Polar day: no rise/set":"Polar night: no rise/set");else sprintf(out,"%02d:%02d%s",minute/60,minute%60,day<0 ? " (previous day)":day>0 ? " (next day)":"");}
