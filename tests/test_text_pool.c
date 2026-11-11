/* Pooled launcher texts: a random model test against plain arrays plus the
 * edge cases (full pool, atomic multi-field sets, values that point into
 * the pool, item copies, compaction, file round trip). */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_config.h"

static struct NDConfig cfg, back;
static char model[ND_MAX_ITEMS][ND_TEXT_COUNT][ND_COMMAND_LEN];
static unsigned long seed=4242UL;
static int rnd(int n) { seed=seed*1103515245UL+12345UL; return (int)((seed>>16)%(unsigned long)n); }

static void random_text(char *out,int which)
{
    int len=rnd(4)==0 ? 0 : 1+rnd((int)nd_text_limit(which)-1),i;
    for (i=0;i<len;++i) out[i]=(char)('a'+rnd(26));
    out[len]='\0';
}
static void check_all(void)
{
    int i,k;
    for (i=0;i<cfg.count;++i)
        for (k=0;k<ND_TEXT_COUNT;++k)
            assert(!strcmp(nd_item_text(&cfg,&cfg.items[i],k),model[i][k]));
    assert(cfg.text_used<=ND_TEXT_POOL_SIZE && cfg.text_pool[0]=='\0');
}
static void model_test(void)
{
    char value[ND_COMMAND_LEN];
    int op,i,j,k,ok,refusals=0;
    memset(&cfg,0,sizeof(cfg));memset(model,0,sizeof(model));
    cfg.count=ND_MAX_ITEMS;
    for (op=0;op<200000;++op) {
        int r=rnd(100);
        i=rnd(cfg.count);
        if (r<80) {
            k=rnd(ND_TEXT_COUNT);random_text(value,k);
            ok=nd_item_set_text(&cfg,&cfg.items[i],k,value);
            if (ok) strcpy(model[i][k],value); else ++refusals;
        } else if (r<95) {
            /* Item copies inside one configuration share texts safely. */
            j=rnd(cfg.count);
            cfg.items[j]=cfg.items[i];
            memcpy(model[j],model[i],sizeof(model[j]));
        } else nd_text_pool_compact(&cfg,NULL);
        if (op%997==0) check_all();
    }
    check_all();
    printf("  model: 200000 operations, %d refused when the pool was genuinely full\n",refusals);
}
static void edge_cases(void)
{
    static const int three[3]={ND_TEXT_ARGS,ND_TEXT_WORKDIR,ND_TEXT_HOTKEY};
    const char *values[3];
    char big[ND_COMMAND_LEN];
    struct NDItem before;
    int i,k;
    /* Repeated edits of one field never exhaust the pool: old copies are
     * reclaimed by compaction. */
    memset(&cfg,0,sizeof(cfg));cfg.count=1;
    memset(big,'x',sizeof(big)-1);big[sizeof(big)-1]='\0';
    for (i=0;i<1000;++i) {
        big[0]=(char)('a'+i%26);
        assert(nd_item_set_text(&cfg,&cfg.items[0],ND_TEXT_ARGS,big));
    }
    assert(strlen(nd_item_text(&cfg,&cfg.items[0],ND_TEXT_ARGS))==ND_COMMAND_LEN-1);
    /* Over-long values are truncated to the field's former size. */
    assert(nd_item_set_text(&cfg,&cfg.items[0],ND_TEXT_HOTKEY,big));
    assert(strlen(nd_item_text(&cfg,&cfg.items[0],ND_TEXT_HOTKEY))==ND_HOTKEY_LEN-1);
    /* Fill the pool with distinct live texts, then a multi-field set that
     * cannot fit must change nothing at all. */
    memset(&cfg,0,sizeof(cfg));cfg.count=ND_MAX_ITEMS;
    for (i=0;i<ND_MAX_ITEMS;++i)
        for (k=0;k<3;++k) {
            sprintf(big,"%02d%d",i,k);memset(big+3,'y',ND_COMMAND_LEN-4);
            big[ND_COMMAND_LEN-1]='\0';
            if (!nd_item_set_text(&cfg,&cfg.items[i],k,big)) goto full;
        }
full:
    before=cfg.items[0];
    memset(big,'z',sizeof(big)-1);big[sizeof(big)-1]='\0';
    values[0]=big;values[1]=big;values[2]="control alt q";
    assert(!nd_item_set_texts(&cfg,&cfg.items[0],3,three,values));
    assert(!memcmp(&before,&cfg.items[0],sizeof(before)));
    /* A value that points into the pool survives the compaction it causes. */
    memset(&cfg,0,sizeof(cfg));cfg.count=3;
    assert(nd_item_set_text(&cfg,&cfg.items[0],ND_TEXT_ICON,"SYS:Shared/Icon"));
    for (i=0;i<200;++i) {
        big[0]=(char)('a'+i%26);
        assert(nd_item_set_text(&cfg,&cfg.items[1],ND_TEXT_ARGS,big));
    }
    assert(nd_item_set_text(&cfg,&cfg.items[2],ND_TEXT_ICON,
                            nd_item_text(&cfg,&cfg.items[0],ND_TEXT_ICON)));
    nd_text_pool_compact(&cfg,NULL);
    assert(!strcmp(nd_item_text(&cfg,&cfg.items[2],ND_TEXT_ICON),"SYS:Shared/Icon"));
    /* Changing one of two copied items leaves the other alone. */
    cfg.items[1]=cfg.items[0];
    assert(nd_item_set_text(&cfg,&cfg.items[1],ND_TEXT_ICON,"Other:Icon"));
    assert(!strcmp(nd_item_text(&cfg,&cfg.items[0],ND_TEXT_ICON),"SYS:Shared/Icon"));
}
static void round_trip(void)
{
    int i;
    nd_default_config(&cfg);
    for (i=0;i<20 && cfg.count<ND_MAX_ITEMS;++i) {
        char name[16],target[32];
        sprintf(name,"App %d",i);sprintf(target,"Work:Apps/App%d",i);
        assert(nd_add_launcher(&cfg,ND_ITEM_LAUNCH,name,target,"Icons/Tools.iff"));
        assert(nd_set_launcher_action(&cfg,cfg.count-1,ND_LAUNCH_CLI,
               "FILE=S:Startup-Sequence QUIET","Work:Apps","control alt a",1));
        assert(nd_set_launcher_match(&cfg,cfg.count-1,"AppTask","App *","APP.1"));
    }
    assert(nd_write_config(&cfg,"tests/nd_text_pool_test.cfg"));
    assert(nd_load_config(&back,"tests/nd_text_pool_test.cfg")==0);
    assert(nd_config_runtime_equal(&cfg,&back));
    remove("tests/nd_text_pool_test.cfg");
    remove("tests/nd_text_pool_test.cfg.bak");
    remove("tests/nd_text_pool_test.cfg.old");
    remove("tests/nd_text_pool_test.cfg.good"); /* last-known-good copy */
}
int main(void)
{
    model_test();
    edge_cases();
    round_trip();
    /* Formerly 92252 bytes on a 68k (4-byte int): 976 bytes of fixed text
     * per item now live in one shared pool. */
    printf("  NDConfig is now %lu bytes on this host\n",(unsigned long)sizeof(struct NDConfig));
    printf("Pooled launcher texts: model, full pool, atomic sets, copies, round trip passed.\n");
    return 0;
}
