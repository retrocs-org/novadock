#ifndef ND_WARP_CONTROL_H
#define ND_WARP_CONTROL_H
#define ND_WARP_BUFFER_SIZE 428
#define ND_WARP_QUERY 1
#define ND_WARP_WIFI 2
#define ND_WARP_TURBO 3
void nd_warp_erase(void *data,unsigned int size);
int nd_warp_supported(unsigned int version,unsigned int revision);
int nd_warp_1260(const unsigned char *diagnostic,unsigned int size);
int nd_warp_wifi_valid(const char *ssid,const char *password);
int nd_warp_turbo_packet(int level,unsigned char packet[10]);
unsigned long nd_warp_frequency_khz(const unsigned char *diagnostic,unsigned int size);
/* Intuition edit op numbers from sghooks.h. Display/work buffers hold masks. */
int nd_warp_password_edit(char secret[64],char *work,int oldpos,int newpos,
                          int oldlen,int newlen,unsigned int operation);
#endif
