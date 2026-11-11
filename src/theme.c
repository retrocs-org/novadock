#include "nd_theme.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const unsigned char defaults[ND_THEME_COUNT][ND_THEME_COLOR_COUNT][3]={
    {{24,24,37},{49,50,68},{69,71,90},{205,214,244},{166,173,200},{137,180,250},{35,36,50},{166,227,161}},
    {{205,212,227},{239,241,247},{250,252,255},{46,52,70},{101,109,132},{30,102,188},{182,188,202},{27,133,93}},
    {{148,158,178},{196,202,214},{224,228,236},{12,20,32},{54,66,88},{35,112,192},{124,136,158},{228,140,42}},
    {{28,30,32},{46,49,52},{65,69,73},{232,235,238},{170,176,182},{85,170,255},{80,84,88},{92,214,150}},
    {{0,0,0},{20,20,20},{60,60,60},{255,255,255},{220,220,220},{255,220,0},{8,8,8},{0,255,130}},
    {{43,22,15},{76,38,25},{111,58,34},{255,230,199},{205,154,112},{225,112,56},{55,28,19},{54,176,165}},
    {{4,32,61},{9,48,91},{16,78,130},{220,247,255},{126,199,226},{45,188,235},{5,37,69},{76,232,188}},
    {{3,43,24},{8,70,42},{16,102,59},{221,255,226},{120,205,145},{56,226,119},{4,50,29},{240,184,55}},
    {{78,78,78},{174,174,174},{218,218,218},{8,8,8},{58,58,58},{238,238,238},{48,48,48},{255,255,255}},
    {{156,156,148},{214,212,200},{242,199,48},{22,22,20},{102,100,94},{250,205,40},{118,116,108},{72,138,151}},
    {{38,45,58},{59,66,82},{76,86,106},{236,239,244},{129,161,193},{136,192,208},{46,52,64},{163,190,140}},
    {{20,10,38},{46,24,71},{76,34,104},{245,230,255},{173,128,204},{255,83,185},{28,14,51},{80,225,230}}
};
static const char *names[ND_THEME_COUNT]={
    "Mocha","Light","Workbench","Graphite","High Contrast",
    "Copper","Ocean","Emerald","NeXTSTEP","BeOS","Nord","Vapor"
};
static const char *dirs[ND_THEME_COUNT]={
    "Mocha","Light","Workbench","Graphite","HighContrast",
    "Copper","Ocean","Emerald","NeXTSTEP","BeOS","Nord","Vapor"
};
static const unsigned char default_shapes[ND_THEME_COUNT]={
    ND_SHAPE_CHAMFER,ND_SHAPE_ROUNDED,ND_SHAPE_SQUARE,ND_SHAPE_MINIMAL,
    ND_SHAPE_SQUARE,ND_SHAPE_CHAMFER,ND_SHAPE_ROUNDED,ND_SHAPE_CHAMFER,
    ND_SHAPE_SQUARE,ND_SHAPE_CHAMFER,ND_SHAPE_ROUNDED,ND_SHAPE_MINIMAL
};
static const unsigned char default_sides[ND_THEME_COUNT]={
    ND_SIDE_BOTTOM,ND_SIDE_BOTTOM,ND_SIDE_BOTTOM,ND_SIDE_BOTTOM,
    ND_SIDE_BOTTOM,ND_SIDE_BOTTOM,ND_SIDE_BOTTOM,ND_SIDE_BOTTOM,
    ND_SIDE_RIGHT,ND_SIDE_BOTTOM,ND_SIDE_BOTTOM,ND_SIDE_BOTTOM
};
static const unsigned char default_positions[ND_THEME_COUNT]={
    ND_POSITION_CENTER,ND_POSITION_CENTER,ND_POSITION_CENTER,ND_POSITION_CENTER,
    ND_POSITION_CENTER,ND_POSITION_CENTER,ND_POSITION_CENTER,ND_POSITION_CENTER,
    ND_POSITION_CENTER,ND_POSITION_CENTER,ND_POSITION_CENTER,ND_POSITION_CENTER
};
static const unsigned char default_fonts[ND_THEME_COUNT]={
    ND_FONT_WORKBENCH,ND_FONT_WORKBENCH,ND_FONT_TOPAZ,ND_FONT_TOPAZ,
    ND_FONT_BOLD,ND_FONT_TOPAZ,ND_FONT_WORKBENCH,ND_FONT_BOLD,
    ND_FONT_TOPAZ,ND_FONT_TOPAZ,ND_FONT_WORKBENCH,ND_FONT_BOLD
};
static const unsigned char default_labels[ND_THEME_COUNT]={
    ND_LABEL_ALWAYS,ND_LABEL_ALWAYS,ND_LABEL_ALWAYS,ND_LABEL_HOVER,
    ND_LABEL_ALWAYS,ND_LABEL_ALWAYS,ND_LABEL_HOVER,ND_LABEL_ALWAYS,
    ND_LABEL_ALWAYS,ND_LABEL_ALWAYS,ND_LABEL_ALWAYS,ND_LABEL_ALWAYS
};
static const unsigned char default_spacer_widths[ND_THEME_COUNT]={
    48,44,42,48,50,52,46,48,42,44,46,48
};
static const unsigned char default_spacer_lines[ND_THEME_COUNT]={
    0,1,1,1,1,1,0,1,1,1,1,1
};
static const unsigned char default_spacer_line_widths[ND_THEME_COUNT]={
    2,1,1,2,3,2,2,2,1,1,1,2
};
static const unsigned char default_spacer_line_colors[ND_THEME_COUNT]={
    ND_SPACER_COLOR_SUBTLE,ND_SPACER_COLOR_SUBTLE,ND_SPACER_COLOR_TEXT,
    ND_SPACER_COLOR_ACCENT,ND_SPACER_COLOR_ACCENT,ND_SPACER_COLOR_ACCENT,
    ND_SPACER_COLOR_SECONDARY,ND_SPACER_COLOR_SECONDARY,
    ND_SPACER_COLOR_TEXT,ND_SPACER_COLOR_SUBTLE,ND_SPACER_COLOR_SUBTLE,
    ND_SPACER_COLOR_ACCENT
};
static char *trim(char *s)
{
    char *end;
    while (*s && isspace((unsigned char)*s)) ++s;
    end=s+strlen(s);
    while (end>s && isspace((unsigned char)end[-1])) --end;
    *end='\0';
    return s;
}
static int copy_text(char *out,unsigned int size,const char *value)
{
    size_t n;
    if (!out || !size || !value) return 0;
    n=strlen(value);
    if (n>=size) return 0;
    memcpy(out,value,n+1);
    return 1;
}
static int join(char *out,unsigned int size,const char *a,const char *b)
{
    char first[360];
    size_t n;
    if (!copy_text(first,sizeof(first),a ? a : "")) return 0;
    if (!copy_text(out,size,first)) return 0;
    n=strlen(out);
    if (n && out[n-1]!='/' && out[n-1]!=':') {
        if (n+2>size) return 0;
        out[n++]='/';out[n]='\0';
    }
    if (strlen(out)+strlen(b)+1>size) return 0;
    strcat(out,b);
    return 1;
}
static int color_value(const char *value,unsigned char rgb[3])
{
    char tail;
    int r,g,b;
    if (sscanf(value,"%d,%d,%d%c",&r,&g,&b,&tail)!=3) return 0;
    if (r<0 || r>255 || g<0 || g>255 || b<0 || b>255) return 0;
    rgb[0]=(unsigned char)r;rgb[1]=(unsigned char)g;rgb[2]=(unsigned char)b;
    return 1;
}
static int spacer_color_value(const char *value)
{
    if (!strcmp(value,"SUBTLE")) return ND_SPACER_COLOR_SUBTLE;
    if (!strcmp(value,"ACCENT")) return ND_SPACER_COLOR_ACCENT;
    if (!strcmp(value,"SECONDARY")) return ND_SPACER_COLOR_SECONDARY;
    if (!strcmp(value,"TEXT")) return ND_SPACER_COLOR_TEXT;
    return -1;
}
static int same_id(const char *a,const char *b)
{
    unsigned char ca,cb;
    if (!a || !b) return 0;
    while (*a && *b) {
        ca=(unsigned char)tolower((unsigned char)*a++);
        cb=(unsigned char)tolower((unsigned char)*b++);
        if (ca!=cb) return 0;
    }
    return *a==*b;
}
const char *nd_theme_directory(int theme)
{
    if (theme<0 || theme>=ND_THEME_COUNT) theme=ND_THEME_MOCHA;
    return dirs[theme];
}
int nd_theme_id_valid(const char *theme_id)
{
    unsigned int n=0;
    if (!theme_id || !*theme_id) return 0;
    while (*theme_id) {
        unsigned char c=(unsigned char)*theme_id++;
        if (!isalnum(c) && c!='_' && c!='-') return 0;
        if (++n>=ND_THEME_ID_LEN) return 0;
    }
    return 1;
}
int nd_theme_index_from_id(const char *theme_id)
{
    int i;
    if (!theme_id) return -1;
    if (same_id(theme_id,"HIGH_CONTRAST") || same_id(theme_id,"HIGH CONTRAST"))
        return ND_THEME_HIGH_CONTRAST;
    for (i=0;i<ND_THEME_COUNT;++i)
        if (same_id(theme_id,dirs[i]) || same_id(theme_id,names[i])) return i;
    return -1;
}
void nd_theme_defaults(int theme,struct NDThemeSpec *spec)
{
    if (!spec) return;
    if (theme<0 || theme>=ND_THEME_COUNT) theme=ND_THEME_MOCHA;
    memset(spec,0,sizeof(*spec));
    copy_text(spec->name,sizeof(spec->name),names[theme]);
    memcpy(spec->rgb,defaults[theme],sizeof(spec->rgb));
    spec->shape=default_shapes[theme];
    spec->side=default_sides[theme];
    spec->position=default_positions[theme];
    spec->font_mode=default_fonts[theme];
    spec->label_mode=default_labels[theme];
    spec->binding_mask=ND_THEME_BIND_ALL;
    spec->spacer_width=default_spacer_widths[theme];
    spec->spacer_line=default_spacer_lines[theme];
    spec->spacer_line_width=default_spacer_line_widths[theme];
    spec->spacer_line_color=default_spacer_line_colors[theme];
    /* Formerly hard-wired to the Workbench theme's index; now a theme key
     * any theme can set (and Workbench's own file sets explicitly). */
    spec->bevel=theme==ND_THEME_WORKBENCH;
}
void nd_theme_defaults_id(const char *theme_id,struct NDThemeSpec *spec)
{
    int theme=nd_theme_index_from_id(theme_id);
    if (theme<0) theme=ND_THEME_MOCHA;
    nd_theme_defaults(theme,spec);
    if (theme_id && nd_theme_index_from_id(theme_id)<0)
        copy_text(spec->name,sizeof(spec->name),theme_id);
}
int nd_theme_load_id(const char *root,const char *theme_id,struct NDThemeSpec *spec)
{
    FILE *fp;
    char directory[320],path[360],line[320];
    int loaded=0;
    int theme;
    const char *folder;
    if (!spec) return 0;
    nd_theme_defaults_id(theme_id,spec);
    if (!root || !*root || !nd_theme_id_valid(theme_id)) return 0;
    theme=nd_theme_index_from_id(theme_id);
    folder=theme>=0 ? nd_theme_directory(theme) : theme_id;
    if (!join(directory,sizeof(directory),root,"Themes") ||
        !join(directory,sizeof(directory),directory,folder) ||
        !join(path,sizeof(path),directory,"theme.cfg")) return 0;
    fp=fopen(path,"r");
    if (!fp) return 0;
    /* A partial third-party theme only owns the properties it declares. */
    spec->binding_mask=0;
    while (fgets(line,sizeof(line),fp)) {
        char *key,*value,*eq;
        key=trim(line);
        if (!*key || *key=='#' || *key==';') continue;
        eq=strchr(key,'=');if (!eq) continue;
        *eq++='\0';key=trim(key);value=trim(eq);
        if (!strcmp(key,"NAME")) copy_text(spec->name,sizeof(spec->name),value);
        else if (!strcmp(key,"SHAPE")) {
            if (!strcmp(value,"ROUNDED")) {spec->shape=ND_SHAPE_ROUNDED;spec->binding_mask|=ND_THEME_BIND_SHAPE;}
            else if (!strcmp(value,"SQUARE")) {spec->shape=ND_SHAPE_SQUARE;spec->binding_mask|=ND_THEME_BIND_SHAPE;}
            else if (!strcmp(value,"CHAMFER")) {spec->shape=ND_SHAPE_CHAMFER;spec->binding_mask|=ND_THEME_BIND_SHAPE;}
            else if (!strcmp(value,"MINIMAL")) {spec->shape=ND_SHAPE_MINIMAL;spec->binding_mask|=ND_THEME_BIND_SHAPE;}
        } else if (!strcmp(key,"SIDE")) {
            if (!strcmp(value,"BOTTOM")) {spec->side=ND_SIDE_BOTTOM;spec->binding_mask|=ND_THEME_BIND_SIDE;}
            else if (!strcmp(value,"TOP")) {spec->side=ND_SIDE_TOP;spec->binding_mask|=ND_THEME_BIND_SIDE;}
            else if (!strcmp(value,"LEFT")) {spec->side=ND_SIDE_LEFT;spec->binding_mask|=ND_THEME_BIND_SIDE;}
            else if (!strcmp(value,"RIGHT")) {spec->side=ND_SIDE_RIGHT;spec->binding_mask|=ND_THEME_BIND_SIDE;}
        } else if (!strcmp(key,"POSITION")) {
            if (!strcmp(value,"START")) {spec->position=ND_POSITION_START;spec->binding_mask|=ND_THEME_BIND_POSITION;}
            else if (!strcmp(value,"CENTER")) {spec->position=ND_POSITION_CENTER;spec->binding_mask|=ND_THEME_BIND_POSITION;}
            else if (!strcmp(value,"END")) {spec->position=ND_POSITION_END;spec->binding_mask|=ND_THEME_BIND_POSITION;}
        } else if (!strcmp(key,"FONT")) {
            if (!strcmp(value,"WORKBENCH")) {spec->font_mode=ND_FONT_WORKBENCH;spec->binding_mask|=ND_THEME_BIND_FONT;}
            else if (!strcmp(value,"TOPAZ")) {spec->font_mode=ND_FONT_TOPAZ;spec->binding_mask|=ND_THEME_BIND_FONT;}
            else if (!strcmp(value,"BOLD")) {spec->font_mode=ND_FONT_BOLD;spec->binding_mask|=ND_THEME_BIND_FONT;}
        } else if (!strcmp(key,"LABELS")) {
            if (!strcmp(value,"ALWAYS")) {spec->label_mode=ND_LABEL_ALWAYS;spec->binding_mask|=ND_THEME_BIND_LABELS;}
            else if (!strcmp(value,"HOVER")) {spec->label_mode=ND_LABEL_HOVER;spec->binding_mask|=ND_THEME_BIND_LABELS;}
            else if (!strcmp(value,"HIDDEN")) {spec->label_mode=ND_LABEL_HIDDEN;spec->binding_mask|=ND_THEME_BIND_LABELS;}
        } else if (!strcmp(key,"BG")) color_value(value,spec->rgb[0]);
        else if (!strcmp(key,"TILE")) color_value(value,spec->rgb[1]);
        else if (!strcmp(key,"HOVER")) color_value(value,spec->rgb[2]);
        else if (!strcmp(key,"TEXT")) color_value(value,spec->rgb[3]);
        else if (!strcmp(key,"SUBTLE")) color_value(value,spec->rgb[4]);
        else if (!strcmp(key,"ACCENT")) color_value(value,spec->rgb[5]);
        else if (!strcmp(key,"BAR")) color_value(value,spec->rgb[6]);
        else if (!strcmp(key,"ACCENT2")) color_value(value,spec->rgb[7]);
        else if (!strcmp(key,"SPACER_WIDTH")) {
            int width=atoi(value);
            if (width>=20 && width<=140) spec->spacer_width=width;
        } else if (!strcmp(key,"SPACER_LINE")) {
            if (!strcmp(value,"YES")) spec->spacer_line=1;
            else if (!strcmp(value,"NO")) spec->spacer_line=0;
        } else if (!strcmp(key,"SPACER_LINE_WIDTH")) {
            int width=atoi(value);
            if (width>=1 && width<=8) spec->spacer_line_width=width;
        } else if (!strcmp(key,"BEVEL")) {
            if (!strcmp(value,"YES")) spec->bevel=1;
            else if (!strcmp(value,"NO")) spec->bevel=0;
        } else if (!strcmp(key,"SPACER_LINE_COLOR")) {
            int color=spacer_color_value(value);
            if (color>=0) spec->spacer_line_color=color;
        }
        loaded=1;
    }
    fclose(fp);
    return loaded;
}

