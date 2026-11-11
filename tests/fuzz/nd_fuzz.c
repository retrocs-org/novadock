/* libFuzzer targets for NovaDock's portable parsers: everything that reads
 * files, ARexx commands, plugin output or network text.  One target per
 * build, chosen with -DFUZZ_<NAME>; scripts/fuzz.sh builds and runs them:
 *
 *   sh scripts/fuzz.sh [seconds-per-target] [target ...]
 *
 * Each target also checks invariants the callers rely on (terminated,
 * bounded strings), so a silent overrun shows up as a crash. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* NUL-terminated copy of the input (at most LIMIT bytes). */
static char *text_copy(const uint8_t *data,size_t size,size_t limit)
{
    char *text;
    if (size>limit) size=limit;
    text=(char *)malloc(size+1);
    if (!text) abort();
    memcpy(text,data,size);text[size]='\0';
    return text;
}
/* Calls LINE for every line of the input, like a configuration reader. */
static void each_line(const uint8_t *data,size_t size,
                      void (*line)(const char *,void *),void *ctx)
{
    char *text=text_copy(data,size,65536),*p=text,*nl;
    while (*p) {
        nl=strchr(p,'\n');
        if (nl) *nl='\0';
        if (strlen(p)<1024) line(p,ctx);
        if (!nl) break;
        p=nl+1;
    }
    free(text);
}
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"invariant: %s\n",#x); abort(); } } while (0)

#if defined(FUZZ_CONFIG) || defined(FUZZ_MANIFEST)
#include "nd_config.h"
#include "nd_options.h"
static struct NDConfig *fuzz_cfg;
static void config_line(const char *line,void *ctx)
{
    (void)ctx;
    (void)nd_parse_line(fuzz_cfg,line);
}
#endif

