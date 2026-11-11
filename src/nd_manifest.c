/* Portable parser and merger for self-describing plugin manifests. */
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "nd_manifest.h"
#include "nd_version.h"
#include "nd_options.h"

static void copy_text(char *dest,const char *src,size_t size)
{
    strncpy(dest,src,size-1);
    dest[size-1]='\0';
}
static char *trim(char *s)
{
    char *end;
    while (*s==' ' || *s=='\t') ++s;
    end=s+strlen(s);
    while (end>s && (end[-1]==' ' || end[-1]=='\t' || end[-1]=='\n' ||
                     end[-1]=='\r')) --end;
    *end='\0';
    return s;
}
static int same_nocase(const char *a,const char *b)
{
    while (*a && *b) {
        if (toupper((unsigned char)*a)!=toupper((unsigned char)*b)) return 0;
        ++a;++b;
    }
    return !*a && !*b;
}
static int valid_token(const char *s,size_t max)
{
    size_t n=0;
    for (;*s;++s,++n)
        if (!isalnum((unsigned char)*s) && *s!='_' && *s!='-') return 0;
    return n>0 && n<max;
}
static int fail(struct NDManifest *m,const char *why)
{
    if (!m->error[0]) copy_text(m->error,why,sizeof(m->error));
    return 0;
}
void nd_manifest_init(struct NDManifest *m,const char *file_name)
{
    const char *base=file_name ? file_name : "";
    const char *p;
    memset(m,0,sizeof(*m));
    m->abi=1;
    for (p=base;*p;++p) if (*p==':' || *p=='/') base=p+1;
    copy_text(m->name,base,sizeof(m->name));
}
int nd_manifest_line(struct NDManifest *m,const char *line)
{
    char buffer[512],*key,*value,*parts[5];
    int i;
    if (!m || !line) return 0;
    if (strlen(line)>=sizeof(buffer)) return fail(m,"line too long");
    strcpy(buffer,line);
    key=trim(buffer);
    if (strncmp(key,"NOVADOCK_",9) && strncmp(key,"novadock_",9)) return -1;
    value=strchr(key,'=');
    if (!value) return fail(m,"missing '='");
    *value++='\0';
    key=trim(key);value=trim(value);
    if (same_nocase(key,"NOVADOCK_ID")) {
        if (!valid_token(value,sizeof(m->id))) return fail(m,"invalid NOVADOCK_ID");
        copy_text(m->id,value,sizeof(m->id));
        return 1;
    }
    if (same_nocase(key,"NOVADOCK_NAME")) {
        if (!*value || strlen(value)>=sizeof(m->name) || strchr(value,'|'))
            return fail(m,"invalid NOVADOCK_NAME");
        copy_text(m->name,value,sizeof(m->name));
        return 1;
    }
    if (same_nocase(key,"NOVADOCK_VIEW")) {
        if (same_nocase(value,"GRAPH")) m->graph=1;
        else if (same_nocase(value,"METER")) m->graph=0;
        else return fail(m,"NOVADOCK_VIEW must be METER or GRAPH");
        return 1;
    }
    if (same_nocase(key,"NOVADOCK_ABI")) {
        if (strlen(value)!=1 || value[0]<'1' || value[0]>'9')
            return fail(m,"invalid NOVADOCK_ABI");
        m->abi=value[0]-'0';
        return 1;
    }
    if (same_nocase(key,"NOVADOCK_MULTIPLE")) {
        if (same_nocase(value,"YES")) m->multiple_instances=1;
        else if (same_nocase(value,"NO")) m->multiple_instances=0;
        else return fail(m,"NOVADOCK_MULTIPLE must be YES or NO");
        return 1;
    }
#define COPY_FIELD(KEY,FIELD) \
    if (same_nocase(key,KEY)) { \
        if (!*value || strlen(value)>=sizeof(m->FIELD) || strchr(value,'|')) \
            return fail(m,"invalid " KEY); \
        copy_text(m->FIELD,value,sizeof(m->FIELD)); \
        return 1; \
    }
    COPY_FIELD("NOVADOCK_VERSION",version)
    COPY_FIELD("NOVADOCK_AUTHOR",author)
    COPY_FIELD("NOVADOCK_DESCRIPTION",description)
    COPY_FIELD("NOVADOCK_LICENSE",license)
    COPY_FIELD("NOVADOCK_WEBSITE",website)
    COPY_FIELD("NOVADOCK_FEATURES",features)
    COPY_FIELD("NOVADOCK_PROGRAM",program)
#undef COPY_FIELD
    if (same_nocase(key,"NOVADOCK_OPTION")) {
        struct NDOptionDef *def;
        if (m->option_count>=ND_MANIFEST_MAX_OPTIONS)
            return fail(m,"too many NOVADOCK_OPTION lines");
        parts[0]=value;
        for (i=1;i<5;++i) {
            char *sep=strchr(parts[i-1],'|');
            if (!sep) return fail(m,"NOVADOCK_OPTION needs 5 fields");
            *sep++='\0';parts[i]=sep;
        }
        if (strchr(parts[4],'|')) return fail(m,"NOVADOCK_OPTION has extra fields");
        for (i=0;i<5;++i) parts[i]=trim(parts[i]);
        def=&m->options[m->option_count];
        memset(def,0,sizeof(*def));
        if (!valid_token(parts[0],sizeof(def->key)) || !*parts[1] ||
            strlen(parts[1])>=sizeof(def->label) ||
            strlen(parts[3])>=sizeof(def->choices) ||
            strlen(parts[4])>=sizeof(def->default_value))
            return fail(m,"invalid NOVADOCK_OPTION field");
        if (strcmp(parts[2],"BOOL") && strcmp(parts[2],"CHOICE") &&
            strcmp(parts[2],"UINT") && strcmp(parts[2],"TEXT") &&
            strcmp(parts[2],"FILE"))
            return fail(m,"option type must be BOOL, CHOICE, UINT, TEXT or FILE");
        copy_text(def->key,parts[0],sizeof(def->key));
        copy_text(def->label,parts[1],sizeof(def->label));
        copy_text(def->type,parts[2],sizeof(def->type));
        copy_text(def->choices,parts[3],sizeof(def->choices));
        copy_text(def->default_value,parts[4],sizeof(def->default_value));
        if (!nd_option_validate(def,def->default_value))
            return fail(m,"option default does not match its type");
        for (i=0;i<m->option_count;++i)
            if (!strcmp(m->options[i].key,def->key))
                return fail(m,"duplicate option key");
        ++m->option_count;
        return 1;
    }
    return -1; /* unknown NOVADOCK_* key: newer manifest, ignore */
}
int nd_manifest_finish(struct NDManifest *m)
{
    int i;
    if (!m) return 0;
    if (m->error[0]) return 0;
    if (!m->id[0]) return fail(m,"NOVADOCK_ID missing");
    if (m->abi>ND_MANIFEST_DOCK_ABI) return fail(m,"needs a newer NovaDock");
    if (!m->name[0]) copy_text(m->name,m->id,sizeof(m->name));
    for (i=0;i<m->option_count;++i)
        copy_text(m->options[i].id,m->id,sizeof(m->options[i].id));
    return 1;
}
int nd_manifest_package_ready(struct NDManifest *m)
{
    if (!nd_manifest_finish(m)) return 0;
    if (!m->version[0]) return fail(m,"NOVADOCK_VERSION missing");
    if (!m->author[0]) return fail(m,"NOVADOCK_AUTHOR missing");
    if (!m->description[0]) return fail(m,"NOVADOCK_DESCRIPTION missing");
    if (!m->license[0]) return fail(m,"NOVADOCK_LICENSE missing");
    if (!m->features[0]) return fail(m,"NOVADOCK_FEATURES missing");
    if (!m->program[0]) return fail(m,"NOVADOCK_PROGRAM missing");
    return 1;
}
int nd_manifest_bundled_id(const char *id)
{
    static const char *bundled[]={"CLOCK","CPU","RAM","CHIPRAM","FASTRAM",
        "NET","ROADSHOW","MUSIC","EYES","DOPUS4","ARCHIVE","RUNNINGAPPS",
        "COMMAND","TRASH","WHDLOAD","AMIRC","CLIPBOARD","BITSKIM",
        "MIXER","SCREENSHOT","SCREENMODE",
        NULL};
    int i;
    if (!id) return 0;
    for (i=0;bundled[i];++i) if (!strcmp(bundled[i],id)) return 1;
    return 0;
}
static int defs_match(const struct NDConfig *cfg,const struct NDManifest *m)
{
    int i,j,count=0;
    for (i=0;i<cfg->def_count;++i)
        if (!strcmp(cfg->defs[i].id,m->id)) ++count;
    if (count!=m->option_count) return 0;
    for (i=0;i<m->option_count;++i) {
        for (j=0;j<cfg->def_count;++j)
            if (!strcmp(cfg->defs[j].id,m->id) &&
                !strcmp(cfg->defs[j].key,m->options[i].key) &&
                !strcmp(cfg->defs[j].label,m->options[i].label) &&
                !strcmp(cfg->defs[j].type,m->options[i].type) &&
                !strcmp(cfg->defs[j].choices,m->options[i].choices) &&
                !strcmp(cfg->defs[j].default_value,m->options[i].default_value))
                break;
        if (j==cfg->def_count) return 0;
    }
    return 1;
}
static int define_all(struct NDConfig *cfg,const struct NDManifest *m)
{
    int i,j,count=0;
    const struct NDOptionDef *built=nd_builtin_options(&count);
    for (i=0;i<m->option_count;++i) {
        const struct NDOptionDef *d=&m->options[i];
        /* A bundled manifest also describes settings for discovery on older
         * docks. Current built-in schemas remain authoritative. */
        for (j=0;j<count;++j)
            if (nd_manifest_bundled_id(m->id) && !strcmp(built[j].id,m->id) &&
                !strcmp(built[j].key,d->key)) break;
        if (j<count) continue;
        if (!nd_option_define(cfg,m->id,d->key,d->label,d->type,d->choices,
                              d->default_value)) return 0;
    }
    return 1;
}
int nd_manifest_merge(struct NDConfig *cfg,const struct NDManifest *m,
                      const char *command)
{
    char line[ND_COMMAND_LEN+ND_LABEL_LEN+48];
    int i,w,present=-1,others=0,capability_changed=0;
    if (!cfg || !m || !m->id[0] || m->error[0]) return ND_MANIFEST_REJECTED;
    for (i=0;i<cfg->count;++i)
        if (cfg->items[i].kind==ND_ITEM_PLUGIN && !strcmp(cfg->items[i].id,m->id))
            present=i;
    /* Bundled plugins keep their built-in schema once present, but their
     * manifests must still be allowed to restore a missing catalogue entry. */
    if (present>=0 && nd_manifest_bundled_id(m->id))
        return ND_MANIFEST_UNCHANGED;
    for (i=0;i<cfg->def_count;++i)
        if (strcmp(cfg->defs[i].id,m->id)) ++others;
    if (others+m->option_count>ND_MAX_OPTION_DEFS) return ND_MANIFEST_REJECTED;
    if (present>=0) {
        capability_changed=cfg->items[present].multiple_instances!=
                           m->multiple_instances;
        for (i=0;i<cfg->count;++i)
            if (cfg->items[i].kind==ND_ITEM_PLUGIN &&
                !strcmp(cfg->items[i].id,m->id))
                cfg->items[i].multiple_instances=m->multiple_instances;
        if (defs_match(cfg,m) && !capability_changed)
            return ND_MANIFEST_UNCHANGED;
        /* An upgraded plugin may add, rename or drop settings: its manifest
         * is authoritative for its own schema.  Stored values are kept. */
        for (i=0,w=0;i<cfg->def_count;++i)
            if (strcmp(cfg->defs[i].id,m->id)) cfg->defs[w++]=cfg->defs[i];
        cfg->def_count=w;
        return define_all(cfg,m) ? ND_MANIFEST_UPDATED : ND_MANIFEST_REJECTED;
    }
    if (!command || !*command || strchr(command,'|') ||
        strlen(command)>=ND_COMMAND_LEN || cfg->count>=ND_MAX_ITEMS)
        return ND_MANIFEST_REJECTED;
    sprintf(line,"PLUGIN=%s|%s|%s|%s|OFF|%s",m->name,command,m->id,
            m->graph ? "GRAPH" : "METER",
            m->multiple_instances ? "YES" : "NO");
    if (!nd_parse_line(cfg,line)) return ND_MANIFEST_REJECTED;
    if (!define_all(cfg,m)) {
        /* Roll back the half-added entry and any definitions it got. */
        --cfg->count;
        for (i=0,w=0;i<cfg->def_count;++i)
            if (strcmp(cfg->defs[i].id,m->id)) cfg->defs[w++]=cfg->defs[i];
        cfg->def_count=w;
        return ND_MANIFEST_REJECTED;
    }
    return ND_MANIFEST_ADDED;
}

