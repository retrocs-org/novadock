#ifndef ND_HASS_ROOM_NATIVE_H
#define ND_HASS_ROOM_NATIVE_H
#include "nd_client.h"
int nd_hass_room_run(struct NDClient *dock,const char *owner); /* 1 settings changed, 0 quit */
int nd_hass_room_selftest(void);
int nd_hass_room_shared_selftest(void);
#endif
