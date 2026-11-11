#include "nd_config.h"
#include "nd_options.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__amigaos__) || defined(AMIGA) || defined(__VBCC__)
#include <proto/dos.h>
#define ND_CONFIG_HAVE_PROGDIR() (GetProgramDir()!=0)
#else
#define ND_CONFIG_HAVE_PROGDIR() 1
#endif

static void upgrade_network_tiles(struct NDConfig *cfg);

static void copy_string(char *dest, const char *src, unsigned int limit)
{
    if (!limit) return;
    strncpy(dest, src, limit - 1U);
    dest[limit - 1U] = '\0';
}

/* ---- Pooled item texts ------------------------------------------------ */
static const unsigned int text_limits[ND_TEXT_COUNT]={
    ND_COMMAND_LEN,ND_COMMAND_LEN,ND_COMMAND_LEN,ND_HOTKEY_LEN,
    ND_MATCH_LEN,ND_MATCH_LEN,ND_MATCH_LEN
};
unsigned int nd_text_limit(int which)
{
    return which>=0 && which<ND_TEXT_COUNT ? text_limits[which] : 0U;
}
const char *nd_pool_text(const char *pool,unsigned short offset)
{
    if (!pool || !offset || offset>=ND_TEXT_POOL_SIZE) return "";
    return pool+offset;
}
/* Compaction state is static: a full rebuild never needs a pool-sized
 * temporary, which keeps it safe on a small plugin stack. */
#define ND_TEXT_REFS ((ND_MAX_ITEMS+1)*ND_TEXT_COUNT)
static unsigned short compact_old[ND_TEXT_REFS],compact_new[ND_TEXT_REFS];
static void compact_collect(int *n,const struct NDItem *item)
{
    int k,j;
    for (k=0;k<ND_TEXT_COUNT;++k) {
        unsigned short off=item->text[k];
        if (!off || off>=ND_TEXT_POOL_SIZE) continue;
        for (j=0;j<*n;++j) if (compact_old[j]==off) break;
        if (j==*n && *n<ND_TEXT_REFS) compact_old[(*n)++]=off;
    }
}
static void compact_remap(struct NDItem *item,int n)
{
    int k,j;
    for (k=0;k<ND_TEXT_COUNT;++k) {
        unsigned short off=item->text[k];
        if (!off) continue;
        item->text[k]=0;
        for (j=0;j<n;++j)
            if (compact_old[j]==off) {item->text[k]=compact_new[j];break;}
    }
}
void nd_text_pool_compact(struct NDConfig *cfg,struct NDItem *extra)
{
    int i,j,n=0,outside;
    unsigned int used=1,len;
    unsigned short key;
    if (!cfg) return;
    outside=extra && (extra<cfg->items || extra>=cfg->items+cfg->count);
    for (i=0;i<cfg->count;++i) compact_collect(&n,&cfg->items[i]);
    if (outside) compact_collect(&n,extra);
    /* Ascending order lets every string slide down in place: a string's
     * new position never exceeds its old one. */
    for (i=1;i<n;++i) {
        key=compact_old[i];
        for (j=i-1;j>=0 && compact_old[j]>key;--j) compact_old[j+1]=compact_old[j];
        compact_old[j+1]=key;
    }
    cfg->text_pool[0]='\0';
    for (i=0;i<n;++i) {
        len=(unsigned int)strlen(cfg->text_pool+compact_old[i])+1U;
        if (compact_old[i]!=used)
            memmove(cfg->text_pool+used,cfg->text_pool+compact_old[i],len);
        compact_new[i]=(unsigned short)used;
        used+=len;
    }
    for (i=0;i<cfg->count;++i) compact_remap(&cfg->items[i],n);
    if (outside) compact_remap(extra,n);
    cfg->text_used=used;
}
int nd_item_set_texts(struct NDConfig *cfg,struct NDItem *item,int count,
                      const int *which,const char *const *values)
{
    static char moved[ND_TEXT_COUNT][ND_COMMAND_LEN];
    const char *vals[ND_TEXT_COUNT];
    unsigned int need=0,used,len,limit;
    int i,pass;
    const char *v;
    if (!cfg || !item || count<0 || count>ND_TEXT_COUNT ||
        (count && (!which || !values))) return 0;
    for (i=0;i<count;++i) {
        if (which[i]<0 || which[i]>=ND_TEXT_COUNT) return 0;
        vals[i]=values[i] ? values[i] : "";
    }
    /* Two passes: measure (compacting once if needed), then write, so the
     * item is changed completely or not at all. */
    for (pass=0;pass<2;++pass) {
        used=cfg->text_used ? cfg->text_used : 1U;
        need=0;
        for (i=0;i<count;++i) {
            v=vals[i];
            if (!*v || !strcmp(v,nd_item_text(cfg,item,which[i]))) continue;
            len=(unsigned int)strlen(v);limit=text_limits[which[i]];
            if (len>limit-1U) len=limit-1U;
            need+=len+1U;
        }
        if (used+need<=ND_TEXT_POOL_SIZE) break;
        if (pass) return 0;
        /* A value may itself be another item's pooled text; compaction
         * moves those, so take a private copy first. */
        for (i=0;i<count;++i)
            if (vals[i]>=cfg->text_pool &&
                vals[i]<cfg->text_pool+ND_TEXT_POOL_SIZE) {
                copy_string(moved[i],vals[i],sizeof(moved[i]));
                vals[i]=moved[i];
            }
        nd_text_pool_compact(cfg,item);
    }
    for (i=0;i<count;++i) {
        v=vals[i];
        if (!*v) {item->text[which[i]]=0;continue;}
        if (!strcmp(v,nd_item_text(cfg,item,which[i]))) continue;
        len=(unsigned int)strlen(v);limit=text_limits[which[i]];
        if (len>limit-1U) len=limit-1U;
        memcpy(cfg->text_pool+used,v,len);
        cfg->text_pool[used+len]='\0';
        item->text[which[i]]=(unsigned short)used;
        used+=len+1U;
    }
    cfg->text_used=used;
    cfg->text_pool[0]='\0';
    return 1;
}
int nd_item_set_text(struct NDConfig *cfg,struct NDItem *item,int which,
                     const char *value)
{
    return nd_item_set_texts(cfg,item,1,&which,&value);
}
/* Keywords and enumerated values are accepted in any letter case so that a
 * hand-edited "theme=mocha" does not invalidate the whole file. */
