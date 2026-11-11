/* Regression tests for the ONE-FILE dock/plugin preference model. C89. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_config.h"
#include "nd_options.h"
#include "nd_option_runtime.h"
#include "nd_musicplayer.h"
#include "nd_roadshow.h"
#include "nd_network.h"

static void verify_at(int condition,int line)
{
    if (!condition) {
        fprintf(stderr,"Unified preference test failed at line %d\n",line);
        abort();
    }
}
#define verify(condition) verify_at((condition),__LINE__)
int main(void)
{
    struct NDConfig *old, *cfg, *readback;
    struct NDOptionDef options[64];
    struct NDNetConfig net;
    struct RDRoadshowConfig road;
    struct NDMusicConfig music;
    const char *fn="tests/unified-test.cfg";
    int n, i, enabled=0;
    char line[200];
    FILE *f;
    old=(struct NDConfig *)calloc(1,sizeof(*old));
    cfg=(struct NDConfig *)calloc(1,sizeof(*cfg));
    readback=(struct NDConfig *)calloc(1,sizeof(*readback));
    verify(old && cfg && readback);
    nd_default_config(cfg);
    verify(!strcmp(nd_option_get(cfg,"CLOCK","FORMAT"),"DIGITAL"));
    verify(!nd_option_set(cfg,"CLOCK","FORMAT","UNKNOWN"));
    verify(!nd_option_set(cfg,"NET","UNIT","-1"));
    verify(!nd_option_set(cfg,"RAM","SHOW","INVALID"));
    verify(nd_option_set(cfg,"CHIPRAM","POLL_TICKS","60"));
    verify(nd_option_set(cfg,"FASTRAM","POLL_TICKS","70"));
    verify(nd_option_set(cfg,"CLOCK","FORMAT","ANALOG"));
    verify(nd_option_set(cfg,"CLOCK","HOUR_MODE","12"));
    verify(nd_option_set(cfg,"NET","DEVICE","DEVS:Networks/a2065.device"));
    verify(nd_option_set(cfg,"NET","UNIT","1"));
    verify(nd_option_set(cfg,"NET","SCALE_PPS","200"));
    verify(nd_option_set(cfg,"NET","POLL_TICKS","50"));
    verify(nd_option_set(cfg,"ROADSHOW","INTERFACE","eth1"));
    verify(nd_option_set(cfg,"ROADSHOW","ALLOW_SHUTDOWN","NO"));
    verify(nd_option_set(cfg,"MUSIC","MODE","AMIGAAMP"));
    verify(nd_option_set(cfg,"MUSIC","PREFER","HIPPOPLAYER"));
    verify(nd_option_set(cfg,"MUSIC","DEFAULT_APP","SYS:Utilities/HippoPlayer/HippoPlayer"));
    verify(nd_option_set(cfg,"MUSIC","HIPPO_QUERY_TITLE","GET NAME"));
    verify(nd_option_set(cfg,"MUSIC","AUTO_QUERY","YES"));
    verify(nd_option_set(cfg,"DOPUS4","PORT","DOPUS.1"));
    verify(nd_option_set(cfg,"DOPUS4","RESTORE_COMMAND","UNICONIFY"));
    verify(nd_option_set(cfg,"DOPUS4","FRONT_COMMAND","DOPUSTOFRONT"));
    verify(nd_option_set(cfg,"DOPUS4","PROGRAM","SYS:DirOpus4/DirectoryOpus"));
    verify(!nd_option_set(cfg,"DOPUS4","POLL_TICKS","2"));
    verify(nd_option_set(cfg,"DOPUS4","POLL_TICKS","25"));
    verify(nd_option_define(cfg,"DEMO","SPEED","Widget speed","UINT","1,10","3"));
    verify(nd_option_define(cfg,"DEMO","STYLE","Widget style","CHOICE","SMALL,BIG","SMALL"));
    verify(nd_option_set(cfg,"DEMO","SPEED","6"));
    verify(nd_option_set(cfg,"DEMO","STYLE","BIG"));
    verify(!nd_option_set(cfg,"DEMO","SPEED","99"));
    n=nd_option_collect(cfg,"DEMO",options,64);
    verify(n==2);
    for (i=0;i<cfg->count;i++) if (cfg->items[i].kind==ND_ITEM_PLUGIN &&
        !strcmp(cfg->items[i].id,"CPU")) { cfg->items[i].enabled=0; ++enabled; }
    verify(enabled==1);
    verify(nd_write_config(cfg,fn));
    verify(nd_load_config(readback,fn)==0);
    verify(readback->count==cfg->count);
    verify(!strcmp(nd_option_get(readback,"CLOCK","FORMAT"),"ANALOG"));
    verify(!strcmp(nd_option_get(readback,"DEMO","SPEED"),"6"));
    verify(!strcmp(nd_option_get(readback,"MUSIC","MODE"),"AMIGAAMP"));
    verify(!strcmp(nd_option_get(readback,"MUSIC","PREFER"),"HIPPOPLAYER"));
    verify(!strcmp(nd_option_get(readback,"MUSIC","DEFAULT_APP"),
                   "SYS:Utilities/HippoPlayer/HippoPlayer"));
    verify(!strcmp(nd_option_get(readback,"CHIPRAM","POLL_TICKS"),"60"));
    verify(!strcmp(nd_option_get(readback,"FASTRAM","POLL_TICKS"),"70"));
    verify(!nd_options_changed(cfg,readback,"CLOCK"));
    verify(nd_option_set(readback,"CLOCK","FORMAT","DIGITAL"));
    verify(nd_options_changed(cfg,readback,"CLOCK"));
    verify(!nd_options_changed(cfg,readback,"MUSIC"));
    verify(nd_write_config(readback,fn));
    verify(nd_load_config(old,fn)==0);
    verify(!strcmp(nd_option_get(old,"DEMO","STYLE"),"BIG"));
    enabled=0;
    for (i=0;i<old->count;i++) if (old->items[i].kind==ND_ITEM_PLUGIN &&
        !strcmp(old->items[i].id,"CPU") && !old->items[i].enabled) ++enabled;
    verify(enabled==1);
    nd_net_defaults(&net);
    verify(nd_plugin_options_read(fn,"NET",nd_net_apply_option,&net)==1);
    verify(!strcmp(net.device,"DEVS:Networks/a2065.device") && net.unit==1);
    rd_config_defaults(&road);
    verify(nd_plugin_options_read(fn,"ROADSHOW",rd_apply_option,&road)==1);
    verify(!strcmp(road.interface_name,"eth1") && !road.allow_shutdown);
    nd_music_defaults(&music);
    verify(nd_plugin_options_read(fn,"MUSIC",nd_music_apply_option,&music)==1);
    verify(music.mode==ND_MUSIC_AMP && music.prefer==ND_MUSIC_HIPPO &&
           music.auto_query);
    verify(nd_load_config(readback,"novadock.cfg")==0);
    n=nd_option_collect(readback,"CLOCK",options,64);
    verify(n>=4);
    verify(!nd_option_set(readback,"CLOCK","SECONDS","MAYBE"));
    /* No split legacy plugin files get emitted by a normal save. */
    f=fopen(fn,"r");verify(f!=NULL);
    while (fgets(line,sizeof(line),f)) {
        if (!strncmp(line,"OPTION.CLOCK.FORMAT=",20)) ++enabled;
    }
    fclose(f);
    verify(enabled==2); /* one disabled plugin + one unified Clock option */
    remove(fn);
    free(readback);free(cfg);free(old);
    puts("Unified plugin options, schema, runtime adapters and persistence passed.");
    return 0;
}
