#include "nd_options.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
/* Built-in schemas are static metadata, not additional configuration files. */
static const struct NDOptionDef builtin[] = {
 {"BITSKIM","PROGRAM","BitSkim executable","FILE","","SYS:Utilities/BitSkim/BitSkim"},
 {"BITSKIM","PORT","ARexx port","TEXT","","AUTO"},
 {"BITSKIM","POLL_TICKS","Refresh ticks","UINT","25,3000","50"},
 {"BITSKIM","BADGE","Badge","CHOICE","PEERS,STATUS,OFF","PEERS"},
 {"AMIRC","PROGRAM","AmIRC executable","FILE","",""},
 {"AMIRC","PORT","ARexx port (AUTO discovers)","TEXT","","AUTO"},
 {"AMIRC","DISPLAY","Display","CHOICE","STATUS,NICK,CHANNEL,SERVER","NICK"},
 {"AMIRC","POLL_TICKS","Refresh ticks","UINT","50,3000","100"},
 {"AMIRC","ALERTS","Alert events (hook required)","CHOICE","BOTH,PRIVATE,HIGHLIGHT,OFF","BOTH"},
 {"AMIRC","CLEAR_ON_FOCUS","Clear alerts on AmIRC focus","BOOL","","YES"},
 {"CLIPBOARD","LIMIT","History entries","UINT","1,32","10"},
 {"CLIPBOARD","BADGE","Show history count","BOOL","","YES"},
 {"CLOCK","FORMAT","Clock face","CHOICE","DIGITAL,ANALOG","DIGITAL"},
 {"CLOCK","HOUR_MODE","Hour format","CHOICE","24,12","24"},
 {"CLOCK","SECONDS","Display seconds","BOOL","","NO"},
 {"CLOCK","POLL_TICKS","Refresh ticks","UINT","10,500","50"},
 {"CPU","POLL_TICKS","Refresh ticks","UINT","10,1000","50"},
 {"EYES","SIZE","Eye size","CHOICE","SMALL,NORMAL,LARGE","NORMAL"},
 {"EYES","EYE_COLOR","Eye colour","CHOICE","WHITE,BLUE,GREEN","WHITE"},
 {"EYES","PUPIL_COLOR","Pupil colour","CHOICE","DARK,BLUE,GREEN","DARK"},
 {"EYES","SENSITIVITY","Pupil movement","UINT","10,100","100"},
 {"EYES","SHOW_LABEL","Show label","BOOL","","NO"},
 {"EYES","BLINK","Occasional blink","BOOL","","YES"},
 {"EYES","POLL_TICKS","Refresh ticks","UINT","20,250","20"},
 {"RAM","POLL_TICKS","Refresh ticks","UINT","10,1000","50"},
 {"CHIPRAM","POLL_TICKS","Refresh ticks","UINT","10,1000","50"},
 {"FASTRAM","POLL_TICKS","Refresh ticks","UINT","10,1000","50"},
 {"NET","VIEW","Default view","CHOICE","METER,GRAPH","METER"},
 {"NET","STACK","Network stack","CHOICE","AUTO,ROADSHOW","AUTO"},
 {"NET","INTERFACE","Network interface","TEXT","","AUTO"},
 {"NET","DHCP_TIMEOUT","Connection timeout","UINT","10,300","60"},
 {"NET","SAMPLING_OVERRIDE","Override sampling driver","BOOL","","NO"},
 {"NET","DEVICE","Sampling driver","TEXT","",""},
 {"NET","UNIT","Sampling unit","UINT","0,255","0"},
 {"NET","SCALE_PPS","Meter scale PPS","UINT","1,1000000","250"},
 {"NET","POLL_TICKS","Refresh ticks","UINT","10,250","50"},
 {"NET","DEBUG_LOG","Debug log","BOOL","","NO"},
 {"ROADSHOW","INTERFACE","Network interface","TEXT","","eth0"},
 {"ROADSHOW","POLL_TICKS","Refresh ticks","UINT","100,3000","250"},
 {"ROADSHOW","DHCP_TIMEOUT","DHCP timeout","UINT","10,300","60"},
 {"ROADSHOW","PING_HOST","Ping host","TEXT","","1.1.1.1"},
 {"ROADSHOW","PING_COUNT","Ping packets","UINT","1,10","4"},
 {"ROADSHOW","ALLOW_SHUTDOWN","Allow shutdown","BOOL","","NO"},
 {"DOPUS4","PORT","ARexx port","TEXT","","DOPUS.1"},
 {"DOPUS4","ALT_PORT","Alternate port","TEXT","","DOPUS"},
 {"DOPUS4","RESTORE_COMMAND","Restore command","TEXT","","UNICONIFY"},
 {"DOPUS4","FRONT_COMMAND","Bring to front","TEXT","","DOPUSTOFRONT"},
 {"DOPUS4","PROGRAM","Program path","FILE","","SYS:DirOpus4/DirectoryOpus"},
 {"DOPUS4","CONFIG","Configuration file","TEXT","",""},
 {"DOPUS4","POLL_TICKS","Refresh ticks","UINT","10,500","25"},
 {"ARCHIVE","POLL_TICKS","Active refresh ticks","UINT","5,250","50"},
 /* Shared schema of every Command-output tile (Plugins/Command); each tile
  * keeps its own values under its own ID, see nd_option_schema_id(). */
 {"SCRIPT","SCRIPT","ARexx script","FILE","","NovaDock:Rexx/ScriptTile.rexx"},
 {"SCRIPT","INTERVAL","Refresh seconds","UINT","1,86400","30"},
 {"SCRIPT","TIMEOUT","Timeout seconds","UINT","1,600","30"},
 {"SCRIPT","CAPTION","Caption","TEXT","",""},
 {"SCRIPT","CLICK","Click action","CHOICE","RUN,NONE","RUN"},
 {"COMMAND","COMMAND","Command","TEXT","","Version"},
 {"COMMAND","INTERVAL","Refresh seconds","UINT","1,86400","60"},
 {"COMMAND","LINE","Output line","UINT","1,20","1"},
 {"COMMAND","METER","Meter from output","CHOICE","NONE,PERCENT","NONE"},
 {"COMMAND","CAPTION","Caption","TEXT","",""},
 {"COMMAND","CLICK","Click action","CHOICE","RERUN,NONE,COMMAND","RERUN"},
 {"COMMAND","CLICK_COMMAND","Click command","TEXT","",""},
 {"COMMAND","TIMEOUT","Timeout seconds","UINT","1,600","30"},
 {"MIXER","SAVE","Remember volume after restart","BOOL","","YES"},
 {"MIXER","POLL_TICKS","Refresh ticks","UINT","50,3000","100"},
 {"SCREENSHOT","DRAWER","Save in drawer","FILE","","RAM:"},
 {"SCREENSHOT","DELAY","Countdown seconds","UINT","0,30","2"},
 {"SCREENSHOT","SCREEN","Screen","CHOICE","FRONT,WORKBENCH","FRONT"},
 {"SCREENMODE","PRESETS","Presets drawer","FILE","","SYS:Prefs/Presets"},
 {"SCREENMODE","PROGRAM","ScreenMode program","FILE","","SYS:Prefs/ScreenMode"},
 {"MUSIC","MODE","Player","CHOICE","AUTO,EAGLEPLAYER,AMIGAAMP,HIPPOPLAYER","AUTO"},
 {"MUSIC","PREFER","Preferred player","CHOICE","EAGLEPLAYER,AMIGAAMP,HIPPOPLAYER","EAGLEPLAYER"},
 {"MUSIC","DEFAULT_APP","Default application","FILE","",""},
 {"MUSIC","POLL_TICKS","Refresh ticks","UINT","25,3000","100"},
 {"MUSIC","AUTO_QUERY","Query metadata","BOOL","","YES"},
 {"MUSIC","QUERY_TICKS","Metadata ticks","UINT","100,30000","100"},
 {"MUSIC","SCROLL_TITLE","Scroll title","BOOL","","YES"},
 {"MUSIC","SCROLL_TICKS","Scroll ticks","UINT","10,3000","25"},
 {"MUSIC","TITLE_WIDTH","Title width","UINT","8,30","24"},
 {"MUSIC","EAGLE_PORT","Eagle ARexx port","TEXT","","rexx_EP"},
 {"MUSIC","EAGLE_PORT_ALT","Eagle alternate port","TEXT","","EAGLEPLAYER"},
 {"MUSIC","EAGLE_PLAY","Eagle play cmd","TEXT","","PLAY 1"},
 {"MUSIC","EAGLE_PAUSE","Eagle pause cmd","TEXT","","PLAY 0"},
 {"MUSIC","EAGLE_STOP","Eagle stop cmd","TEXT","","STOP"},
 {"MUSIC","EAGLE_PREVIOUS","Eagle previous cmd","TEXT","","PREVMODULE"},
 {"MUSIC","EAGLE_NEXT","Eagle next cmd","TEXT","","NEXTMODULE"},
 {"MUSIC","EAGLE_QUERY_TITLE","Eagle title query","TEXT","","STATUS M FIL"},
 {"MUSIC","EAGLE_QUERY_TIME","Eagle time query","TEXT","",""},
 {"MUSIC","EAGLE_VOL_UP","Eagle volume +","TEXT","",""},
 {"MUSIC","EAGLE_VOL_DOWN","Eagle volume -","TEXT","",""},
 {"MUSIC","EAGLE_QUERY_VOLUME","Eagle volume query","TEXT","","STATUS G VOL"},
 {"MUSIC","EAGLE_SET_VOLUME","Eagle volume cmd","TEXT","","VOLUME"},
 {"MUSIC","EAGLE_QUERY_PLAYING","Eagle playing query","TEXT","",""},
 {"MUSIC","EAGLE_VOLUME_MAX","Eagle volume maximum","UINT","1,65535","255"},
 {"MUSIC","AMIGAAMP_PORT","AMP ARexx port","TEXT","","AMIGAAMP"},
 {"MUSIC","AMIGAAMP_PORT_ALT","AMP alternate port","TEXT","","AMIGAAMP.1"},
 {"MUSIC","AMIGAAMP_PLAY","AMP play cmd","TEXT","","PLAY"},
 {"MUSIC","AMIGAAMP_PAUSE","AMP pause cmd","TEXT","","PAUSE"},
 {"MUSIC","AMIGAAMP_STOP","AMP stop cmd","TEXT","","STOP"},
 {"MUSIC","AMIGAAMP_PREVIOUS","AMP previous cmd","TEXT","","PREV"},
 {"MUSIC","AMIGAAMP_NEXT","AMP next cmd","TEXT","","NEXT"},
 {"MUSIC","AMIGAAMP_QUERY_TITLE","AMP title query","TEXT","",""},
 {"MUSIC","AMIGAAMP_QUERY_TIME","AMP time query","TEXT","",""},
 {"MUSIC","AMIGAAMP_VOL_UP","AMP volume +","TEXT","",""},
 {"MUSIC","AMIGAAMP_VOL_DOWN","AMP volume -","TEXT","",""},
 {"MUSIC","AMIGAAMP_QUERY_VOLUME","AMP volume query","TEXT","",""},
 {"MUSIC","AMIGAAMP_SET_VOLUME","AMP volume cmd","TEXT","",""},
 {"MUSIC","AMIGAAMP_QUERY_PLAYING","AMP playing query","TEXT","",""},
 {"MUSIC","AMIGAAMP_VOLUME_MAX","AMP volume maximum","UINT","1,65535","100"},
 {"MUSIC","HIPPO_PORT","Hippo ARexx port","TEXT","","HIPPOPLAYER"},
 {"MUSIC","HIPPO_PORT_ALT","Hippo alternate port","TEXT","","HIPPOPLAYER"},
 {"MUSIC","HIPPO_PLAY","Hippo continue cmd","TEXT","","CONT"},
 {"MUSIC","HIPPO_PAUSE","Hippo pause cmd","TEXT","","STOP"},
 {"MUSIC","HIPPO_STOP","Hippo stop cmd","TEXT","","STOP"},
 {"MUSIC","HIPPO_PREVIOUS","Hippo previous cmd","TEXT","","CHOOSEPREV"},
 {"MUSIC","HIPPO_NEXT","Hippo next cmd","TEXT","","CHOOSENEXT"},
 {"MUSIC","HIPPO_QUERY_TITLE","Hippo title query","TEXT","","GET NAME"},
 {"MUSIC","HIPPO_QUERY_TIME","Hippo time query","TEXT","",""},
 {"MUSIC","HIPPO_VOL_UP","Hippo volume +","TEXT","",""},
 {"MUSIC","HIPPO_VOL_DOWN","Hippo volume -","TEXT","",""},
 {"MUSIC","HIPPO_QUERY_VOLUME","Hippo volume query","TEXT","","GET VOLU"},
 {"MUSIC","HIPPO_SET_VOLUME","Hippo volume cmd","TEXT","","VOLUME"},
 {"MUSIC","HIPPO_QUERY_PLAYING","Hippo playing query","TEXT","","GET PLAY"},
 {"MUSIC","HIPPO_VOLUME_MAX","Hippo volume maximum","UINT","1,65535","64"}
};
const struct NDOptionDef *nd_builtin_options(int *count)
{
    if (count) *count = (int)(sizeof(builtin)/sizeof(builtin[0]));
    return builtin;
}
int nd_option_validate(const struct NDOptionDef *def, const char *value)
{
    const char *p;
    char *end;
    unsigned long v, lo, hi;
    size_t n;
    if (!def || !value || strlen(value) >= ND_OPT_VALUE_LEN ||
        strchr(value,'\r') || strchr(value,'\n') || strchr(value,'|')) return 0;
    if (!strcmp(def->type,"BOOL")) return !strcmp(value,"YES") || !strcmp(value,"NO");
    if (!strcmp(def->type,"TEXT") || !strcmp(def->type,"FILE")) {
        for (p=value; *p; ++p) if ((unsigned char)*p < 32U) return 0;
        return 1;
    }
    if (!strcmp(def->type,"UINT")) {
        if (!*value || !isdigit((unsigned char)value[0])) return 0;
        v = strtoul(value,&end,10);
        if (*end) return 0;
        lo = strtoul(def->choices,&end,10);
        if (*end!=',') return 0;
        hi = strtoul(end+1,&end,10);
        return !*end && v>=lo && v<=hi;
    }
    if (!strcmp(def->type,"CHOICE")) {
        p=def->choices;
        while (*p) {
            n=strcspn(p,",");
            if (strlen(value)==n && !strncmp(p,value,n)) return 1;
            p+=n;
            if (*p==',') ++p;
        }
    }
    return 0;
}
int nd_option_collect(const struct NDConfig *cfg, const char *id,
                      struct NDOptionDef *out, int maximum)
{
    const struct NDOptionDef *list;
    const char *schema;
    int n=0, cnt, i;
    if (!cfg || !id || !out || maximum<=0) return 0;
    list=nd_builtin_options(&cnt);
    schema=nd_option_schema_id(cfg,id);
    for (i=0; i<cnt && n<maximum; ++i)
        if (!strcmp(list[i].id,schema)) {
            out[n]=list[i];
            /* Shared schemas are presented under the tile's own ID. */
            strncpy(out[n].id,id,sizeof(out[n].id)-1);
            out[n].id[sizeof(out[n].id)-1]='\0';
            ++n;
        }
    for (i=0; i<cfg->def_count && n<maximum; ++i)
        if (!strcmp(cfg->defs[i].id,id)) out[n++]=cfg->defs[i];
    return n;
}

int nd_option_group_settings(const char *id,struct NDOptionDef *defs,int count)
{
    int i,j,basic=0;struct NDOptionDef temp;
    if(!id || !defs || count<0)return 0;
    if(strcmp(id,"NET"))return count;
    for(i=0;i<count;++i){
        const char *key=defs[i].key;
        if(!strcmp(key,"SAMPLING_OVERRIDE") || !strcmp(key,"DEVICE") || !strcmp(key,"UNIT"))continue;
        temp=defs[i];
        for(j=i;j>basic;--j)defs[j]=defs[j-1];
        defs[basic++]=temp;
    }
    return basic;
}