static int ci_cmp(const char *a,const char *b)
{
    int ca,cb;
    do {
        ca=toupper((unsigned char)*a++);
        cb=toupper((unsigned char)*b++);
        if (ca!=cb) return ca-cb;
    } while (ca);
    return 0;
}
static const char *config_theme_id(int theme)
{
    switch (theme) {
    case ND_THEME_LIGHT: return "Light";
    case ND_THEME_WORKBENCH: return "Workbench";
    case ND_THEME_GRAPHITE: return "Graphite";
    case ND_THEME_HIGH_CONTRAST: return "HighContrast";
    case ND_THEME_COPPER: return "Copper";
    case ND_THEME_OCEAN: return "Ocean";
    case ND_THEME_EMERALD: return "Emerald";
    case ND_THEME_NEXTSTEP: return "NeXTSTEP";
    case ND_THEME_BEOS: return "BeOS";
    case ND_THEME_NORD: return "Nord";
    case ND_THEME_VAPOR: return "Vapor";
    default: return "Mocha";
    }
}
static int config_theme_index(const char *id)
{
    int i;
    if (!id) return -1;
    if (!ci_cmp(id,"HIGH_CONTRAST") || !ci_cmp(id,"HIGH CONTRAST") ||
        !ci_cmp(id,"HighContrast")) return ND_THEME_HIGH_CONTRAST;
    for (i=0;i<ND_THEME_COUNT;++i)
        if (!ci_cmp(id,config_theme_id(i)) || !ci_cmp(id,nd_theme_name(i)))
            return i;
    return -1;
}
static int config_theme_id_valid(const char *id)
{
    unsigned int n=0;
    if (!id || !*id) return 0;
    while (*id) {
        unsigned char c=(unsigned char)*id++;
        if (!isalnum(c) && c!='_' && c!='-') return 0;
        if (++n>=ND_THEME_ID_LEN) return 0;
    }
    return 1;
}
static char *trim(char *s)
{
    char *end;
    while (*s == ' ' || *s == '\t') ++s;
    end = s + strlen(s);
    while (end > s && (end[-1] == ' ' || end[-1] == '\t' ||
                        end[-1] == '\n' || end[-1] == '\r')) --end;
    *end = '\0';
    return s;
}
static int valid_id(const char *s)
{
    int n = 0;
    while (*s) {
        if (!(isalnum((unsigned char)*s) || *s == '_' || *s == '-')) return 0;
        ++s;
        ++n;
    }
    return n > 0 && n < 20;
}
static int numeric(const char *s, int lo, int hi)
{
    char *end;
    long n;
    if (!*s) return -1;
    n = strtol(s, &end, 10);
    if (*end || n < lo || n > hi) return -1;
    return (int)n;
}
const char *nd_spacer_color_name(int color)
{
    if (color==ND_SPACER_COLOR_ACCENT) return "ACCENT";
    if (color==ND_SPACER_COLOR_SECONDARY) return "SECONDARY";
    if (color==ND_SPACER_COLOR_TEXT) return "TEXT";
    return "SUBTLE";
}
const char *nd_launch_mode_name(int mode)
{
    if (mode==ND_LAUNCH_WORKBENCH) return "WORKBENCH";
    if (mode==ND_LAUNCH_CLI) return "CLI";
    if (mode==ND_LAUNCH_AREXX) return "AREXX";
    return "AUTO";
}
const char *nd_refresh_mode_name(int mode)
{
    return mode==ND_REFRESH_SMOOTH ? "SMOOTH" : "LOW_MEMORY";
}
static int launch_mode_value(const char *value)
{
    if (!ci_cmp(value,"AUTO")) return ND_LAUNCH_AUTO;
    if (!ci_cmp(value,"WORKBENCH")) return ND_LAUNCH_WORKBENCH;
    if (!ci_cmp(value,"CLI")) return ND_LAUNCH_CLI;
    if (!ci_cmp(value,"AREXX")) return ND_LAUNCH_AREXX;
    return -1;
}
static int spacer_color_value(const char *value)
{
    if (!ci_cmp(value,"SUBTLE")) return ND_SPACER_COLOR_SUBTLE;
    if (!ci_cmp(value,"ACCENT")) return ND_SPACER_COLOR_ACCENT;
    if (!ci_cmp(value,"SECONDARY")) return ND_SPACER_COLOR_SECONDARY;
    if (!ci_cmp(value,"TEXT")) return ND_SPACER_COLOR_TEXT;
    return -1;
}
static int add_item(struct NDConfig *cfg, int kind, const char *name,
                    const char *command, const char *id, const char *icon,
                    int graph, int enabled,
                    int multiple_instances)
{
    int i;
    struct NDItem *p;
    if (cfg->count >= ND_MAX_ITEMS || !*name || !*command) return 0;
    if (strlen(name) >= ND_LABEL_LEN || strlen(command) >= ND_COMMAND_LEN) return 0;
    if (icon && strlen(icon) >= ND_COMMAND_LEN) return 0;
    if (kind == ND_ITEM_PLUGIN) {
        if (!id || !valid_id(id)) return 0;
        for (i = 0; i < cfg->count; ++i)
            if (cfg->items[i].kind == ND_ITEM_PLUGIN &&
                !strcmp(cfg->items[i].id, id) &&
                !(multiple_instances && cfg->items[i].multiple_instances))
                return 0;
    }
    p = &cfg->items[cfg->count];
    memset(p, 0, sizeof(*p));
    if (icon && !nd_item_set_text(cfg, p, ND_TEXT_ICON, icon)) return 0;
    p->kind = kind;
    copy_string(p->label, name, sizeof(p->label));
    copy_string(p->caption, name, sizeof(p->caption));
    copy_string(p->command, command, sizeof(p->command));
    if (id) copy_string(p->id, id, sizeof(p->id));
    copy_string(p->value, kind == ND_ITEM_PLUGIN ? "OFFLINE" :
                ((kind == ND_ITEM_DRAWER || kind == ND_ITEM_GROUP) ? "OPEN" :
                (!strcmp(command, "@quit") ? "CLOSE" : "LAUNCH")),
                sizeof(p->value));
    p->meter = 65535U;
    /* Transfer progress is not a time-series monitor. Ignore saved graph
     * flags from older builds as well as graph requests on new items. */
    p->graph = graph && kind == ND_ITEM_PLUGIN && strcmp(p->id,"BITSKIM");
    p->enabled = enabled;
    p->multiple_instances = kind == ND_ITEM_PLUGIN && multiple_instances;
    p->launch_mode = ND_LAUNCH_AUTO;
    /* Only volume roots opt in automatically.  Ordinary drawers support the
     * same meter, but must be enabled explicitly in Preferences. */
    p->show_volume_meter = kind==ND_ITEM_DRAWER &&
                           command[strlen(command)-1]==':';
    ++cfg->count;
    return 1;
}
static int safe_field(const char *s, unsigned int limit, int optional)
{
    size_t n;
    if (!s) return optional;
    n = strlen(s);
    if ((!n && !optional) || n >= limit) return 0;
    return !strchr(s, '|') && !strchr(s, '\r') && !strchr(s, '\n');
}
int nd_item_is_volume(const struct NDItem *item)
{
    size_t n;
    if (!item || item->kind != ND_ITEM_DRAWER || !item->command[0]) return 0;
    n = strlen(item->command);
    return n > 0 && item->command[n - 1] == ':';
}
int nd_item_is_group(const struct NDItem *item)
{
    return item && item->kind==ND_ITEM_GROUP && item->command[0];
}
int nd_item_supports_space_meter(const struct NDItem *item)
{
    return item && item->kind==ND_ITEM_DRAWER && item->command[0];
}
unsigned int nd_used_percent(unsigned long used, unsigned long total)
{
    /* Keep the multiply safe on a 32-bit Amiga even for unusually large
     * volumes. The paired shifts preserve the ratio closely enough for a
     * percentage meter. */
    if (!total) return 0;
    if (used >= total) return 100;
    while (total > 40000000UL) {
        total = (total >> 1) + (total & 1UL);
        used >>= 1;
    }
    return (unsigned int)((used * 100UL) / total);
}
int nd_add_launcher(struct NDConfig *cfg, int kind, const char *name,
                    const char *target, const char *icon)
{
    if (!cfg || (kind != ND_ITEM_LAUNCH && kind != ND_ITEM_DRAWER &&
                 kind != ND_ITEM_GROUP) ||
        !safe_field(name, ND_LABEL_LEN, 0) ||
        !safe_field(target, ND_COMMAND_LEN, 0) ||
        !safe_field(icon, ND_COMMAND_LEN, 1)) return 0;
    return add_item(cfg, kind, (char *)name, (char *)target, NULL,
                    (char *)icon, 0, 1, 0);
}
int nd_add_spacer(struct NDConfig *cfg, const char *name)
{
    const char *label=(name && *name) ? name : "Spacer";
    int added;
    if (!cfg || !safe_field(label,ND_LABEL_LEN,0)) return 0;
    added=add_item(cfg,ND_ITEM_SPACER,(char *)label,"@spacer",NULL,NULL,0,1,0);
    if (added) {
        struct NDItem *item=&cfg->items[cfg->count-1];
        item->spacer_width=0;
        item->spacer_line=1;
        item->spacer_line_width=2;
        item->spacer_line_color=ND_SPACER_COLOR_SUBTLE;
    }
    return added;
}
int nd_edit_launcher(struct NDConfig *cfg, int index, int kind,
                     const char *name, const char *target, const char *icon)
{
    struct NDItem *item;
    if (!cfg || index < 0 || index >= cfg->count ||
        (cfg->items[index].kind != ND_ITEM_LAUNCH &&
         cfg->items[index].kind != ND_ITEM_DRAWER &&
         cfg->items[index].kind != ND_ITEM_GROUP) ||
        (kind != ND_ITEM_LAUNCH && kind != ND_ITEM_DRAWER &&
         kind != ND_ITEM_GROUP) ||
        !safe_field(name, ND_LABEL_LEN, 0) ||
        !safe_field(target, ND_COMMAND_LEN, 0) ||
        !safe_field(icon, ND_COMMAND_LEN, 1)) return 0;
    item = &cfg->items[index];
    if (!nd_item_set_text(cfg, item, ND_TEXT_ICON, icon ? icon : "")) return 0;
    item->kind = kind;
    copy_string(item->label, name, sizeof(item->label));
    copy_string(item->caption, name, sizeof(item->caption));
    copy_string(item->command, target, sizeof(item->command));
    copy_string(item->value, (kind == ND_ITEM_DRAWER ||
                kind == ND_ITEM_GROUP) ? "OPEN" :
                (!strcmp(target, "@quit") ? "CLOSE" : "LAUNCH"),
                sizeof(item->value));
    return 1;
}
int nd_set_launcher_action(struct NDConfig *cfg, int index, int mode,
                           const char *arguments, const char *work_directory,
                           const char *hotkey, int tools_menu)
{
    struct NDItem *item;
    if (!cfg || index<0 || index>=cfg->count ||
        (cfg->items[index].kind!=ND_ITEM_LAUNCH &&
         cfg->items[index].kind!=ND_ITEM_DRAWER &&
         cfg->items[index].kind!=ND_ITEM_GROUP) ||
        mode<0 || mode>=ND_LAUNCH_MODE_COUNT ||
        !safe_field(arguments,ND_COMMAND_LEN,1) ||
        !safe_field(work_directory,ND_COMMAND_LEN,1) ||
        !safe_field(hotkey,ND_HOTKEY_LEN,1)) return 0;
    item=&cfg->items[index];
    {
        static const int which[3]={ND_TEXT_ARGS,ND_TEXT_WORKDIR,ND_TEXT_HOTKEY};
        const char *values[3];
        values[0]=arguments;values[1]=work_directory;values[2]=hotkey;
        if (!nd_item_set_texts(cfg,item,3,which,values)) return 0;
    }
    item->launch_mode=mode;
    item->tools_menu=tools_menu ? 1 : 0;
    return 1;
}
int nd_set_launcher_match(struct NDConfig *cfg,int index,
                          const char *task_name,const char *window_pattern,
                          const char *arexx_port)
{
    struct NDItem *item;
    if (!cfg || index<0 || index>=cfg->count ||
        (cfg->items[index].kind!=ND_ITEM_LAUNCH &&
         cfg->items[index].kind!=ND_ITEM_DRAWER &&
         cfg->items[index].kind!=ND_ITEM_GROUP) ||
        !safe_field(task_name,ND_MATCH_LEN,1) ||
        !safe_field(window_pattern,ND_MATCH_LEN,1) ||
        !safe_field(arexx_port,ND_MATCH_LEN,1)) return 0;
    item=&cfg->items[index];
    {
        static const int which[3]={ND_TEXT_MATCH_TASK,ND_TEXT_MATCH_WINDOW,
                                   ND_TEXT_MATCH_PORT};
        const char *values[3];
        values[0]=task_name;values[1]=window_pattern;values[2]=arexx_port;
        return nd_item_set_texts(cfg,item,3,which,values);
    }
}
int nd_remove_launcher(struct NDConfig *cfg, int index)
{
    int i;
    if (!cfg || index < 0 || index >= cfg->count ||
        (cfg->items[index].kind != ND_ITEM_LAUNCH &&
         cfg->items[index].kind != ND_ITEM_DRAWER &&
         cfg->items[index].kind != ND_ITEM_GROUP &&
         cfg->items[index].kind != ND_ITEM_SPACER)) return 0;
    for (i = index; i + 1 < cfg->count; ++i) cfg->items[i] = cfg->items[i+1];
    --cfg->count;
    return 1;
}
/* Forget an uninstalled plugin: every definition, settings schema and
 * stored value for ID.  Refused (0) while any of its tiles is in the dock;
 * a plugin is only removed after its tiles are. */