/* Public metadata for the bundled plugins (descriptions follow the
 * project website's plugin list). */
struct BuiltinPlugin {
    const char *id,*name,*features,*description;
    int multiple;
};
static const struct BuiltinPlugin builtin_plugins[]={
    {"BITSKIM","BitSkim","meter,click,drop,badge",
     "Transfer speed/results and stable-ID file sending; starts or restores BitSkim.",0},
    {"AMIRC","AmIRC","meter,click,badge",
     "AmIRC status and unread event badges; click to start or restore it.",0},
    {"CLIPBOARD","Clippy","meter,click,popup,badge",
     "Session text history; click to copy, delete, clear or pause capture.",0},
    {"CLOCK","Clock","meter,click,popup",
     "Digital or analogue clock; click opens a calendar.",0},
    {"CPU","CPU Activity","meter,graph",
     "Activity graph or meter from scheduler activity (not an exact CPU percentage).",0},
    {"RAM","RAM","meter,graph",
     "Chip and Fast memory readings or bars, or a two-line history graph.",0},
    {"CHIPRAM","Chip RAM","meter,graph","Free Chip memory.",0},
    {"FASTRAM","Fast RAM","meter,graph","Free Fast memory.",0},
    {"NET","Network","meter,graph,click,badge",
     "Roadshow Online/Offline controls and live packet meters or graphs.",0},
    {"MUSIC","MusicPlayer","meter,click,media",
     "EaglePlayer, AmigaAMP and HippoPlayer playback controls via ARexx.",0},
    {"EYES","Eyes","",
     "Two eyes follow the Workbench mouse pointer; optional blinking.",1},
    {"DOPUS4","DOpus 4","meter,click,badge",
     "Starts Directory Opus 4, restores its iconified screen or brings it forward.",0},
    {"ARCHIVE","Archive Extractor","meter,drop,badge",
     "Drop ZIP, LHA, RAR, ARJ, LZX, TAR or GZ archives to unpack them beside the archive.",0},
    {"WHDLOAD","WHDLoad Games","meter,click,popup",
     "Lists the WHDLoad games in a drawer and starts them as Workbench does.",0},
    {"TRASH","Trash","meter,click,drop,badge",
     "Drop files and drawers to move them to the Trashcan; empty it from the tile's menu.",0},
    {"RUNNINGAPPS","Running Apps","meter,click,popup,badge",
     "On-demand browser for open applications; select one to bring it forward.",0},
    {"MIXER","Mixer","meter,click,popup",
     "AHI output volume: click for a slider and mute; remembered across restarts.",0},
    {"SCREENSHOT","Screenshot","click,badge",
     "Click to save the front screen or Workbench as an IFF picture after a countdown.",0},
    {"SCREENMODE","Screen Mode","click,popup",
     "Shows the Workbench resolution; click to switch between saved ScreenMode presets.",0},
    {NULL,NULL,NULL,NULL,0}
};
int nd_manifest_builtin(const char *id,struct NDManifest *m)
{
    int i;
    if (!id || !m) return 0;
    for (i=0;builtin_plugins[i].id;++i)
        if (!strcmp(builtin_plugins[i].id,id)) {
            const struct BuiltinPlugin *b=&builtin_plugins[i];
            nd_manifest_init(m,b->name);
            strncpy(m->id,b->id,sizeof(m->id)-1);
            strncpy(m->name,b->name,sizeof(m->name)-1);
            strcpy(m->version,ND_VERSION);
            strcpy(m->author,"NovaDock");
            strncpy(m->description,b->description,sizeof(m->description)-1);
            strncpy(m->features,b->features,sizeof(m->features)-1);
            strcpy(m->license,"MIT");
            m->multiple_instances=b->multiple;
            return 1;
        }
    return 0;
}
static void describe_append(char *out,unsigned int size,const char *text,
                            const char *separator)
{
    size_t used=strlen(out),need=strlen(text)+(used ? strlen(separator) : 0);
    if (used+need+1>size) return;
    if (used) strcat(out,separator);
    strcat(out,text);
}
void nd_manifest_describe(const char *features,int multiple,int settings,
                          char *out,unsigned int size)
{
    static const char *const known[][2]={
        {"meter","Meter"},{"graph","graph"},{"click","click"},
        {"popup","popup"},{"drop","drop target"},{"media","media controls"},
        {"badge","badge"},{NULL,NULL}
    };
    char word[32];
    const char *p=features ? features : "";
    int i,n;
    if (!out || !size) return;
    out[0]='\0';
    while (*p) {
        while (*p==',' || *p==' ') ++p;
        for (n=0;p[n] && p[n]!=',' && n<(int)sizeof(word)-1;++n) word[n]=p[n];
        word[n]='\0';
        while (*p && *p!=',') ++p;
        if (!n) continue;
        while (n>0 && word[n-1]==' ') word[--n]='\0';
        for (i=0;known[i][0];++i) if (!strcmp(known[i][0],word)) break;
        describe_append(out,size,known[i][0] ? known[i][1] : word,", ");
    }
    if (multiple) describe_append(out,size,"several tiles","; ");
    if (settings) describe_append(out,size,"settings","; ");
    if (!out[0]) describe_append(out,size,"Tile only","");
    if (out[0]>='a' && out[0]<='z') out[0]=(char)(out[0]-'a'+'A');
}

