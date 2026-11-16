#ifndef ND_GAMES_FILTER_H
#define ND_GAMES_FILTER_H
#include <stddef.h>
static int nd_games_fold(int c)
{return c>='A' && c<='Z' ? c+'a'-'A':c;}
/* Every space-separated word must occur in the title, ignoring ASCII case. */
static int nd_games_matches(const char *name,const char *filter)
{
    const char *word,*at;size_t length,i;int found;
    if(!name)return 0;if(!filter)return 1;
    while(*filter){
        while(*filter==' ' || *filter=='\t')++filter;
        word=filter;while(*filter && *filter!=' ' && *filter!='\t')++filter;
        length=(size_t)(filter-word);if(!length)break;
        found=0;
        for(at=name;*at && !found;++at){
            for(i=0;i<length && at[i] && nd_games_fold((unsigned char)at[i])==nd_games_fold((unsigned char)word[i]);++i){}
            if(i==length)found=1;
        }
        if(!found)return 0;
    }
    return 1;
}
#endif