int nd_remove_plugin(struct NDConfig *cfg, const char *id)
{
    int i, w;
    if (!cfg || !id || !*id) return 0;
    for (i = 0; i < cfg->count; ++i)
        if (cfg->items[i].kind == ND_ITEM_PLUGIN &&
            !strcmp(cfg->items[i].id, id) && cfg->items[i].enabled) return 0;
    for (i = 0, w = 0; i < cfg->count; ++i) {
        if (cfg->items[i].kind == ND_ITEM_PLUGIN && !strcmp(cfg->items[i].id, id))
            continue;
        if (w != i) cfg->items[w] = cfg->items[i];
        ++w;
    }
    cfg->count = w;
    for (i = 0, w = 0; i < cfg->def_count; ++i)
        if (strcmp(cfg->defs[i].id, id)) cfg->defs[w++] = cfg->defs[i];
    cfg->def_count = w;
    for (i = 0, w = 0; i < cfg->option_count; ++i)
        if (strcmp(cfg->options[i].id, id)) cfg->options[w++] = cfg->options[i];
    cfg->option_count = w;
    return 1;
}
/* VISIBLE positions are the layout indices after OFF plugin filtering. */
int nd_visible_to_full(const struct NDConfig *cfg, int visible)
{
    int i, n = 0;
    if (!cfg || visible < 0) return -1;
    for (i = 0; i < cfg->count; ++i) {
        if (!cfg->items[i].enabled) continue;
        if (n++ == visible) return i;
    }
    return -1;
}
int nd_move_full_item(struct NDConfig *cfg, int from, int to)
{
    struct NDItem copy;
    int i;
    if (!cfg || from < 0 || from >= cfg->count ||
        to < 0 || to >= cfg->count) return 0;
    if (from == to) return 1;
    copy = cfg->items[from];
    if (from < to) for (i = from; i < to; ++i) cfg->items[i] = cfg->items[i+1];
    else for (i = from; i > to; --i) cfg->items[i] = cfg->items[i-1];
    cfg->items[to] = copy;
    return 1;
}
static int visible_row(const struct NDConfig *cfg,int visible)
{
    int row,total=0;
    if (!cfg || !cfg->row_layout || visible<0) return -1;
    for (row=0;row<cfg->rows && row<ND_ROWS_MAX;++row) {
        total+=cfg->row_counts[row];
        if (visible<total) return row;
    }
    return cfg->rows>0 ? cfg->rows-1 : 0;
}
static void normalize_row_counts(struct NDConfig *cfg)
{
    int i,enabled=0,remaining,rows;
    if (!cfg || !cfg->row_layout) return;
    rows=cfg->rows;
    if (rows<ND_ROWS_MIN) rows=ND_ROWS_MIN;
    if (rows>ND_ROWS_MAX) rows=ND_ROWS_MAX;
    for (i=0;i<cfg->count;++i)
        if (cfg->items[i].enabled) ++enabled;
    remaining=enabled;
    for (i=0;i<rows;++i) {
        if (cfg->row_counts[i]<0) cfg->row_counts[i]=0;
        if (cfg->row_counts[i]>remaining) cfg->row_counts[i]=remaining;
        remaining-=cfg->row_counts[i];
    }
    for (i=rows;i<ND_ROWS_MAX;++i) cfg->row_counts[i]=0;
    /* Definitions added by a newer version, or enabled in Preferences,
     * remain reachable without destroying an intentionally empty row. */
    if (remaining>0) cfg->row_counts[rows-1]+=remaining;
}
int nd_move_visible_to_row(struct NDConfig *cfg,int from,int to,int row)
{
    int a=nd_visible_to_full(cfg,from),b=nd_visible_to_full(cfg,to);
    int source_row=visible_row(cfg,from);
    if (a<0 || b<0 || row<0 || row>=cfg->rows || row>=ND_ROWS_MAX)
        return 0;
    if (!nd_move_full_item(cfg,a,b)) return 0;
    if (cfg->row_layout && source_row>=0 && source_row!=row) {
        if (cfg->row_counts[source_row]>0) --cfg->row_counts[source_row];
        ++cfg->row_counts[row];
    }
    return 1;
}
int nd_move_visible(struct NDConfig *cfg, int from, int to)
{
    int row=visible_row(cfg,to);
    if (row>=0) return nd_move_visible_to_row(cfg,from,to,row);
    {
        int a=nd_visible_to_full(cfg,from),b=nd_visible_to_full(cfg,to);
        if (a<0 || b<0) return 0;
        return nd_move_full_item(cfg,a,b);
    }
}
int nd_set_item_enabled(struct NDConfig *cfg,int index,int enabled)
{
    int visible=0,row=-1,i;
    if (!cfg || index<0 || index>=cfg->count) return 0;
    enabled=enabled ? 1 : 0;
    if (cfg->items[index].enabled==enabled) return 1;
    if (cfg->row_layout) {
        normalize_row_counts(cfg);
        for (i=0;i<index;++i) if (cfg->items[i].enabled) ++visible;
        if (cfg->items[index].enabled) row=visible_row(cfg,visible);
    }
    cfg->items[index].enabled=enabled ? 1 : 0;
    if (cfg->row_layout) {
        if (!enabled && row>=0 && cfg->row_counts[row]>0)
            --cfg->row_counts[row];
        else if (enabled)
            ++cfg->row_counts[cfg->rows>0 ? cfg->rows-1 : 0];
        normalize_row_counts(cfg);
    }
    return 1;
}
int nd_plugin_allows_multiple(const struct NDItem *item)
{
    return item && item->kind==ND_ITEM_PLUGIN && item->multiple_instances;
}
int nd_add_plugin_instance(struct NDConfig *cfg,int index)
{
    struct NDItem *item;
    if (!cfg || index<0 || index>=cfg->count || cfg->count>=ND_MAX_ITEMS ||
        !nd_plugin_allows_multiple(&cfg->items[index])) return 0;
    item=&cfg->items[cfg->count];
    *item=cfg->items[index];
    copy_string(item->caption,item->label,sizeof(item->caption));
    copy_string(item->value,"OFFLINE",sizeof(item->value));
    item->meter=65535U;
    item->clicks=0;
    item->media_capabilities=0;
    item->media_state=0;
    item->media_volume=65535U;
    item->media_event=0;
    item->media_volume_request=65535U;
    item->online=0;
    item->settings_dirty=0;
    item->enabled=1;
    item->transient=ND_TRANSIENT_NONE;
    ++cfg->count;
    if (cfg->row_layout) {
        ++cfg->row_counts[cfg->rows>0 ? cfg->rows-1 : 0];
        normalize_row_counts(cfg);
    }
    return 1;
}
int nd_remove_visible_launcher(struct NDConfig *cfg, int visible)
{
    return nd_remove_launcher(cfg, nd_visible_to_full(cfg, visible));
}
int nd_remove_visible_item(struct NDConfig *cfg, int visible)
{
    int row = nd_visible_to_full(cfg, visible);
    int layout_row=visible_row(cfg,visible);
    if (!cfg || row < 0) return 0;
    /* Removing from the dock keeps the definition available in Preferences. */
    cfg->items[row].enabled = 0;
    if (layout_row>=0 && cfg->row_counts[layout_row]>0)
        --cfg->row_counts[layout_row];
    return 1;
}
int nd_insert_launcher_visible(struct NDConfig *cfg, int visible, int kind,
                               const char *name, const char *target, const char *icon)
{
    int i, before, last,reuse=-1,recycle=-1,target_row=-1;
    struct NDItem *item;
    if (!cfg || visible < 0 ||
        (kind!=ND_ITEM_LAUNCH && kind!=ND_ITEM_DRAWER &&
         kind!=ND_ITEM_GROUP) ||
        !safe_field(name,ND_LABEL_LEN,0) ||
        !safe_field(target,ND_COMMAND_LEN,0) ||
        !safe_field(icon,ND_COMMAND_LEN,1)) return 0;
    /* Dropping beyond the right/bottom edge appends after the final item. */
    before = nd_visible_to_full(cfg, visible);
    if (before < 0) {
        int n=0;
        for (i=0; i<cfg->count; ++i) if (cfg->items[i].enabled) ++n;
        if (visible != n) return 0;
        before = cfg->count;
    }
    target_row=visible_row(cfg,visible);
    /* Removing a launcher keeps it in Preferences' Available catalogue.
     * A later Workbench drop of the same object must restore that definition
     * instead of consuming another slot (and eventually reporting a false
     * "dock limit" while only a few tiles are visible). Preserve its custom
     * label, launch mode and matching rules; just enable and reposition it. */
    for (i=0;i<cfg->count;++i) {
        const struct NDItem *item=&cfg->items[i];
        if (!item->enabled &&
            (item->kind==ND_ITEM_LAUNCH || item->kind==ND_ITEM_DRAWER ||
             item->kind==ND_ITEM_GROUP) &&
            item->kind==kind && !ci_cmp(item->command,target)) {
            reuse=i;
            break;
        }
    }
    if (reuse>=0) {
        cfg->items[reuse].enabled=1;
        if (reuse<before) --before;
        if (before<0) before=0;
        if (before>=cfg->count) before=cfg->count-1;
        if (!nd_move_full_item(cfg,reuse,before)) return 0;
        if (cfg->row_layout && target_row>=0) {
            ++cfg->row_counts[target_row];
            normalize_row_counts(cfg);
        }
        return 1;
    }
    /* The Preferences catalogue deliberately retains removed applications.
     * Once all 24 definition slots are occupied, that must not make an
     * otherwise spacious multi-row dock reject a new Workbench drop. Recycle
     * one disabled launcher/drawer/group definition, but never a plugin or
     * spacer. */
    if (cfg->count>=ND_MAX_ITEMS) {
        for (i=cfg->count-1;i>=0;--i)
            if (!cfg->items[i].enabled &&
                (cfg->items[i].kind==ND_ITEM_LAUNCH ||
                 cfg->items[i].kind==ND_ITEM_DRAWER ||
                 cfg->items[i].kind==ND_ITEM_GROUP)) {
                recycle=i;
                break;
            }
        if (recycle<0) return 0;
        item=&cfg->items[recycle];
        {
            /* Build in a scratch slot first: the recycled definition keeps
             * its old texts until the new icon is safely in the pool. */
            struct NDItem fresh;
            memset(&fresh,0,sizeof(fresh));
            if (!nd_item_set_text(cfg,&fresh,ND_TEXT_ICON,icon ? icon : ""))
                return 0;
            *item=fresh;
        }
        item->kind=kind;
        copy_string(item->label,name,sizeof(item->label));
        copy_string(item->caption,name,sizeof(item->caption));
        copy_string(item->command,target,sizeof(item->command));
        copy_string(item->value,(kind==ND_ITEM_DRAWER ||
                    kind==ND_ITEM_GROUP) ? "OPEN" : "LAUNCH",
                    sizeof(item->value));
        item->meter=65535U;
        item->enabled=1;
        item->launch_mode=ND_LAUNCH_AUTO;
        item->show_volume_meter=kind==ND_ITEM_DRAWER &&
                                target[strlen(target)-1]==':';
        if (recycle<before) --before;
        if (before<0) before=0;
        if (before>=cfg->count) before=cfg->count-1;
        if (!nd_move_full_item(cfg,recycle,before)) return 0;
        if (cfg->row_layout && target_row>=0) {
            ++cfg->row_counts[target_row];
            normalize_row_counts(cfg);
        }
        return 1;
    }
    if (!nd_add_launcher(cfg, kind, name, target, icon)) return 0;
    last = cfg->count - 1;
    if (!nd_move_full_item(cfg, last, before)) return 0;
    if (cfg->row_layout && target_row>=0) {
        ++cfg->row_counts[target_row];
        normalize_row_counts(cfg);
    }
    return 1;
}
int nd_insert_spacer_visible(struct NDConfig *cfg,int visible,const char *name)
{
    int i,before,last,target_row;
    if (!cfg || visible<0) return 0;
    before=nd_visible_to_full(cfg,visible);
    if (before<0) {
        int n=0;
        for (i=0;i<cfg->count;++i) if (cfg->items[i].enabled) ++n;
        if (visible!=n) return 0;
        before=cfg->count;
    }
    target_row=visible_row(cfg,visible);
    if (!nd_add_spacer(cfg,name)) return 0;
    last=cfg->count-1;
    if (!nd_move_full_item(cfg,last,before)) return 0;
    if (cfg->row_layout && target_row>=0) {
        ++cfg->row_counts[target_row];
        normalize_row_counts(cfg);
    }
    return 1;
}
void nd_label_from_path(const char *path, char *label, unsigned int size)
{
    const char *p, *last;
    size_t n;
    if (!label || !size) return;
    label[0] = '\0';
    if (!path) return;
    last = path;
    for (p = path; *p; ++p)
        if ((*p == '/' || *p == ':') && p[1]) last = p + 1;
    n = strlen(last);
    /* Volume root names such as Work: should display as Work. */
    if (n && last[n - 1] == ':') --n;
    if (n > 5 && !strncmp(last + n - 5, ".info", 5)) n -= 5;
    if (n >= size) n = size - 1;
    memcpy(label, last, n);
    label[n] = '\0';
}
static int opt_token(const char *p, unsigned int max)
{
    unsigned int n=0;
    while (*p) {
        if (!(isalnum((unsigned char)*p) || *p=='_' || *p=='-')) return 0;
        ++p; ++n;
    }
    return n>0 && n<max;
}
/* Events the dock can run a HOOK command for (see docs/EVENT_HOOKS.md). */
static const char *const hook_events[]={"STARTUP","QUIT","DISK_IN","DISK_OUT",
    "NET_ONLINE","NET_OFFLINE","PLUGIN_STALE","SAVE_FAILED",NULL};
