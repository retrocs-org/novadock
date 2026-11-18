#ifndef ND_HASS_NATIVE_H
#define ND_HASS_NATIVE_H
#include "nd_hass.h"
#include "service/amihass.h"
#define NDHassClient AmiHASSClient
#define NDHassMessage AmiHASSMessage
#define ND_HASS_ACTION AMIHASS_ACTION
#define ND_HASS_BUSY AMIHASS_BUSY
#define ND_HASS_FAILED AMIHASS_FAILED
#define ND_HASS_LIST AMIHASS_LIST
#define ND_HASS_DEVICES AMIHASS_DEVICES
#define ND_HASS_MAGIC AMIHASS_MAGIC
#define ND_HASS_OK AMIHASS_OK
#define ND_HASS_PAGE AMIHASS_PAGE
#define ND_HASS_PORT AMIHASS_PORT
#define ND_HASS_QUERY AMIHASS_QUERY
#define ND_HASS_RELOAD AMIHASS_RELOAD
#define ND_HASS_STALE AMIHASS_STALE
#define ND_HASS_UNAVAILABLE AMIHASS_UNAVAILABLE
#define ND_HASS_UNCONFIGURED AMIHASS_UNCONFIGURED
#define ND_HASS_UNSUPPORTED AMIHASS_UNSUPPORTED
#define ND_HASS_WAIT AMIHASS_WAIT
#define nd_hass_backend amihass_backend
#define nd_hass_client_close amihass_client_close
#define nd_hass_connection_load amihass_connection_load
#define nd_hass_connection_save amihass_connection_save
#define nd_hass_erase amihass_erase
#define nd_hass_problem amihass_problem
#define nd_hass_reply amihass_reply
#define nd_hass_send amihass_send
#define nd_hass_start amihass_start
#define nd_hass_worker amihass_worker
int nd_hass_picker_selftest(void);
int nd_hass_choose_target_area(const char *domain,char area[96],char device[96],char name[64],struct NDHassEntity *entity);
int nd_hass_choose_target(const char *domain,char device[96],char name[64],struct NDHassEntity *entity);
int nd_hass_choose_device(const char *domain,char device[96],char name[64]);
int nd_hass_choose(const char *device,const char *domain,char entity[96]);
#endif
