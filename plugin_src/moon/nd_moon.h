#ifndef ND_MOON_H
#define ND_MOON_H
struct NDMoon {unsigned int frame,illumination,age100;int sunrise,sunset,rise_day,set_day,polar;char date[16];};
int nd_moon_coordinate(const char *text,int limit,long *value); /* degrees x 10000 */
int nd_moon_offset(const char *text,int *minutes);
int nd_moon_calculate(long days,unsigned long seconds,int offset,long latitude,long longitude,struct NDMoon *out);
const char *nd_moon_phase(unsigned int frame);
void nd_moon_time(int minute,int day,int polar,char out[48]);
#endif
