#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_weather.h"
#include "nd_weather_codes.h"
int main(void)
{
    char csv[17000],copy[17000],url[1000],latitude[20],longitude[20],temp[32];FILE *f;
    struct NDWeather w,original;size_t size,i;
    static const char geo[]="{\"results\":[{\"name\":\"Stockholm\",\"latitude\":59.32938,\"longitude\":18.06871}]}";
    f=fopen("tests/fixtures/weather.csv","rb");assert(f);size=fread(csv,1,sizeof(csv)-1,f);fclose(f);csv[size]=0;
    assert(nd_weather_parse(csv,&w));assert(w.count==5&&w.temperature==50&&w.code==3);assert(w.days[0].low==4&&w.days[0].high==51);
    assert(w.days[2].rain==84 && !strcmp(nd_weather_condition(w.days[2].code),"Rain"));
    assert(w.days[0].snow==-1 && w.days[0].rain_amount==-1); /* Old cached CSV has no snowfall amount. */
    assert(nd_weather_parse("time,temperature_2m (C),weather_code (wmo code)\n2026-11-16T01:30,0.0,71\ntime,weather_code (wmo code),temperature_2m_max (C),temperature_2m_min (C),precipitation_probability_max (%),snowfall_sum (cm)\n2026-11-16,71,2.0,-3.0,84,1.8\n",&w));
    assert(w.days[0].snow==18&&w.days[0].rain==84&&!strcmp(nd_weather_condition(w.days[0].code),"Snow"));
    assert(!strcmp(nd_weather_condition(85),"Snow showers")&&!strcmp(nd_weather_condition(66),"Freezing rain"));
    assert(nd_weather_parse("time,temperature_2m (C),weather_code (wmo code)\n2026-11-16T01:30,0.0,71\ntime,weather_code (wmo code),temperature_2m_max (C),temperature_2m_min (C),precipitation_probability_max (%),snowfall_sum (cm),rain_sum (mm),showers_sum (mm)\n2026-11-16,71,2.0,-3.0,84,1.8,2.0,0.4\n2026-11-17,61,4.0,1.0,90,0.0,2.0,0.4\n2026-11-18,0,2.0,-3.0,0,0.0,0.0,0.0\n",&w));
    assert(w.days[0].rain_amount==24 && w.days[0].snow==18);
    nd_weather_amount(&w.days[0],temp);assert(!strcmp(temp,"2.4mm+1.8cm"));
    nd_weather_amount(&w.days[1],temp);assert(!strcmp(temp,"2.4 mm rain"));
    nd_weather_amount(&w.days[2],temp);assert(!strcmp(temp,"None"));
    w.days[0].rain_amount=0;nd_weather_amount(&w.days[0],temp);assert(!strcmp(temp,"1.8 cm snow"));
    w.days[0].rain_amount=9999;w.days[0].snow=9999;nd_weather_amount(&w.days[0],temp);assert(!strcmp(temp,"1000mm+1000cm"));
    assert(nd_weather_parse(csv,&w));nd_weather_amount(&w.days[0],temp);assert(!strcmp(temp,"--"));
    original=w;strcpy(copy,csv);strcpy(strstr(copy,"5.0,3"),"nan,3\n");assert(!nd_weather_parse(copy,&w));assert(!memcmp(&w,&original,sizeof(w)));
    assert(nd_weather_geocode(geo,latitude,longitude,sizeof(latitude)));assert(!strcmp(latitude,"59.32938")&&!strcmp(longitude,"18.06871"));
    assert(!nd_weather_geocode("{\"results\":[]}",latitude,longitude,sizeof(latitude)));
    assert(!nd_weather_geocode("{\"results\":[{\"latitude\":90.01,\"longitude\":0}]}",latitude,longitude,sizeof(latitude)));
    assert(!nd_weather_geocode("{\"error\":true,\"reason\":\"latitude\"}",latitude,longitude,sizeof(latitude)));
    for(i=0;i<strlen(geo);++i){memcpy(copy,geo,i);copy[i]=0;assert(!nd_weather_geocode(copy,latitude,longitude,sizeof(latitude)));}
    assert(nd_weather_search_url("Stockholm, Sweden",url,sizeof(url)));assert(strstr(url,"Stockholm%2C%20Sweden"));
    assert(!nd_weather_search_url("bad\nlocation",url,sizeof(url)));assert(!nd_weather_search_url("Stockholm",url,10));
    assert(nd_weather_forecast_url("59.3293","18.0686",1,url,sizeof(url)));assert(strstr(url,"temperature_unit=fahrenheit")&&strstr(url,"format=csv")&&strstr(url,"snowfall_sum,rain_sum,showers_sum")&&strstr(url,"precipitation_unit=mm"));
    assert(!nd_weather_forecast_url("180","0",0,url,sizeof(url)));
    nd_weather_temperature(-4,0,temp);assert(!strcmp(temp,"-0.4 C"));nd_weather_temperature(511,1,temp);assert(!strcmp(temp,"51.1 F"));
    assert(nd_weather_key("Stockholm",0)!=nd_weather_key("Stockholm",1));
    assert(strstr(nd_weather_download_problem("https://example.com/: Unsupported scheme."),"no HTTPS support"));
    assert(strstr(nd_weather_download_problem("curl: (1) Protocol \"https\" not supported"),"no HTTPS support"));
    assert(strstr(nd_weather_download_problem("curl: (60) SSL certificate problem: expired"),"certificate check failed"));
    assert(strstr(nd_weather_download_problem("curl: (6) Could not resolve host"),"DNS lookup failed"));
    assert(strstr(nd_weather_download_problem("AmiSSL library could not be opened"),"library unavailable"));
    assert(strstr(nd_weather_download_problem("curl: (28) Connection timed out"),"timed out"));
    assert(!nd_weather_download_problem("unknown error") && !nd_weather_download_problem(NULL));
    assert(!strcmp(nd_weather_weekday("2026-11-14"),"Saturday"));
    assert(!strcmp(nd_weather_weekday("2026-11-16"),"Monday"));
    assert(!strcmp(nd_weather_weekday("1978-01-01"),"Sunday"));
    assert(!strcmp(nd_weather_weekday("2000-02-29"),"Tuesday"));
    assert(!strcmp(nd_weather_weekday("2100-03-01"),"Monday"));
    assert(!strcmp(nd_weather_weekday("2027-01-01"),"Friday"));
    assert(!strcmp(nd_weather_weekday("2100-02-29"),"--"));
    assert(!strcmp(nd_weather_weekday("2026-04-31"),"--"));
    assert(!strcmp(nd_weather_weekday(NULL),"--"));
    puts("Weather: live-format forecast, geocoding, malformed/truncated data, bounds, URL escaping and units passed.");return 0;
}
