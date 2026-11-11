#ifndef ND_SEARCH_H
#define ND_SEARCH_H
#include <ctype.h>
#include <stddef.h>
static int nd_search_contains(const char *text,const char *query)
{
    size_t i;
    if(!text || !query)return 0;
    if(!*query)return 1;
    for(;*text;++text){
        for(i=0;query[i] && text[i] && tolower((unsigned char)text[i])==tolower((unsigned char)query[i]);++i){}
        if(!query[i])return 1;
    }
    return 0;
}
#endif