static void package_reason(char *why,unsigned int size,const char *text)
{
    if (!why || !size) return;
    strncpy(why,text,size-1);why[size-1]='\0';
}
int nd_package_check(struct NDManifest *m,const char *manifest_file,
                     char *why,unsigned int why_size)
{
    size_t len,suffix=strlen(ND_MANIFEST_SUFFIX),i;
    const char *p;
    if (why && why_size) why[0]='\0';
    if (!m || !manifest_file) {package_reason(why,why_size,"no manifest");return 0;}
    if (!nd_manifest_package_ready(m)) {
        package_reason(why,why_size,m->error[0] ? m->error : "incomplete manifest");
        return 0;
    }
    for (p=m->program;*p;++p)
        if (*p==':' || *p=='/' || *p=='"' || *p=='|' || *p==' ') {
            package_reason(why,why_size,"NOVADOCK_PROGRAM must be a plain file name");
            return 0;
        }
    len=strlen(manifest_file);
    /* AmigaDOS names compare without regard to case. */
    if (len<=suffix || strlen(m->program)!=len-suffix) goto mismatch;
    for (i=0;i<len-suffix;++i)
        if (tolower((unsigned char)manifest_file[i])!=
            tolower((unsigned char)m->program[i])) goto mismatch;
    for (i=0;i<suffix;++i)
        if (tolower((unsigned char)manifest_file[len-suffix+i])!=
            tolower((unsigned char)ND_MANIFEST_SUFFIX[i])) goto mismatch;
    {
        static const char *programs[]={"Clock","Eyes","Activity","Memory",
            "Network","Roadshow","MusicPlayer","DOpus4","ArchiveExtractor",
            "Command","RunningApps","Trash","Games","AmIRC","BitSkim",
            "Clipboard","Mixer","Screenshot","ScreenMode","Script",NULL};
        const char *dot=strrchr(m->program,'.');
        int k;
        /* Also reserve icons/manifests and transaction suffixes. */
        if (dot && (same_nocase(dot,".info") || same_nocase(dot,".ndplugin") ||
                    same_nocase(dot,".tmp") || same_nocase(dot,".bak"))) {
            package_reason(why,why_size,"program name uses a reserved suffix");return 0;
        }
        for (k=0;programs[k];++k) if (same_nocase(m->program,programs[k])) {
            package_reason(why,why_size,"program name belongs to a bundled plugin");return 0;
        }
    }
    if (nd_manifest_bundled_id(m->id)) {
        package_reason(why,why_size,"its ID belongs to a bundled plugin");
        return 0;
    }
    return 1;
mismatch:
    package_reason(why,why_size,"manifest name must be <NOVADOCK_PROGRAM>.ndplugin");
    return 0;
}
