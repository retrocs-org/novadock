/* Pooled option values: overwrite churn with compaction, shared values,
 * refusal when the pool is genuinely full, and save/load round trips. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_config.h"
#include "nd_options.h"
static struct NDConfig cfg,back;
static char expect[32][ND_OPT_VALUE_LEN];
static void value_for(char *out,int key,int round)
{
    int i,len=40+(key*7+round*13)%55;           /* 40..94 characters */
    for (i=0;i<len;++i) out[i]=(char)('a'+(key+round+i)%26);
    sprintf(out,"%02d-%04d",key,round);out[7]='-';
    out[len]='\0';
}
int main(void)
{
    char line[256],key[16];
    int i,round,compactions=0;
    unsigned int last;
    nd_default_config(&cfg);
    for (i=0;i<32;++i) {
        sprintf(line,"OPTIONDEF=POOLTEST|K%d|Key %d|TEXT||",i,i);
        assert(nd_parse_line(&cfg,line));
    }
    /* 400 rounds of 32 long values append ~1 MB in total through the bounded
     * pool: every value must still read back after each compaction. */
    last=cfg.option_used;
    for (round=0;round<400;++round)
        for (i=0;i<32;++i) {
            sprintf(key,"K%d",i);value_for(expect[i],i,round);
            assert(nd_option_set(&cfg,"POOLTEST",key,expect[i]));
            if (cfg.option_used<last) ++compactions;
            last=cfg.option_used;
            assert(!strcmp(nd_option_get(&cfg,"POOLTEST",key),expect[i]));
        }
    for (i=0;i<32;++i) {
        sprintf(key,"K%d",i);
        assert(!strcmp(nd_option_get(&cfg,"POOLTEST",key),expect[i]));
    }
    assert(compactions>10);
    assert(cfg.option_used<=ND_OPTION_POOL_SIZE);
    /* Setting a value to itself (a pointer into the pool) is safe even when
     * it forces a compaction. */
    {
        const char *self=nd_option_get(&cfg,"POOLTEST","K3");
        cfg.option_used=ND_OPTION_POOL_SIZE-2;   /* pretend the pool is full */
        assert(nd_option_set(&cfg,"POOLTEST","K3",self));
        assert(!strcmp(nd_option_get(&cfg,"POOLTEST","K3"),expect[3]));
        for (i=0;i<32;++i) {sprintf(key,"K%d",i);assert(!strcmp(nd_option_get(&cfg,"POOLTEST",key),expect[i]));}
    }
    /* Empty values take no pool space. */
    last=cfg.option_used;
    assert(nd_option_set(&cfg,"POOLTEST","K5",""));
    assert(cfg.option_used==last && !strcmp(nd_option_get(&cfg,"POOLTEST","K5"),""));
    strcpy(expect[5],"");
    /* Round trip through the file format. */
    assert(nd_write_config(&cfg,"option-pool.cfg"));
    assert(nd_load_config(&back,"option-pool.cfg")==0);
    for (i=0;i<32;++i) {
        sprintf(key,"K%d",i);
        assert(!strcmp(nd_option_get(&back,"POOLTEST",key),expect[i]));
    }
    remove("option-pool.cfg");remove("option-pool.cfg.bak");
    printf("Option pool: %d compactions over 12800 sets, self-copy, legacy-capacity storage and round trip passed.\n",compactions);
    printf("  NDConfig is now %lu bytes (options %lu + pool %d)\n",(unsigned long)sizeof(struct NDConfig),
           (unsigned long)sizeof(cfg.options),ND_OPTION_POOL_SIZE);
    return 0;
}