int nd_hook_event_valid(const char *event)
{
    int i;
    if (!event) return 0;
    for (i=0;hook_events[i];++i) if (!strcmp(hook_events[i],event)) return 1;
    return 0;
}
int nd_script_plugin_path(const char *command)
{
    const char *base,*p;
    if (!command) return 0;
    base=command;
    for (p=command;*p;++p) if (*p==':' || *p=='/') base=p+1;
    return !strcmp(base,"Script");
}
int nd_command_plugin_path(const char *command)
{
    const char *base,*p;
    if (!command) return 0;
    base=command;
    for (p=command;*p;++p) if (*p==':' || *p=='/') base=p+1;
    return !strcmp(base,"Command");
}
const char *nd_option_schema_id(const struct NDConfig *cfg,const char *id)
{
    int i;
    if (!cfg || !id) return id;
    for (i=0;i<cfg->count;++i)
        if (cfg->items[i].kind==ND_ITEM_PLUGIN && !strcmp(cfg->items[i].id,id))
            return nd_command_plugin_path(cfg->items[i].command) ?
                   ND_COMMAND_SCHEMA :
                   nd_script_plugin_path(cfg->items[i].command) ?
                   ND_SCRIPT_SCHEMA : id;
    return id;
}
const char *nd_option_get(const struct NDConfig *cfg, const char *id, const char *key)
{
    const struct NDOptionDef *built;
    const char *schema;
    int i, count;
    if (!cfg || !id || !key) return NULL;
    for (i=0; i<cfg->option_count; ++i)
        if (!strcmp(cfg->options[i].id,id) && !strcmp(cfg->options[i].key,key))
            return cfg->options[i].value;
    /* Legacy Network tiles store their view on the PLUGIN line. Reflect
     * that choice until an explicit VIEW preference is saved. */
    if(!strcmp(id,"NET") && !strcmp(key,"VIEW"))
        for(i=0;i<cfg->count;++i)if(cfg->items[i].kind==ND_ITEM_PLUGIN &&
            !strcmp(cfg->items[i].id,"NET"))return cfg->items[i].graph ? "GRAPH":"METER";
    built=nd_builtin_options(&count);
    schema=nd_option_schema_id(cfg,id);
    for (i=0; i<count; ++i)
        if (!strcmp(built[i].id,schema) && !strcmp(built[i].key,key))
            return built[i].default_value;
    for (i=0; i<cfg->def_count; ++i)
        if (!strcmp(cfg->defs[i].id,id) && !strcmp(cfg->defs[i].key,key))
            return cfg->defs[i].default_value;
    return NULL;
}
static void apply_network_view(struct NDConfig *cfg,const char *value)
{
    int i;
    for(i=0;i<cfg->count;++i)if(cfg->items[i].kind==ND_ITEM_PLUGIN &&
        !strcmp(cfg->items[i].id,"NET"))cfg->items[i].graph=!strcmp(value,"GRAPH");
}
static void apply_saved_network_view(struct NDConfig *cfg)
{
    int i;
    for(i=0;i<cfg->option_count;++i)if(!strcmp(cfg->options[i].id,"NET") &&
        !strcmp(cfg->options[i].key,"VIEW"))apply_network_view(cfg,cfg->options[i].value);
}
int nd_option_set(struct NDConfig *cfg, const char *id, const char *key,
                  const char *value)
{
    const struct NDOptionDef *built, *spec=NULL;
    const char *schema;
    int i,n;
    if (!cfg || !id || !key || !value ||
        !opt_token(id,20) || !opt_token(key,ND_OPT_KEY_LEN)) return 0;
    built=nd_builtin_options(&n);
    schema=nd_option_schema_id(cfg,id);
    for (i=0; i<n; ++i)
        if (!strcmp(built[i].id,schema) && !strcmp(built[i].key,key)) spec=&built[i];
    for (i=0; i<cfg->def_count; ++i)
        if (!strcmp(cfg->defs[i].id,id) && !strcmp(cfg->defs[i].key,key))
            spec=&cfg->defs[i];
    if (!spec || !nd_option_validate(spec,value)) return 0;
    for (i=0; i<cfg->option_count; ++i) {
        if (!strcmp(cfg->options[i].id,id) && !strcmp(cfg->options[i].key,key)) {
            copy_string(cfg->options[i].value,value,ND_OPT_VALUE_LEN);
            if(!strcmp(id,"NET") && !strcmp(key,"VIEW"))apply_network_view(cfg,value);
            return 1;
        }
    }
    if (cfg->option_count >= ND_MAX_OPTIONS) return 0;
    i=cfg->option_count++;
    copy_string(cfg->options[i].id,id,sizeof(cfg->options[i].id));
    copy_string(cfg->options[i].key,key,sizeof(cfg->options[i].key));
    copy_string(cfg->options[i].value,value,sizeof(cfg->options[i].value));
    if(!strcmp(id,"NET") && !strcmp(key,"VIEW"))apply_network_view(cfg,value);
    return 1;
}
int nd_options_changed(const struct NDConfig *old,const struct NDConfig *next,
                       const char *id)
{
    const char *a,*b;
    const struct NDOptionDef *builtin;
    int i,n;
    if (!old || !next || !id) return 1;
    builtin=nd_builtin_options(&n);
    for (i=0;i<n;++i) if (!strcmp(builtin[i].id,nd_option_schema_id(next,id))) {
        if(!strcmp(id,"NET") && !strcmp(builtin[i].key,"VIEW"))continue;
        a=nd_option_get(old,id,builtin[i].key);
        b=nd_option_get(next,id,builtin[i].key);
        if (!a || !b || strcmp(a,b)) return 1;
    }
    for (i=0;i<old->option_count;++i) if (!strcmp(old->options[i].id,id)) {
        if(!strcmp(id,"NET") && !strcmp(old->options[i].key,"VIEW"))continue;
        a=nd_option_get(old,id,old->options[i].key);
        b=nd_option_get(next,id,old->options[i].key);
        if (!a || !b || strcmp(a,b)) return 1;
    }
    for (i=0;i<next->option_count;++i) if (!strcmp(next->options[i].id,id)) {
        if(!strcmp(id,"NET") && !strcmp(next->options[i].key,"VIEW"))continue;
        a=nd_option_get(old,id,next->options[i].key);
        b=nd_option_get(next,id,next->options[i].key);
        if (!a || !b || strcmp(a,b)) return 1;
    }
    return 0;
}
int nd_option_define(struct NDConfig *cfg, const char *id, const char *key,
                     const char *label, const char *type, const char *choices,
                     const char *default_value)
{
    struct NDOptionDef def;
    int i, count;
    const struct NDOptionDef *built;
    if (!cfg || !id || !key || !label || !type || !choices || !default_value ||
        !opt_token(id,20) || !opt_token(key,ND_OPT_KEY_LEN) ||
        !*label || strlen(label)>=sizeof(def.label) ||
        strlen(type)>=sizeof(def.type) || strlen(choices)>=sizeof(def.choices) ||
        strlen(default_value)>=sizeof(def.default_value) ||
        strchr(label,'|') || strchr(choices,'|') || strchr(default_value,'|')) return 0;
    memset(&def,0,sizeof(def));
    copy_string(def.id,id,sizeof(def.id)); copy_string(def.key,key,sizeof(def.key));
    copy_string(def.label,label,sizeof(def.label));
    copy_string(def.type,type,sizeof(def.type));
    copy_string(def.choices,choices,sizeof(def.choices));
    copy_string(def.default_value,default_value,sizeof(def.default_value));
    if (!nd_option_validate(&def,default_value)) return 0;
    built=nd_builtin_options(&count);
    for (i=0; i<count; ++i)
        if (!strcmp(built[i].id,id) && !strcmp(built[i].key,key)) return 0;
    for (i=0; i<cfg->def_count; ++i)
        if (!strcmp(cfg->defs[i].id,id) && !strcmp(cfg->defs[i].key,key)) return 0;
    if (cfg->def_count>=ND_MAX_OPTION_DEFS) return 0;
    cfg->defs[cfg->def_count++]=def;
    return 1;
}
void nd_default_config(struct NDConfig *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    cfg->side = ND_SIDE_BOTTOM;
    cfg->position = ND_POSITION_CENTER;
    cfg->font_mode = ND_FONT_WORKBENCH;
    cfg->label_mode = ND_LABEL_ALWAYS;
    cfg->item_size = ND_ITEM_SIZE_NORMAL;
    cfg->running_apps = 1;
    cfg->removable_media = 1;
    cfg->refresh_mode = ND_REFRESH_LOW_MEMORY;
    cfg->theme = ND_THEME_NORD;
    copy_string(cfg->theme_id,"Nord",sizeof(cfg->theme_id));
    cfg->shape = ND_SHAPE_ROUNDED;
    cfg->tile_width = 86;
    cfg->tile_height = 76;
    cfg->gap = 5;
    cfg->rows = 1;
    cfg->left_click = ND_CLICK_LAUNCH;
    cfg->tooltips = 1;
    cfg->always_on_top = 0;
    cfg->auto_hide = 0;
    cfg->hide_delay = 3;
    add_item(cfg, ND_ITEM_DRAWER, "System", "SYS:", NULL, "SYS:Disk", 0, 1, 0);
    cfg->items[0].show_volume_meter=1;
    add_item(cfg, ND_ITEM_LAUNCH, "Shell", "NewShell", NULL, "SYS:System/Shell", 0, 1, 0);
    add_item(cfg, ND_ITEM_PLUGIN, "MusicPlayer", "Plugins/MusicPlayer", "MUSIC", NULL, 0, 1, 0);
    add_item(cfg, ND_ITEM_PLUGIN, "CPU", "Plugins/Activity", "CPU", NULL, 0, 1, 0);
    add_item(cfg, ND_ITEM_PLUGIN, "RAM", "Plugins/Memory", "RAM", NULL, 0, 1, 0);
    add_item(cfg, ND_ITEM_PLUGIN, "Clock", "Plugins/Clock", "CLOCK", NULL, 0, 1, 0);
    add_item(cfg, ND_ITEM_PLUGIN, "Network", "Plugins/Network", "NET", NULL, 0, 0, 0);
    add_item(cfg, ND_ITEM_PLUGIN, "Eyes", "Plugins/Eyes", "EYES", NULL, 0, 0, 1);
    add_item(cfg, ND_ITEM_PLUGIN, "DOpus 4", "Plugins/DOpus4", "DOPUS4", NULL, 0, 0, 0);
    add_item(cfg, ND_ITEM_PLUGIN, "Extract", "Plugins/ArchiveExtractor", "ARCHIVE", NULL, 0, 0, 0);
    add_item(cfg, ND_ITEM_PLUGIN, "Running Apps", "Plugins/RunningApps",
             "RUNNINGAPPS", NULL, 0, 0, 0);
}
const char *nd_theme_name(int theme)
{
    switch (theme) {
    case ND_THEME_LIGHT: return "LIGHT";
    case ND_THEME_WORKBENCH: return "WORKBENCH";
    case ND_THEME_GRAPHITE: return "GRAPHITE";
    case ND_THEME_HIGH_CONTRAST: return "HIGH_CONTRAST";
    case ND_THEME_COPPER: return "COPPER";
    case ND_THEME_OCEAN: return "OCEAN";
    case ND_THEME_EMERALD: return "EMERALD";
    case ND_THEME_NEXTSTEP: return "NEXTSTEP";
    case ND_THEME_BEOS: return "BEOS";
    case ND_THEME_NORD: return "NORD";
    case ND_THEME_VAPOR: return "VAPOR";
    default: return "MOCHA";
    }
}
const char *nd_config_theme_id(const struct NDConfig *cfg)
{
    int mapped;
    if (!cfg) return "Mocha";
    if (!cfg->theme_id[0]) return config_theme_id(cfg->theme);
    mapped=config_theme_index(cfg->theme_id);
    if (mapped>=0 && mapped!=cfg->theme) return config_theme_id(cfg->theme);
    return cfg->theme_id;
}
int nd_config_set_theme(struct NDConfig *cfg,const char *theme_id)
{
    int theme;
    if (!cfg || !config_theme_id_valid(theme_id)) return 0;
    theme=config_theme_index(theme_id);
    if (theme>=0) {
        cfg->theme=theme;
        copy_string(cfg->theme_id,config_theme_id(theme),sizeof(cfg->theme_id));
    } else {
        cfg->theme=ND_THEME_MOCHA;
        copy_string(cfg->theme_id,theme_id,sizeof(cfg->theme_id));
    }
    return 1;
}
const char *nd_shape_name(int shape)
{
    switch (shape) {
    case ND_SHAPE_SQUARE: return "SQUARE";
    case ND_SHAPE_CHAMFER: return "CHAMFER";
    case ND_SHAPE_MINIMAL: return "MINIMAL";
    default: return "ROUNDED";
    }
}
const char *nd_position_name(int position)
{
    if (position==ND_POSITION_START) return "START";
    if (position==ND_POSITION_END) return "END";
    return "CENTER";
}
const char *nd_font_name(int mode)
{
    if (mode==ND_FONT_TOPAZ) return "TOPAZ";
    if (mode==ND_FONT_BOLD) return "BOLD";
    return "WORKBENCH";
}
const char *nd_label_mode_name(int mode)
{
    if (mode==ND_LABEL_HOVER) return "HOVER";
    if (mode==ND_LABEL_HIDDEN) return "HIDDEN";
    return "ALWAYS";
}
const char *nd_item_size_name(int mode)
{
    return mode==ND_ITEM_SIZE_TINY ? "TINY" : "NORMAL";
}
int nd_preserve_unknown(struct NDConfig *cfg,const char *key,const char *value)
{
    const unsigned char *p;
    int i;
    if (!cfg || !key || !value || !*key) return 0;
    for (p=(const unsigned char *)key;*p;++p)
        if (!(isalnum(*p) || *p=='_' || *p=='.' || *p=='-')) return 0;
    if (strlen(key)+strlen(value)+2>ND_UNKNOWN_LEN) {
        ++cfg->unknown_dropped;
        return 1;
    }
    for (i=0;i<cfg->unknown_count;++i)
        if (!strncmp(cfg->unknown[i],key,strlen(key)) &&
            cfg->unknown[i][strlen(key)]=='=') {
            sprintf(cfg->unknown[i],"%s=%s",key,value);
            return 1;
        }
    if (cfg->unknown_count>=ND_MAX_UNKNOWN) {
        ++cfg->unknown_dropped;
        return 1;
    }
    sprintf(cfg->unknown[cfg->unknown_count++],"%s=%s",key,value);
    return 1;
}
int nd_parse_line(struct NDConfig *cfg, const char *line)
{
    char buffer[1280];
    char *key, *value, *split, *name, *cmd, *id, *view, *state;
    int graph, enabled, multiple;
    int n;
    if (strlen(line) >= sizeof(buffer)) return 0;
    copy_string(buffer, line, sizeof(buffer));
    key = trim(buffer);
    if (!*key || *key == '#' || *key == ';') return 1;
    value = strchr(key, '=');
    if (!value) return 0;
    *value++ = '\0';
    key = trim(key);
    value = trim(value);
    if (!strncmp(key,"OPTION.",7)) {
        char *dot=strchr(key+7,'.');
        if (!dot) return 0;
        *dot++='\0';
        /* Migrate only the former shipped development path. */
        if (!strcmp(key+7,"BITSKIM") && !strcmp(dot,"PROGRAM") &&
            !ci_cmp(value,"Bin:BitSkim/BitSkim"))
            value="SYS:Utilities/BitSkim/BitSkim";
        if (!strcmp(dot,"SCRIPT") && !strcmp(value,"SYS:Utilities/NovaDock/Rexx/ScriptTile.rexx"))
            value="NovaDock:Rexx/ScriptTile.rexx";
        return nd_option_set(cfg,key+7,dot,value);
    }
    if (!ci_cmp(key,"OPTIONDEF")) {
        char *parts[6]; int i;
        parts[0]=value;
        for (i=1; i<6; ++i) {
            char *sep=strchr(parts[i-1],'|');
            if (!sep) return 0;
            *sep++='\0'; parts[i]=sep;
        }
        if (strchr(parts[5],'|')) return 0;
        return nd_option_define(cfg,trim(parts[0]),trim(parts[1]),trim(parts[2]),
                                trim(parts[3]),trim(parts[4]),trim(parts[5]));
    }
    if (!ci_cmp(key, "SIDE")) {
        if (!ci_cmp(value, "BOTTOM")) cfg->side = ND_SIDE_BOTTOM;
        else if (!ci_cmp(value, "TOP")) cfg->side = ND_SIDE_TOP;
        else if (!ci_cmp(value, "LEFT")) cfg->side = ND_SIDE_LEFT;
        else if (!ci_cmp(value, "RIGHT")) cfg->side = ND_SIDE_RIGHT;
        else return 0;
        return 1;
    }
    if (!ci_cmp(key,"POSITION")) {
        if (!ci_cmp(value,"START")) cfg->position=ND_POSITION_START;
        else if (!ci_cmp(value,"CENTER")) cfg->position=ND_POSITION_CENTER;
        else if (!ci_cmp(value,"END")) cfg->position=ND_POSITION_END;
        else return 0;
        return 1;
    }
    if (!ci_cmp(key,"FONT")) {
        if (!ci_cmp(value,"WORKBENCH")) cfg->font_mode=ND_FONT_WORKBENCH;
        else if (!ci_cmp(value,"TOPAZ")) cfg->font_mode=ND_FONT_TOPAZ;
        else if (!ci_cmp(value,"BOLD")) cfg->font_mode=ND_FONT_BOLD;
        else return 0;
        return 1;
    }
    if (!ci_cmp(key,"LABELS")) {
        if (!ci_cmp(value,"ALWAYS")) cfg->label_mode=ND_LABEL_ALWAYS;
        else if (!ci_cmp(value,"HOVER")) cfg->label_mode=ND_LABEL_HOVER;
        else if (!ci_cmp(value,"HIDDEN")) cfg->label_mode=ND_LABEL_HIDDEN;
        else return 0;
        return 1;
    }
    if (!ci_cmp(key,"ITEM_SIZE")) {
        if (!ci_cmp(value,"NORMAL")) cfg->item_size=ND_ITEM_SIZE_NORMAL;
        else if (!ci_cmp(value,"TINY")) cfg->item_size=ND_ITEM_SIZE_TINY;
        else return 0;
        return 1;
    }
    if (!ci_cmp(key, "THEME")) {
        return nd_config_set_theme(cfg,value);
    }
    if (!ci_cmp(key,"SHAPE")) {
        if (!ci_cmp(value,"ROUNDED")) cfg->shape=ND_SHAPE_ROUNDED;
        else if (!ci_cmp(value,"SQUARE")) cfg->shape=ND_SHAPE_SQUARE;
        else if (!ci_cmp(value,"CHAMFER")) cfg->shape=ND_SHAPE_CHAMFER;
        else if (!ci_cmp(value,"MINIMAL")) cfg->shape=ND_SHAPE_MINIMAL;
        else return 0;
        return 1;
    }
    if (!ci_cmp(key,"BACKGROUND")) {
        /* Compatibility only: older configurations remain loadable, but
         * graphic dock backdrops are deliberately ignored. */
        return !strchr(value,'|') && !strchr(value,'\n') && !strchr(value,'\r');
    }
    if (!ci_cmp(key, "TILE_WIDTH")) {
        n = numeric(value, 55, 140);
        if (n < 0) return 0;
        cfg->tile_width = n;
        return 1;
    }
    if (!ci_cmp(key, "TILE_HEIGHT")) {
        n = numeric(value, 46, 100);
        if (n < 0) return 0;
        cfg->tile_height = n;
        return 1;
    }
    if (!ci_cmp(key, "ROWS")) {
        n = numeric(value, ND_ROWS_MIN, ND_ROWS_MAX);
        if (n < 0) return 0;
        cfg->rows = n;
        return 1;
    }
    if (!ci_cmp(key,"ROW_COUNTS")) {
        char counts[64],*part,*next;
        int row=0,total=0,count;
        if (!ci_cmp(value,"AUTO")) {
            cfg->row_layout=0;
            memset(cfg->row_counts,0,sizeof(cfg->row_counts));
            return 1;
        }
        if (strlen(value)>=sizeof(counts)) return 0;
        strcpy(counts,value);part=counts;
        memset(cfg->row_counts,0,sizeof(cfg->row_counts));
        while (part && row<ND_ROWS_MAX) {
            next=strchr(part,',');if (next) *next++='\0';
            count=numeric(trim(part),0,ND_MAX_ITEMS);
            if (count<0 || total+count>ND_MAX_ITEMS) return 0;
            cfg->row_counts[row++]=count;total+=count;part=next;
        }
        if (part || row<1) return 0;
        cfg->row_layout=1;
        return 1;
    }
    if (!ci_cmp(key, "LEFT_CLICK")) {
        if (!ci_cmp(value, "MENU")) cfg->left_click = ND_CLICK_MENU;
        else if (!ci_cmp(value, "LAUNCH")) cfg->left_click = ND_CLICK_LAUNCH;
        else return 0;
        return 1;
    }
    if (!ci_cmp(key, "TOOLTIPS")) {
        if (!ci_cmp(value, "YES")) cfg->tooltips = 1;
        else if (!ci_cmp(value, "NO")) cfg->tooltips = 0;
        else return 0;
        return 1;
    }
    if (!ci_cmp(key, "ALWAYS_ON_TOP")) {
        if (!ci_cmp(value, "YES")) cfg->always_on_top = 1;
        else if (!ci_cmp(value, "NO")) cfg->always_on_top = 0;
        else return 0;
        return 1;
    }
    if (!ci_cmp(key, "FOLDER_VIEW")) {
        if (!ci_cmp(value,"ICONS")) cfg->folder_view=0;
        else if (!ci_cmp(value,"LIST")) cfg->folder_view=1;
        else return 0;
        return 1;
    }
    if (!ci_cmp(key, "FOLDER_SORT")) {
        if (!ci_cmp(value,"NAME")) cfg->folder_sort=0;
        else if (!ci_cmp(value,"DATE")) cfg->folder_sort=1;
        else if (!ci_cmp(value,"SIZE")) cfg->folder_sort=2;
        else return 0;
        return 1;
    }
    if (!ci_cmp(key, "RUNNING_SECTION")) {
        /* Accepted for old files, but automatic transient task tiles were
         * retired in favour of the explicit Running Apps plugin. */
        if (ci_cmp(value,"YES") && ci_cmp(value,"NO")) return 0;
        cfg->running_section=0;
        return 1;
    }
    if (!ci_cmp(key,"HOVER")) {
        /* Retired pointer effects: accept old files without enabling any
         * effect, and omit the directive on the next save. */
        return !ci_cmp(value,"OFF") || !ci_cmp(value,"IMAGE") ||
               !ci_cmp(value,"ZOOM") || !ci_cmp(value,"BOTH");
    }
    if (!ci_cmp(key, "HOOK")) {
        /* HOOK=EVENT|command; an event may have several hooks. */
        const char *bar=strchr(value,'|');
        size_t event_len=bar ? (size_t)(bar-value) : 0;
        struct NDHook *hook;
        if (!bar || !event_len || event_len>=ND_HOOK_EVENT_LEN || !bar[1] ||
            strlen(bar+1)>=ND_HOOK_COMMAND_LEN || cfg->hook_count>=ND_MAX_HOOKS)
            return 0;
        hook=&cfg->hooks[cfg->hook_count];
        memcpy(hook->event,value,event_len);hook->event[event_len]='\0';
        for (event_len=0;hook->event[event_len];++event_len)
            hook->event[event_len]=(char)toupper((unsigned char)hook->event[event_len]);
        if (!nd_hook_event_valid(hook->event)) return 0;
        strcpy(hook->command,bar+1);
        ++cfg->hook_count;
        return 1;
    }
    if (!ci_cmp(key, "DOCK_HOTKEY")) {
        if (strlen(value)>=sizeof(cfg->dock_hotkey)) return 0;
        strcpy(cfg->dock_hotkey,value);
        return 1;
    }
    if (!ci_cmp(key, "REMOVABLE_MEDIA")) {
        if (!ci_cmp(value,"YES")) cfg->removable_media=1;
        else if (!ci_cmp(value,"NO")) cfg->removable_media=0;
        else return 0;
        return 1;
    }
    if (!ci_cmp(key, "RUNNING_APPS")) {
        if (!ci_cmp(value,"YES")) cfg->running_apps=1;
        else if (!ci_cmp(value,"NO")) cfg->running_apps=0;
        else return 0;
        return 1;
    }
    if (!ci_cmp(key, "REFRESH_MODE")) {
        if (!ci_cmp(value,"LOW_MEMORY")) cfg->refresh_mode=ND_REFRESH_LOW_MEMORY;
        else if (!ci_cmp(value,"SMOOTH")) cfg->refresh_mode=ND_REFRESH_SMOOTH;
        else return 0;
        return 1;
    }
    if (!ci_cmp(key, "AUTO_HIDE")) {
        if (!ci_cmp(value, "YES")) cfg->auto_hide = 1;
        else if (!ci_cmp(value, "NO")) cfg->auto_hide = 0;
        else return 0;
        return 1;
    }
    if (!ci_cmp(key, "HIDE_DELAY")) {
        n = numeric(value, 1, 60);
        if (n < 0) return 0;
        cfg->hide_delay = n;
        return 1;
    }
    if (!ci_cmp(key, "GAP")) {
        n = numeric(value, 0, 16);
        if (n < 0) return 0;
        cfg->gap = n;
        return 1;
    }
    if (!ci_cmp(key,"SPACER")) {
        int added,width=0,line=1,line_width=2,line_color=ND_SPACER_COLOR_SUBTLE;
        char *size,*show,*color,*thickness;
        state=strchr(value,'|');
        enabled=1;
        if (state) {
            *state++='\0'; state=trim(state);
            size=strchr(state,'|');
            if (size) {
                *size++='\0';size=trim(size);
                show=strchr(size,'|');
                if (show) {
                    *show++='\0';show=trim(show);
                    color=strchr(show,'|');
                    if (color) {
                        *color++='\0';color=trim(color);
                        thickness=strchr(color,'|');
                        if (thickness) {
                            *thickness++='\0';thickness=trim(thickness);
                            line_width=numeric(thickness,1,8);
                            if (line_width<0) return 0;
                        }
                        line_color=spacer_color_value(color);
                        if (line_color<0) return 0;
                    }
                    if (!ci_cmp(show,"YES")) line=1;
                    else if (!ci_cmp(show,"NO")) line=0;
                    else return 0;
                }
                width=numeric(size,20,140);
                if (width<0 && ci_cmp(size,"DEFAULT")) return 0;
                if (width<0) width=0;
            }
            if (!ci_cmp(state,"ON")) enabled=1;
            else if (!ci_cmp(state,"OFF")) enabled=0;
            else return 0;
        }
        added=nd_add_spacer(cfg,trim(value));
        if (added) {
            cfg->items[cfg->count-1].enabled=enabled;
            cfg->items[cfg->count-1].spacer_width=width;
            cfg->items[cfg->count-1].spacer_line=line;
            cfg->items[cfg->count-1].spacer_line_width=line_width;
            cfg->items[cfg->count-1].spacer_line_color=line_color;
        }
        return added;
    }
    if (!ci_cmp(key, "LAUNCH") || !ci_cmp(key, "DRAWER") ||
        !ci_cmp(key, "GROUP") || !ci_cmp(key, "PLUGIN")) {
        name = value;
        split = strchr(name, '|');
        if (!split) return 0;
        *split++ = '\0';
        cmd = trim(split);
        name = trim(name);
        if (!ci_cmp(key, "LAUNCH") || !ci_cmp(key, "DRAWER") ||
            !ci_cmp(key, "GROUP")) {
            char *icon = strchr(cmd, '|');
            char *parts[11];
            int count=0,mode=ND_LAUNCH_AUTO,tools=0,show_meter=-1,added,i;
            enabled=1;
            if (icon) {
                *icon++ = '\0';
                parts[count++]=icon;
                while (count<11) {
                    char *sep=strchr(parts[count-1],'|');
                    if (!sep) break;
                    *sep++='\0';parts[count++]=sep;
                }
                if (strchr(parts[count-1],'|')) return 0;
                for (i=0;i<count;++i) parts[i]=trim(parts[i]);
                icon=parts[0];
                if (count>=2) {
                    state=parts[1];
                    if (!ci_cmp(state,"ON")) enabled=1;
                    else if (!ci_cmp(state,"OFF")) enabled=0;
                    else return 0;
                }
                if (count>=3) {
                    mode=launch_mode_value(parts[2]);
                    if (mode<0) return 0;
                }
                if (count>=7) {
                    if (!ci_cmp(parts[6],"YES")) tools=1;
                    else if (ci_cmp(parts[6],"NO")) return 0;
                }
                if (count>=8) {
                    if (!ci_cmp(parts[7],"YES")) show_meter=1;
                    else if (!ci_cmp(parts[7],"NO")) show_meter=0;
                    else return 0;
                }
            }
            added=nd_add_launcher(cfg, !ci_cmp(key, "DRAWER") ?
                                  ND_ITEM_DRAWER : !ci_cmp(key, "GROUP") ?
                                  ND_ITEM_GROUP : ND_ITEM_LAUNCH,
                                  name, trim(cmd), icon);
            if (!added) return 0;
            cfg->items[cfg->count-1].enabled=enabled;
            if (show_meter>=0)
                cfg->items[cfg->count-1].show_volume_meter=show_meter;
            if (count>=3 && !nd_set_launcher_action(cfg,cfg->count-1,mode,
                    count>=4 ? parts[3] : "",count>=5 ? parts[4] : "",
                    count>=6 ? parts[5] : "",tools)) {
                --cfg->count;
                return 0;
            }
            if (count>=9 && !nd_set_launcher_match(cfg,cfg->count-1,
                    parts[8],count>=10 ? parts[9] : "",
                    count>=11 ? parts[10] : "")) {
                --cfg->count;
                return 0;
            }
            return 1;
        }
        split = strchr(cmd, '|');
        if (!split) return 0;
        *split++ = '\0';
        id = trim(split);
        cmd = trim(cmd);
        view = strchr(id, '|');
        graph = 0; /* Three-field plugin lines remain compatible. */
        enabled = 1;
        multiple = -1;
        if (view) {
            char *multi;
            *view++ = '\0';
            state = strchr(view, '|');
            if (state) {
                *state++ = '\0';
                multi = strchr(state,'|');
                if (multi) *multi++='\0';
                state = trim(state);
                if (!ci_cmp(state, "ON")) enabled = 1;
                else if (!ci_cmp(state, "OFF")) enabled = 0;
                else return 0;
                if (multi) {
                    multi=trim(multi);
                    if (!ci_cmp(multi,"YES")) multiple=1;
                    else if (!ci_cmp(multi,"NO")) multiple=0;
                    else return 0;
                    if (strchr(multi,'|')) return 0;
                }
            }
            view = trim(view);
            if (!ci_cmp(view, "GRAPH")) graph = 1;
            else if (!ci_cmp(view, "METER")) graph = 0;
            else return 0;
        }
        id = trim(id);
        if (multiple<0) multiple=!ci_cmp(id,"EYES"); /* legacy Eyes entries */
        return add_item(cfg, ND_ITEM_PLUGIN, name, cmd, id, NULL, graph,
                        enabled, multiple);
    }
    /* Forward compatibility: a well-formed KEY=value line that this version
     * does not understand (typically written by a newer NovaDock) is kept
     * verbatim and written back on save instead of invalidating the file. */
    return nd_preserve_unknown(cfg,key,value);
}
int nd_load_config(struct NDConfig *cfg, const char *filename)
{
    FILE *f;
    char line[1280];
    int line_number = 0, bad_lines = 0;
    nd_default_config(cfg);
    f = fopen(filename, "r");
    if (!f) return -1; /* defaults are usable */
    cfg->count = 0;
    while (fgets(line, sizeof(line), f)) {
        ++line_number;
        if (!strchr(line, '\n') && strlen(line) == sizeof(line) - 1U) {
            int ch;
            while ((ch = fgetc(f)) != '\n' && ch != EOF) {}
            ++bad_lines;
            continue;
        }
        if (!nd_parse_line(cfg, line)) ++bad_lines;
    }
    fclose(f);
    /* 0.1 originally shipped separate CHIPRAM and FASTRAM tiles.  Fold old
     * user configs into the compact dual-bar RAM tile on first load while
     * retaining the earliest tile position and either tile's enabled state. */
    {
        int r,w=0,first=-1,enabled=0,legacy=0,has_ram_poll=0;
        char poll_value[ND_OPT_VALUE_LEN];
        poll_value[0]='\0';
        for (r=0;r<cfg->option_count;++r) {
            if (!strcmp(cfg->options[r].id,"RAM") &&
                !strcmp(cfg->options[r].key,"POLL_TICKS")) has_ram_poll=1;
            if (!poll_value[0] &&
                (!strcmp(cfg->options[r].id,"CHIPRAM") ||
                 !strcmp(cfg->options[r].id,"FASTRAM")) &&
                !strcmp(cfg->options[r].key,"POLL_TICKS"))
                copy_string(poll_value,cfg->options[r].value,sizeof(poll_value));
        }
        for (r=0;r<cfg->count;++r) {
            struct NDItem *it=&cfg->items[r];
            if (it->kind==ND_ITEM_PLUGIN &&
                (!strcmp(it->id,"RAM") || !strcmp(it->id,"CHIPRAM") ||
                 !strcmp(it->id,"FASTRAM"))) {
                enabled=enabled || it->enabled;
                if (first<0) {
                    if (w!=r) cfg->items[w]=cfg->items[r];
                    first=w++;
                }
                if (strcmp(it->id,"RAM")) legacy=1;
            } else {
                if (w!=r) cfg->items[w]=cfg->items[r];
                ++w;
            }
        }
        if (first>=0 && legacy) {
            struct NDItem *ram=&cfg->items[first];
            ram->kind=ND_ITEM_PLUGIN;
            copy_string(ram->label,"RAM",sizeof(ram->label));
            copy_string(ram->command,"Plugins/Memory",sizeof(ram->command));
            copy_string(ram->id,"RAM",sizeof(ram->id));
            ram->graph=0;ram->enabled=enabled;
            cfg->count=w;
            if (!has_ram_poll && poll_value[0])
                (void)nd_option_set(cfg,"RAM","POLL_TICKS",poll_value);
        }
    }
    /* Keep the bundled plugin catalogue complete after an upgrade without
     * changing an existing user's visible dock.  Older configurations may
     * predate any of these executables; missing entries are retained as
     * disabled definitions so Add Item can offer every installed component.
     * Command is intentionally absent because it is exposed as the
     * Application page's Command tile kind, not as a raw plugin. */
    {
        static const struct {
            const char *label,*command,*id;
            int graph,multiple;
        } bundled[]={
            {"Clock","Plugins/Clock","CLOCK",0,0},
            {"CPU","Plugins/Activity","CPU",1,0},
            {"RAM","Plugins/Memory","RAM",0,0},
            {"Network","Plugins/Network","NET",0,0},
            {"MusicPlayer","Plugins/MusicPlayer","MUSIC",0,0},
            {"Eyes","Plugins/Eyes","EYES",0,1},
            {"DOpus 4","Plugins/DOpus4","DOPUS4",0,0},
            {"Extract","Plugins/ArchiveExtractor","ARCHIVE",0,0},
            {"Running Apps","Plugins/RunningApps","RUNNINGAPPS",0,0}
        };
        int i,j,found;
        for (j=0;j<(int)(sizeof(bundled)/sizeof(bundled[0]));++j) {
            found=0;
            for (i=0;i<cfg->count;++i)
                if (cfg->items[i].kind==ND_ITEM_PLUGIN &&
                    !strcmp(cfg->items[i].id,bundled[j].id)) {
                    found=1;
                    /* Eyes gained repeated-tile support after its original
                     * configuration format shipped. */
                    if (bundled[j].multiple)
                        cfg->items[i].multiple_instances=1;
                }
            if (!found && cfg->count<ND_MAX_ITEMS)
                add_item(cfg,ND_ITEM_PLUGIN,bundled[j].label,
                         bundled[j].command,bundled[j].id,NULL,
                         bundled[j].graph,0,bundled[j].multiple);
        }
    }
    upgrade_network_tiles(cfg);
    normalize_row_counts(cfg);
    apply_saved_network_view(cfg);
    (void)line_number;
    return bad_lines;
}

