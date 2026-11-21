#ifndef AMIHASS_H
#define AMIHASS_H
#define AMIHASS_MAX_ENTITIES 16
#define AMIHASS_RESPONSE_LIMIT 65536UL
#define AMIHASS_ID 96
struct AmiHASSEntity {char id[96],name[64],state[40],unit[16],device_class[24],updated[40];};
struct AmiHASSConnection {char url[160];unsigned int poll;int allow_http;};
int amihass_request(const struct AmiHASSConnection *connection,const char *token,const char *path,const char *entity,char *out,unsigned int capacity);
int amihass_device_id(const char *id);
int amihass_area_valid(const char *area);
int amihass_areas_request(int offset,char *out,unsigned int capacity);
int amihass_areas_page(const char *json,struct AmiHASSEntity *items,unsigned int capacity,int *total);
int amihass_devices_area_request(const char *area,const char *domain,const char *filter,int offset,char *out,unsigned int capacity);
int amihass_device_area_page_request(const char *device,const char *area,const char *domain,const char *filter,int offset,char *out,unsigned int capacity);
int amihass_devices_domain_request(const char *domain,const char *filter,int offset,char *out,unsigned int capacity);
int amihass_devices_request(const char *filter,int offset,char *out,unsigned int capacity);
int amihass_devices_page(const char *json,struct AmiHASSEntity *items,unsigned int capacity,int *total);
int amihass_device_page_request(const char *device,const char *domain,const char *filter,int offset,char *out,unsigned int capacity);
int amihass_page_request(const char *domain,const char *filter,int offset,char *out,unsigned int capacity);
int amihass_page(const char *json,struct AmiHASSEntity *items,unsigned int capacity,int *total);
int amihass_command(int argc,char **argv);
int amihass_api_response(const char *json);
int amihass_url(const char *url,int allow_http);
int amihass_entity_id(const char *id);
int amihass_token(const char *text,char token[1025]);
int amihass_state(const char *json,struct AmiHASSEntity *entity);
int amihass_catalog(const char *json,struct AmiHASSEntity *items,unsigned int capacity);
int amihass_matches(const struct AmiHASSEntity *entity,const char *domain,const char *filter);
int amihass_action(const char *entity,const char *action,char path[96],char body[180]);
const char *amihass_icon(const struct AmiHASSEntity *entity);
const char *amihass_display_state(const struct AmiHASSEntity *entity);
int amihass_available(const struct AmiHASSEntity *entity);
/* QUERY replies include optional control metadata in items[]; ABI 1 is unchanged.
 * Numeric temperature values are hundredths of the server's native unit. */
struct AHControls {int brightness,color,temperature,level,level_known,color_known;unsigned long rgb;long target,minimum,maximum,step;char unit[16];int fan_speed,speed,speed_known,position,position_value,position_known,cover_open,cover_close,cover_stop;int media_play,media_pause,media_previous,media_next,media_volume,volume,volume_known;char media_title[64],media_artist[64];int lock,unlock,lock_open;};
int amihass_number(const char *text,long *value);
void amihass_number_text(long value,char out[40]);
void amihass_controls(const char *json,struct AHControls *controls);
int amihass_control_matches(const struct AHControls *controls,const char *state,const char *action);
int amihass_temperature_unit(const char *json,char unit[16]);
void amihass_control_unit(struct AHControls *controls,const char *unit);
int amihass_control_allowed(const struct AHControls *controls,const char *action);
unsigned int amihass_control_items(const struct AHControls *controls,struct AmiHASSEntity *items,unsigned int cap);
void amihass_controls_items(const struct AmiHASSEntity *items,unsigned int count,struct AHControls *controls);
int amihass_control_request(const struct AmiHASSConnection *connection,const char *token,const char *entity,const char *action,char *out,unsigned int capacity);
#endif
