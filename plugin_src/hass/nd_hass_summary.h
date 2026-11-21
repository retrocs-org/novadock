#ifndef ND_HASS_SUMMARY_H
#define ND_HASS_SUMMARY_H
#include "nd_hass.h"
#define ND_HASS_SUMMARY_MAX 1024
#define ND_HASS_SUMMARY_ROWS 64
struct NDHassCounts {unsigned int entities,on,off,open,closed,unavailable,lights,contacts;};
struct NDHassRoomData {struct NDHassCounts count;char lights[ND_HASS_SUMMARY_ROWS][128],open[ND_HASS_SUMMARY_ROWS][128];unsigned int light_rows,open_rows;};
struct NDHassAggregate {struct NDHassRoomData data;char ids[ND_HASS_SUMMARY_MAX][96];unsigned long keys[ND_HASS_SUMMARY_MAX];unsigned short slots[ND_HASS_SUMMARY_MAX*2];};
void nd_hass_summary_clear(struct NDHassAggregate *s);
int nd_hass_summary_add(struct NDHassAggregate *s,const struct NDHassEntity *entity);
#endif