/* Preferences keeps inactive definitions; the running dock uses only ON. */
int nd_compact_enabled(struct NDConfig *cfg)
{
    int r, w = 0;
    if (!cfg) return 0;
    normalize_row_counts(cfg);
    for (r = 0; r < cfg->count; ++r) {
        if (!cfg->items[r].enabled) continue;
        if (w != r) cfg->items[w] = cfg->items[r];
        ++w;
    }
    cfg->count = w;
    return w;
}
int nd_item_runtime_equal(const char *pool_a,const struct NDItem *a,
                          const char *pool_b,const struct NDItem *b)
{
    int k;
    if (!a || !b) return 0;
    for (k=0;k<ND_TEXT_COUNT;++k)
        if (strcmp(nd_pool_text(pool_a,a->text[k]),nd_pool_text(pool_b,b->text[k])))
            return 0;
    return a->kind==b->kind && !strcmp(a->label,b->label) &&
        !strcmp(a->command,b->command) &&
        !strcmp(a->id,b->id) &&
        a->graph==b->graph && a->enabled==b->enabled &&
        a->multiple_instances==b->multiple_instances &&
        a->launch_mode==b->launch_mode && a->tools_menu==b->tools_menu &&
        a->show_volume_meter==b->show_volume_meter &&
        a->spacer_width==b->spacer_width &&
        a->spacer_line==b->spacer_line &&
        a->spacer_line_width==b->spacer_line_width &&
        a->spacer_line_color==b->spacer_line_color &&
        a->transient==b->transient;
}
int nd_config_can_update_live(const struct NDConfig *a,const struct NDConfig *b)
{
    return a && b && a->font_mode==b->font_mode &&
        a->refresh_mode==b->refresh_mode && a->item_size==b->item_size &&
        !strcmp(nd_config_theme_id(a),nd_config_theme_id(b));
}
int nd_config_runtime_equal(const struct NDConfig *a,const struct NDConfig *b)
{
    int i;
    if (!a || !b || a->option_count!=b->option_count ||
        a->def_count!=b->def_count || a->count!=b->count ||
        a->side!=b->side || a->position!=b->position ||
        a->font_mode!=b->font_mode || a->label_mode!=b->label_mode ||
        a->item_size!=b->item_size ||
        strcmp(nd_config_theme_id(a),nd_config_theme_id(b)) ||
        a->shape!=b->shape ||
        a->tile_width!=b->tile_width || a->tile_height!=b->tile_height ||
        a->gap!=b->gap || a->rows!=b->rows ||
        a->row_layout!=b->row_layout ||
        a->left_click!=b->left_click || a->tooltips!=b->tooltips ||
        a->always_on_top!=b->always_on_top ||
        a->running_apps!=b->running_apps ||
        a->removable_media!=b->removable_media ||
        strcmp(a->dock_hotkey,b->dock_hotkey) ||
        a->running_section!=b->running_section ||
        a->folder_view!=b->folder_view || a->folder_sort!=b->folder_sort ||
        a->refresh_mode!=b->refresh_mode || a->auto_hide!=b->auto_hide ||
        a->hide_delay!=b->hide_delay ||
        a->hook_count!=b->hook_count) return 0;
    for (i=0;i<a->hook_count && i<ND_MAX_HOOKS;++i)
        if (strcmp(a->hooks[i].event,b->hooks[i].event) ||
            strcmp(a->hooks[i].command,b->hooks[i].command)) return 0;
    for (i=0;i<ND_ROWS_MAX;++i)
        if (a->row_counts[i]!=b->row_counts[i]) return 0;
    for (i=0;i<a->count;++i)
        if (!nd_item_runtime_equal(a->text_pool,&a->items[i],
                                   b->text_pool,&b->items[i])) return 0;
    for (i=0;i<a->option_count;++i)
        if (strcmp(a->options[i].id,b->options[i].id) ||
            strcmp(a->options[i].key,b->options[i].key) ||
            strcmp(a->options[i].value,b->options[i].value)) return 0;
    for (i=0;i<a->def_count;++i)
        if (strcmp(a->defs[i].id,b->defs[i].id) ||
            strcmp(a->defs[i].key,b->defs[i].key) ||
            strcmp(a->defs[i].label,b->defs[i].label) ||
            strcmp(a->defs[i].type,b->defs[i].type) ||
            strcmp(a->defs[i].choices,b->defs[i].choices) ||
            strcmp(a->defs[i].default_value,b->defs[i].default_value)) return 0;
    return 1;
}
static const char *effective_config_source="Built-in defaults";
int nd_publish_config_source(const char *target)
{
    FILE *f;
    const char *source=effective_config_source;
#if defined(__amigaos__) || defined(AMIGA) || defined(__VBCC__)
    char resolved[320];
    BPTR lock;
#endif
    if (!target || !source || !strchr(source,':')) {
        if (target) remove(target);
        return 0;
    }
#if defined(__amigaos__) || defined(AMIGA) || defined(__VBCC__)
    lock=Lock((STRPTR)source,ACCESS_READ);
    if(!lock){remove(target);return 0;}
    if(!NameFromLock(lock,resolved,sizeof(resolved))){UnLock(lock);remove(target);return 0;}
    UnLock(lock);source=resolved;
#endif
    f=fopen(target,"w");
    if (!f) return 0;
    fprintf(f,"%s\n",source);
    return fclose(f)==0;
}
const char *nd_effective_config_source(void)
{
    return effective_config_source;
}
static void upgrade_network_tiles(struct NDConfig *cfg)
{
    /* Replace retired Roadshow definitions with Network. Retain the old
     * option namespace so Network can read legacy settings as a fallback. */
    {
        int net=-1,road=-1,i,j;
        for(i=0;i<cfg->count;++i)if(cfg->items[i].kind==ND_ITEM_PLUGIN){
            if(!strcmp(cfg->items[i].id,"NET"))net=i;
            else if(!strcmp(cfg->items[i].id,"ROADSHOW"))road=i;
        }
        if(road>=0 && net<0){
            strcpy(cfg->items[road].id,"NET");
            strcpy(cfg->items[road].command,"Plugins/Network");
            if(!strcmp(cfg->items[road].label,"Roadshow"))
                strcpy(cfg->items[road].label,"Network");
            net=road;
        }else if(net>=0 && road>=0 && cfg->items[road].enabled){
            if(cfg->items[net].enabled)(void)nd_set_item_enabled(cfg,road,0);
            else {
                struct NDItem old=cfg->items[road];
                cfg->items[road]=cfg->items[net];cfg->items[road].enabled=1;
                cfg->items[road].graph=old.graph;
                cfg->items[net]=old;cfg->items[net].enabled=0;
            }
        }
        for(i=0;i<cfg->count;){
            if(cfg->items[i].kind==ND_ITEM_PLUGIN && !strcmp(cfg->items[i].id,"ROADSHOW")){
                if(cfg->items[i].enabled)(void)nd_set_item_enabled(cfg,i,0);
                for(j=i;j+1<cfg->count;++j)cfg->items[j]=cfg->items[j+1];
                --cfg->count;
            }else ++i;
        }
    }
}
void nd_upgrade_network_config(struct NDConfig *cfg)
{
    if(!cfg)return;
    {
        int i;
        const char *legacy="SYS:Utilities/NovaDock/Plugins/";
        for(i=0;i<cfg->count;++i)if(cfg->items[i].kind==ND_ITEM_PLUGIN &&
            !strncmp(cfg->items[i].command,legacy,strlen(legacy))) {
            char path[ND_COMMAND_LEN];
            strcpy(path,"Plugins/");strcat(path,cfg->items[i].command+strlen(legacy));
            strcpy(cfg->items[i].command,path);
        }
    }
    upgrade_network_tiles(cfg);
    apply_saved_network_view(cfg);
    /* Old visible driver fields could accidentally leave the sampler on a
     * different device than the connection. Overrides now require opt-in. */
    if(strcmp(nd_option_get(cfg,"NET","SAMPLING_OVERRIDE"),"YES")){
        const char *device=nd_option_get(cfg,"NET","DEVICE");
        const char *unit=nd_option_get(cfg,"NET","UNIT");
        if(device && *device)(void)nd_option_set(cfg,"NET","DEVICE","");
        if(unit && strcmp(unit,"0"))(void)nd_option_set(cfg,"NET","UNIT","0");
    }
}
int nd_load_effective(struct NDConfig *cfg)
{
    static const char *paths[]={
        "ENV:NovaDock/novadock.cfg",
        "ENV:NovaDock/novadock.cfg.good",
        "ENV:NovaDock/novadock.cfg.bak",
        "ENVARC:NovaDock/novadock.cfg",
        "ENVARC:NovaDock/novadock.cfg.good",
        "ENVARC:NovaDock/novadock.cfg.bak",
        "PROGDIR:novadock.cfg",
        "PROGDIR:novadock.cfg.good",
        "PROGDIR:novadock.cfg.bak",
        "SYS:NovaDock/novadock.cfg.default",
        NULL
    };
    int status,i,saw_invalid=0;
    if (!cfg) return 1;
    for (i=0;paths[i];++i) {
        if (!strncmp(paths[i],"PROGDIR:",8) && !ND_CONFIG_HAVE_PROGDIR()) continue;
        status=nd_load_config(cfg,paths[i]);
        if (status==0) {nd_upgrade_network_config(cfg);effective_config_source=paths[i];return 0;}
        if (status>0) saw_invalid=1;
    }
    nd_default_config(cfg);
    effective_config_source="Built-in defaults";
    return saw_invalid ? 1 : -1;
}
/* Some classic handlers, including directory-backed ENV: volumes,
 * allow normal file I/O but do not implement replacement Rename() reliably.
 * The caller has already completed and closed SOURCE before using this
 * fallback. A known-good .bak remains available if the destination write
 * itself fails. */
