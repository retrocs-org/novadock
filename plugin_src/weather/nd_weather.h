#ifndef ND_WEATHER_H
#define ND_WEATHER_H
#define ND_WEATHER_DAYS 5
#define ND_WEATHER_RESPONSE 16384
struct NDWeatherDay {char date[11];int code,high,low,rain,snow,rain_amount;};
struct NDWeather {char updated[20];int temperature,code;unsigned int count;struct NDWeatherDay days[ND_WEATHER_DAYS];};
/* rain is chance (%); snow is tenths of cm; rain_amount includes showers
 * in tenths of mm. An amount of -1 means unavailable in an older cache. */
void nd_weather_amount(const struct NDWeatherDay *day,char *out);
/* Temperatures are tenths of C or F, according to the requested units. */
const char *nd_weather_weekday(const char *date);
int nd_weather_parse(const char *csv,struct NDWeather *weather);
int nd_weather_geocode(const char *json,char *latitude,char *longitude,unsigned int size);
int nd_weather_search_url(const char *location,char *url,unsigned int size);
int nd_weather_forecast_url(const char *latitude,const char *longitude,int fahrenheit,char *url,unsigned int size);
void nd_weather_temperature(int tenths,int fahrenheit,char *out);
/* Recognise actionable downloader failures without displaying raw log text. */
const char *nd_weather_download_problem(const char *log);
unsigned long nd_weather_key(const char *location,int fahrenheit);
#endif