int nd_theme_load(const char *root,int theme,struct NDThemeSpec *spec)
{
    return nd_theme_load_id(root,nd_theme_directory(theme),spec);
}

int nd_theme_load_config(const char *root,const struct NDConfig *cfg,
                         struct NDThemeSpec *spec)
{
    const char *id;
    if (!cfg) return 0;
    id=cfg->theme_id[0] ? cfg->theme_id : nd_theme_directory(cfg->theme);
    return nd_theme_load_id(root,id,spec);
}

void nd_theme_apply_spacer(struct NDItem *item,const struct NDThemeSpec *spec)
{
    if (!item || !spec || item->kind!=ND_ITEM_SPACER) return;
    item->spacer_width=spec->spacer_width;
    item->spacer_line=spec->spacer_line;
    item->spacer_line_width=spec->spacer_line_width;
    item->spacer_line_color=spec->spacer_line_color;
}

void nd_theme_apply_spacers(struct NDConfig *cfg,const struct NDThemeSpec *spec)
{
    int i;
    if (!cfg || !spec) return;
    for (i=0;i<cfg->count;++i) nd_theme_apply_spacer(&cfg->items[i],spec);
}

void nd_theme_apply_preferences(struct NDConfig *cfg,const struct NDThemeSpec *spec)
{
    if (!cfg || !spec) return;
    if (spec->binding_mask&ND_THEME_BIND_SHAPE) cfg->shape=spec->shape;
    if (spec->binding_mask&ND_THEME_BIND_SIDE) cfg->side=spec->side;
    if (spec->binding_mask&ND_THEME_BIND_POSITION) cfg->position=spec->position;
    if (spec->binding_mask&ND_THEME_BIND_FONT) cfg->font_mode=spec->font_mode;
    if (spec->binding_mask&ND_THEME_BIND_LABELS) cfg->label_mode=spec->label_mode;
}