static int copy_complete_file(const char *source_name,const char *dest_name)
{
    FILE *source,*destination;
    char buffer[1024];
    size_t got;
    int ok=1;
    source=fopen(source_name,"rb");
    if (!source) return 0;
    destination=fopen(dest_name,"wb");
    if (!destination) {fclose(source);return 0;}
    while ((got=fread(buffer,1,sizeof(buffer),source))!=0)
        if (fwrite(buffer,1,got,destination)!=got) {ok=0;break;}
    if (ferror(source)) ok=0;
    if (fclose(source)!=0) ok=0;
    if (fclose(destination)!=0) ok=0;
    return ok;
}
static int preserve_config_backup(const char *filename,const char *backup)
{
    FILE *source,*destination;
    struct NDConfig *check;
    char stage[320],buffer[1024];
    size_t got;
    int ok=1;
    source=fopen(filename,"rb");
    if (!source) return 1; /* First save: there is no previous generation. */
    fclose(source);
    check=(struct NDConfig *)malloc(sizeof(*check));
    if (!check) return 0;
    ok=nd_load_config(check,filename)==0;
    free(check);
    /* Never replace a known-good backup with the malformed file from which
     * the current session may just have recovered. */
    if (!ok) return 1;
    source=fopen(filename,"rb");
    if (!source) return 0;
    strcpy(stage,backup);strcat(stage,".new");
    remove(stage);
    destination=fopen(stage,"wb");
    if (!destination) {fclose(source);return 0;}
    while ((got=fread(buffer,1,sizeof(buffer),source))!=0)
        if (fwrite(buffer,1,got,destination)!=got) {ok=0;break;}
    if (ferror(source)) ok=0;
    if (fclose(source)!=0) ok=0;
    if (fclose(destination)!=0) ok=0;
    if (!ok) {remove(stage);return 0;}
    if (rename(stage,backup)!=0) {
        remove(backup);
        if (rename(stage,backup)!=0) {
            if (!copy_complete_file(stage,backup)) {
                remove(stage);return 0;
            }
            remove(stage);
        }
    }
    return 1;
}
/* Keep a separately validated last-known-good generation.  Unlike .bak this
 * is refreshed only after the newly written live file can be parsed in full,
 * so recovery never depends on the previous session having exited cleanly. */
