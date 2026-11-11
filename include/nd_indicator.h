#ifndef ND_INDICATOR_H
#define ND_INDICATOR_H
#define ND_INDICATOR_NONE 0
#define ND_INDICATOR_DOT 1
#define ND_INDICATOR_BADGE 2
int nd_indicator_mode(const char *id,int bridge_online,int running_marks,
                      unsigned int media_capabilities,const char *badge);
#endif
