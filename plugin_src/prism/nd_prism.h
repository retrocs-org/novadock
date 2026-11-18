#ifndef ND_PRISM_H
#define ND_PRISM_H
#define ND_PRISM_MAX_NETWORKS 32
#define ND_PRISM_SCAN 4
#define ND_PRISM_SAVE 5
#define ND_PRISM_OK 20
#define ND_PRISM_OPEN_FAILED 21
#define ND_PRISM_SCAN_FAILED 22
#define ND_PRISM_PROFILE_FAILED 23
#define ND_PRISM_INVALID 24
#define ND_PRISM_PARTIAL 25
struct NDPrismNetwork {char ssid[33];long signal;unsigned long channel;};
struct NDPrismScan {unsigned int count;struct NDPrismNetwork networks[ND_PRISM_MAX_NETWORKS];};
int nd_prism_credentials(const char *ssid,const char *password,int open_network);
int nd_prism_psk(const char *ssid,const char *password,unsigned char key[32],void (*yield)(void));
int nd_prism_profile(const char *old,const char *ssid,const unsigned char key[32],int open_network,char *out,unsigned int capacity);
void nd_prism_add_network(struct NDPrismScan *scan,const char *ssid,long signal,unsigned long channel);
/* Native scan never changes interface state. Save preserves other profiles. */
int nd_prism_scan(unsigned long unit,struct NDPrismScan *scan);
int nd_prism_save(const char *ssid,const char *password,int open_network);
int nd_prism_save_paths(const char *paths[2],const char *ssid,const char *password,int open_network,void (*yield)(void));
#endif