static void publish_known_good(const char *filename)
{
    struct NDConfig *check;
    char good[320],stage[320];
    size_t n;
    int valid;
    if (!filename) return;
    n=strlen(filename);
    if (n+10>=sizeof(good)) return;
    check=(struct NDConfig *)malloc(sizeof(*check));
    if (!check) return;
    valid=nd_load_config(check,filename)==0;
    free(check);
    if (!valid) return;
    strcpy(good,filename);strcat(good,".good");
    strcpy(stage,good);strcat(stage,".new");
    remove(stage);
    if (!copy_complete_file(filename,stage)) {remove(stage);return;}
    check=(struct NDConfig *)malloc(sizeof(*check));
    if (!check) {remove(stage);return;}
    valid=nd_load_config(check,stage)==0;
    free(check);
    if (!valid) {remove(stage);return;}
    if (rename(stage,good)!=0) {
        remove(good);
        if (rename(stage,good)!=0) {
            (void)copy_complete_file(stage,good);
            remove(stage);
        }
    }
}
/* Pure C89; data stays plain text for manual editing and third-party tools.
 * The complete new file is staged beside the destination and closed before
 * any replacement. Never truncate or copy into the live configuration.
 */
int nd_write_config(const struct NDConfig *cfg, const char *filename)
{
    char tmp[300], backup[300], swap[300];
    FILE *f;
    int i, ok = 1, had_old = 0;
    size_t n;
    if (!cfg || !filename || cfg->count < 0 || cfg->count > ND_MAX_ITEMS)
        return 0;
    n = strlen(filename);
    if (n + 10 >= sizeof(tmp)) return 0;
    strcpy(tmp, filename); strcat(tmp, ".new");
    strcpy(backup, filename); strcat(backup, ".bak");
    strcpy(swap, filename); strcat(swap, ".old");
    remove(tmp);
    f = fopen(tmp, "w");
    if (!f) return 0;
    if (fprintf(f, "# NovaDock 0.1 - text configuration\nSIDE=%s\nPOSITION=%s\nFONT=%s\nLABELS=%s\nITEM_SIZE=%s\nTHEME=%s\nSHAPE=%s\nTILE_WIDTH=%d\nTILE_HEIGHT=%d\nGAP=%d\nROWS=%d\nLEFT_CLICK=%s\nTOOLTIPS=%s\nALWAYS_ON_TOP=%s\nRUNNING_APPS=%s\nREFRESH_MODE=%s\nAUTO_HIDE=%s\nHIDE_DELAY=%d\n",
                cfg->side == ND_SIDE_TOP ? "TOP" : cfg->side == ND_SIDE_LEFT ? "LEFT" : cfg->side == ND_SIDE_RIGHT ? "RIGHT" : "BOTTOM",
                nd_position_name(cfg->position),nd_font_name(cfg->font_mode),
                nd_label_mode_name(cfg->label_mode),
                nd_item_size_name(cfg->item_size),
                nd_config_theme_id(cfg),
                nd_shape_name(cfg->shape),
                cfg->tile_width, cfg->tile_height, cfg->gap, cfg->rows,
                cfg->left_click == ND_CLICK_MENU ? "MENU" : "LAUNCH",
                cfg->tooltips ? "YES" : "NO",
                cfg->always_on_top ? "YES" : "NO",
                cfg->running_apps ? "YES" : "NO",
                nd_refresh_mode_name(cfg->refresh_mode),
                cfg->auto_hide ? "YES" : "NO", cfg->hide_delay) < 0) ok = 0;
    if (cfg->row_layout) {
        if (fprintf(f,"ROW_COUNTS=%d,%d,%d\n",cfg->row_counts[0],
                    cfg->row_counts[1],cfg->row_counts[2])<0) ok=0;
    } else if (fprintf(f,"ROW_COUNTS=AUTO\n")<0) ok=0;
    if (fprintf(f, "REMOVABLE_MEDIA=%s\nRUNNING_SECTION=%s\n"
                "FOLDER_VIEW=%s\nFOLDER_SORT=%s\n",
                cfg->removable_media ? "YES" : "NO",
                "NO",
                cfg->folder_view ? "LIST" : "ICONS",
                cfg->folder_sort==2 ? "SIZE" : cfg->folder_sort==1 ? "DATE" :
                "NAME") < 0) ok = 0;
    /* Only when set, so configurations without one stay unchanged. */
    if (cfg->dock_hotkey[0] &&
        fprintf(f,"DOCK_HOTKEY=%s\n",cfg->dock_hotkey)<0) ok=0;
    for (i=0;i<cfg->hook_count && i<ND_MAX_HOOKS;++i)
        if (fprintf(f,"HOOK=%s|%s\n",cfg->hooks[i].event,cfg->hooks[i].command)<0)
            ok=0;
    for (i = 0; i < cfg->count; ++i) {
        const struct NDItem *item = &cfg->items[i];
        /* Removable-media tiles exist only while the medium is present. */
        if (item->transient) continue;
        if (item->kind == ND_ITEM_PLUGIN) {
            if (fprintf(f, "PLUGIN=%s|%s|%s|%s|%s|%s\n", item->label,
                  item->command, item->id, item->graph ? "GRAPH" : "METER",
                  item->enabled ? "ON" : "OFF",
                  item->multiple_instances ? "YES" : "NO") < 0) ok = 0;
        } else if (item->kind == ND_ITEM_SPACER) {
            if (fprintf(f,"SPACER=%s|%s|%s",item->label,
                        item->enabled ? "ON" : "OFF",
                        item->spacer_width ? "" : "DEFAULT")<0) ok=0;
            if (item->spacer_width && fprintf(f,"%d",item->spacer_width)<0) ok=0;
            if (fprintf(f,"|%s|%s|%d\n",
                        item->spacer_line ? "YES" : "NO",
                        nd_spacer_color_name(item->spacer_line_color),
                        item->spacer_line_width ? item->spacer_line_width : 2)<0) ok=0;
        } else if (item->kind == ND_ITEM_LAUNCH ||
                   item->kind == ND_ITEM_DRAWER ||
                   item->kind == ND_ITEM_GROUP) {
            if (fprintf(f, "%s=%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s\n",
                        item->kind == ND_ITEM_DRAWER ? "DRAWER" :
                        item->kind == ND_ITEM_GROUP ? "GROUP" : "LAUNCH",
                        item->label,
                        item->command,nd_item_text(cfg,item,ND_TEXT_ICON),
                        item->enabled ? "ON" : "OFF",
                        nd_launch_mode_name(item->launch_mode),
                        nd_item_text(cfg,item,ND_TEXT_ARGS),
                        nd_item_text(cfg,item,ND_TEXT_WORKDIR),
                        nd_item_text(cfg,item,ND_TEXT_HOTKEY),
                        item->tools_menu ? "YES" : "NO",
                        item->show_volume_meter ? "YES" : "NO",
                        nd_item_text(cfg,item,ND_TEXT_MATCH_TASK),
                        nd_item_text(cfg,item,ND_TEXT_MATCH_WINDOW),
                        nd_item_text(cfg,item,ND_TEXT_MATCH_PORT)) < 0) ok = 0;
        }
    }
    if (cfg->unknown_count>0 &&
        fprintf(f,"# Settings preserved from a newer NovaDock version\n")<0) ok=0;
    for (i=0; i<cfg->unknown_count && i<ND_MAX_UNKNOWN; ++i)
        if (fprintf(f,"%s\n",cfg->unknown[i])<0) ok=0;
    for (i=0; i<cfg->def_count; ++i) {
        const struct NDOptionDef *d=&cfg->defs[i];
        if (fprintf(f,"OPTIONDEF=%s|%s|%s|%s|%s|%s\n",
                    d->id,d->key,d->label,d->type,d->choices,d->default_value)<0) ok=0;
    }
    for (i=0; i<cfg->option_count; ++i) {
        const struct NDOption *o=&cfg->options[i];
        if (fprintf(f,"OPTION.%s.%s=%s\n",o->id,o->key,o->value)<0) ok=0;
    }
    if (fclose(f) != 0) ok = 0;
    if (!ok) { remove(tmp); return 0; }
    if (!preserve_config_backup(filename,backup)) {remove(tmp);return 0;}
    if (rename(tmp, filename) == 0) {
        publish_known_good(filename);
        return 1;
    }
    remove(swap);
    if (rename(filename, swap) == 0) had_old = 1;
    if (rename(tmp, filename) != 0) {
        /* If replace-by-rename is unavailable, restore the old complete file.
         * Leaving BACKUP in place is safer than destroying it after a failed
         * rollback; the next successful save cleans stale staging names. */
        if (had_old) (void)rename(swap,filename);
        if (!copy_complete_file(tmp,filename)) {
            /* A failed copy must not replace a previously valid generation
             * with a partial file. BACKUP was validated before staging. */
            if (!copy_complete_file(backup,filename)) remove(filename);
            remove(tmp);
            return 0;
        }
        remove(tmp);
        if (had_old) remove(swap);
        publish_known_good(filename);
        return 1;
    }
    if (had_old) remove(swap);
    publish_known_good(filename);
    return 1;
}

/* A session preset: normal Use/Save controls when this becomes live. */
int nd_config_low_resource(struct NDConfig *cfg)
{
    struct NDConfig *copy;
    int i,ok;
    if(!cfg)return 0;
    copy=(struct NDConfig *)malloc(sizeof(*copy));if(!copy)return 0;
    memcpy(copy,cfg,sizeof(*copy));
    copy->refresh_mode=ND_REFRESH_LOW_MEMORY;copy->folder_view=1;
    copy->running_apps=0;copy->always_on_top=0;
    for(i=0;i<copy->count;++i)if(copy->items[i].kind==ND_ITEM_PLUGIN)copy->items[i].graph=0;
    ok=nd_option_set(copy,"CPU","POLL_TICKS","100") &&
       nd_option_set(copy,"RAM","POLL_TICKS","100") &&
       nd_option_set(copy,"NET","POLL_TICKS","100") &&
       nd_option_set(copy,"NET","VIEW","METER") &&
       nd_option_set(copy,"CLOCK","POLL_TICKS","100") &&
       nd_option_set(copy,"EYES","POLL_TICKS","100");
    if(ok)memcpy(cfg,copy,sizeof(*cfg));
    free(copy);return ok;
}
