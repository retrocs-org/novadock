#ifndef ND_GAMES_CACHE_H
#define ND_GAMES_CACHE_H
#include "nd_games_scan.h"
#define ND_GAMES_CACHE_MAX 10000UL
typedef int (*NDGamesCacheAdd)(void *,const char *,const char *);
typedef int (*NDGamesCacheNext)(void *,char *,unsigned int,char *,unsigned int);
/* Invalid/corrupt caches are rejected before replay. Caller discards replay
 * entries if its own allocation fails. No directory scan is performed. */
int nd_games_cache_read(const char *file,const char *root,NDGamesCacheAdd add,void *context);
int nd_games_cache_write(const char *file,const char *root,unsigned long count,
                          NDGamesCacheNext next,void *context);
/* Options are included in the validated cache header; changing formats
 * invalidates MUSIC indexes without affecting WHDLoad v1 caches. */
int nd_games_cache_read_options(const char *file,const char *root,unsigned long options,NDGamesCacheAdd add,void *context);
int nd_games_cache_write_options(const char *file,const char *root,unsigned long options,unsigned long count,NDGamesCacheNext next,void *context);
#endif
