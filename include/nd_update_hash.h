#ifndef ND_UPDATE_HASH_H
#define ND_UPDATE_HASH_H
#include <stdint.h>
struct NDUpdateHash {uint32_t h[8];unsigned long bytes;unsigned int used;unsigned char block[64];};
void nd_update_hash_init(struct NDUpdateHash *s);
void nd_update_hash_add(struct NDUpdateHash *s,const unsigned char *p,unsigned int n);
void nd_update_hash_end(struct NDUpdateHash *s,char digest[65]);
#endif