#if defined(FUZZ_CONFIG)
int LLVMFuzzerTestOneInput(const uint8_t *data,size_t size)
{
    struct NDOptionDef defs[16];
    int i;
    if (!fuzz_cfg) fuzz_cfg=(struct NDConfig *)malloc(sizeof(*fuzz_cfg));
    nd_default_config(fuzz_cfg);
    each_line(data,size,config_line,NULL);
    CHECK(fuzz_cfg->count>=0 && fuzz_cfg->count<=ND_MAX_ITEMS);
    CHECK(fuzz_cfg->option_count<=ND_MAX_OPTIONS);
    CHECK(fuzz_cfg->hook_count<=ND_MAX_HOOKS);
    CHECK(nd_config_runtime_equal(fuzz_cfg,fuzz_cfg));
    for (i=0;i<fuzz_cfg->count;++i) {
        const struct NDItem *it=&fuzz_cfg->items[i];
        CHECK(memchr(it->label,'\0',sizeof(it->label)));
        CHECK(strlen(nd_item_text(fuzz_cfg,it,ND_TEXT_ICON))<ND_COMMAND_LEN);
        if (it->kind==ND_ITEM_PLUGIN)
            (void)nd_option_collect(fuzz_cfg,it->id,defs,16);
    }
    for (i=0;i<fuzz_cfg->hook_count;++i)
        CHECK(nd_hook_event_valid(fuzz_cfg->hooks[i].event));
    return 0;
}
#elif defined(FUZZ_MANIFEST)
#include "nd_manifest.h"
static void manifest_line(const char *line,void *ctx)
{
    (void)nd_manifest_line((struct NDManifest *)ctx,line);
}
int LLVMFuzzerTestOneInput(const uint8_t *data,size_t size)
{
    static struct NDManifest m;
    if (!fuzz_cfg) fuzz_cfg=(struct NDConfig *)malloc(sizeof(*fuzz_cfg));
    nd_default_config(fuzz_cfg);
    nd_manifest_init(&m,"Fuzz");
    each_line(data,size,manifest_line,&m);
    if (nd_manifest_package_ready(&m)) {
        (void)nd_manifest_merge(fuzz_cfg,&m,"Plugins/Fuzz");
        CHECK(fuzz_cfg->def_count<=ND_MAX_OPTION_DEFS);
    }
    return 0;
}
#elif defined(FUZZ_REXX)
#include "nd_interaction.h"
int LLVMFuzzerTestOneInput(const uint8_t *data,size_t size)
{
    struct NDRexxCommand cmd;
    char *text=text_copy(data,size,600),out[300];
    if (nd_parse_rexx(text,&cmd)) {
        CHECK(memchr(cmd.text,'\0',sizeof(cmd.text)));
        CHECK(memchr(cmd.id,'\0',sizeof(cmd.id)));
    }
    if (nd_quote_path(out,sizeof(out),text)) CHECK(strlen(out)<sizeof(out));
    if (nd_hook_command(out,sizeof(out),"Rexx:x.rexx","DISK_IN",text))
        CHECK(strlen(out)<sizeof(out));
    free(text);
    return 0;
}
#elif defined(FUZZ_ILBM)
#include "nd_ilbm.h"
int LLVMFuzzerTestOneInput(const uint8_t *data,size_t size)
{
    static struct NDILBM icon;
    static char path[64];
    FILE *f;
    if (!path[0]) sprintf(path,"/tmp/nd_fuzz_ilbm_%ld.iff",(long)getpid());
    f=fopen(path,"wb");
    if (!f) return 0;
    fwrite(data,1,size,f);fclose(f);
    if (nd_ilbm_load(path,&icon)) {
        CHECK(icon.width>0 && icon.width<=ND_ICON_MAX_DIM);
        CHECK(icon.height>0 && icon.height<=ND_ICON_MAX_DIM);
        CHECK(icon.colors<=ND_ICON_MAX_COLORS);
    }
    return 0;
}
#elif defined(FUZZ_BITSKIM)
#include "nd_bitskim.h"
int LLVMFuzzerTestOneInput(const uint8_t *data,size_t size)
{
    struct NDBitSkimStatus s;
    char *text=text_copy(data,size,4096),out[64];
    memset(&s,0,sizeof(s));
    (void)nd_bitskim_parse(&s,text);
    if (nd_bitskim_decode(text,out,sizeof(out))) CHECK(strlen(out)<sizeof(out));
    free(text);
    return 0;
}
#elif defined(FUZZ_CLIPBOARD)
#include "nd_clipboard.h"
int LLVMFuzzerTestOneInput(const uint8_t *data,size_t size)
{
    char text[512];
    int result;
    memset(text,'X',sizeof(text));
    result=nd_clip_decode(data,(unsigned long)size,text,sizeof(text));
    CHECK(result>=-2 && result<=1);
    CHECK(memchr(text,'\0',sizeof(text)));          /* always terminated */
    if (result!=1) CHECK(text[0]=='\0' || result==0);
    return 0;
}
#elif defined(FUZZ_BINARY)
#include "nd_ahiprefs.h"
#include "nd_scrmode.h"
int LLVMFuzzerTestOneInput(const uint8_t *data,size_t size)
{
    struct NDScreenModePreset p;
    unsigned long v;
    unsigned char *copy=(unsigned char *)malloc(size ? size : 1);
    char text[24];
    if (!copy) return 0;
    memcpy(copy,data,size);
    (void)nd_ahi_prefs_volume(copy,(unsigned long)size,&v);
    (void)nd_ahi_prefs_set_volume(copy,(unsigned long)size,0x8000UL);
    if (nd_scrmode_parse(data,(unsigned long)size,&p))
        CHECK(nd_scrmode_text(p.width,p.height,p.depth,text,sizeof(text)));
    free(copy);
    return 0;
}
#elif defined(FUZZ_SCRIPT)
#include "nd_script.h"
int LLVMFuzzerTestOneInput(const uint8_t *data,size_t size)
{
    struct NDScriptResult r;
    char *text=text_copy(data,size,2048),cmd[300];
    if (nd_script_parse(text,&r)) {
        CHECK(strlen(r.caption)<sizeof(r.caption));
        CHECK(strlen(r.value)<sizeof(r.value));
        CHECK(strlen(r.badge)<sizeof(r.badge));
        CHECK(r.meter<=100 || r.meter==ND_SCRIPT_NO_METER);
    }
    if (nd_script_command(cmd,sizeof(cmd),text,"UPDATE","SCRIPT1"))
        CHECK(strlen(cmd)<sizeof(cmd));
    free(text);
    return 0;
}
#elif defined(FUZZ_NETWORK)
#include "nd_network.h"
static void net_line(const char *line,void *ctx)
{
    (void)nd_net_parse_line((struct NDNetConfig *)ctx,line);
}
int LLVMFuzzerTestOneInput(const uint8_t *data,size_t size)
{
    struct NDNetConfig cfg;
    nd_net_defaults(&cfg);
    each_line(data,size,net_line,&cfg);
    return 0;
}
#elif defined(FUZZ_MUSIC)
#include "nd_musicplayer.h"
int LLVMFuzzerTestOneInput(const uint8_t *data,size_t size)
{
    char *text=text_copy(data,size,4096);
    int playing;
    (void)nd_music_parse_playing(text,&playing);
    free(text);
    return 0;
}
#else
#error "Define one FUZZ_<target>."
#endif
