#ifndef ND_WEATHER_WINDOW_H
#define ND_WEATHER_WINDOW_H
#include "nd_weather.h"
struct NDWeatherView;
struct NDWeatherView *nd_weather_view_open(const char *location,int fahrenheit,const struct NDWeather *weather,int have,int stale,const char *status);
void nd_weather_view_update(struct NDWeatherView *p,int fahrenheit,const struct NDWeather *weather,int have,int stale,const char *status);
void nd_weather_view_close(struct NDWeatherView *p);
void nd_weather_view_raise(struct NDWeatherView *p);
unsigned long nd_weather_view_signals(const struct NDWeatherView *p);
int nd_weather_view_event(struct NDWeatherView *p);
#endif
