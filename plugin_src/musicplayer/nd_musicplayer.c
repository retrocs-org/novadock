/* Portable C89: transactional text configuration and bounded text helpers. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "nd_musicplayer.h"

static void set_profile(struct NDMusicProfile *p, const char *port,
                        const char *alternate, unsigned int volume_max)
{
    memset(p, 0, sizeof(*p));
    strcpy(p->port, port);
    strcpy(p->alt_port, alternate);
    /* Generic defaults used by players without a verified profile. */
    strcpy(p->commands[ND_MUSIC_PLAY], "PLAY");
    strcpy(p->commands[ND_MUSIC_PAUSE], "PAUSE");
    strcpy(p->commands[ND_MUSIC_STOP], "STOP");
    strcpy(p->commands[ND_MUSIC_PREVIOUS], "PREV");
    strcpy(p->commands[ND_MUSIC_NEXT], "NEXT");
    p->volume_max = volume_max;
    /* No guessed query or volume commands are sent by default. */
}
void nd_music_defaults(struct NDMusicConfig *cfg)
{
    if (!cfg) return;
    memset(cfg, 0, sizeof(*cfg));
    /* EaglePlayer 2.x documents rexx_EP as its public ARexx host. */
    set_profile(&cfg->eagle, "rexx_EP", "EAGLEPLAYER", 255U);
    strcpy(cfg->eagle.commands[ND_MUSIC_PLAY], "PLAY 1");
    strcpy(cfg->eagle.commands[ND_MUSIC_PAUSE], "PLAY 0");
    strcpy(cfg->eagle.commands[ND_MUSIC_PREVIOUS], "PREVMODULE");
    strcpy(cfg->eagle.commands[ND_MUSIC_NEXT], "NEXTMODULE");
    strcpy(cfg->eagle.extras[ND_MUSIC_QUERY_TITLE], "STATUS M FIL");
    strcpy(cfg->eagle.extras[ND_MUSIC_QUERY_VOLUME], "STATUS G VOL");
    strcpy(cfg->eagle.extras[ND_MUSIC_SET_VOLUME], "VOLUME");
    set_profile(&cfg->amp, "AMIGAAMP", "AMIGAAMP.1", 100U);
    /* HippoPlayer 2.47+ exposes these commands on its fixed ARexx port. */
    set_profile(&cfg->hippo, "HIPPOPLAYER", "HIPPOPLAYER", 64U);
    strcpy(cfg->hippo.commands[ND_MUSIC_PLAY], "CONT");
    strcpy(cfg->hippo.commands[ND_MUSIC_PAUSE], "STOP");
    strcpy(cfg->hippo.commands[ND_MUSIC_PREVIOUS], "CHOOSEPREV");
    strcpy(cfg->hippo.commands[ND_MUSIC_NEXT], "CHOOSENEXT");
    strcpy(cfg->hippo.extras[ND_MUSIC_QUERY_TITLE], "GET NAME");
    strcpy(cfg->hippo.extras[ND_MUSIC_QUERY_VOLUME], "GET VOLU");
    strcpy(cfg->hippo.extras[ND_MUSIC_SET_VOLUME], "VOLUME");
    strcpy(cfg->hippo.extras[ND_MUSIC_QUERY_PLAYING], "GET PLAY");
    cfg->mode = ND_MUSIC_AUTO;
    cfg->prefer = ND_MUSIC_EAGLE;
    cfg->poll_ticks = 100;
    cfg->query_ticks = 100;
    cfg->scroll_ticks = 25;
    cfg->title_width = 24;
    cfg->scroll_title = 1;
    cfg->auto_query = 1;
}
int nd_music_valid_port(const char *s)
{
    size_t i, n;
    if (!s || !(n = strlen(s)) || n > ND_MUSIC_PORT_LEN) return 0;
    for (i = 0; i < n; ++i) {
        unsigned char c = (unsigned char)s[i];
        if (!isalnum(c) && c != '_' && c != '.') return 0;
    }
    return 1;
}
int nd_music_valid_command(const char *s)
{
    size_t i, n;
    if (!s || !(n = strlen(s)) || n > ND_MUSIC_COMMAND_LEN) return 0;
    if (s[0] == ' ' || s[n-1] == ' ') return 0;
    for (i = 0; i < n; ++i) {
        unsigned char c = (unsigned char)s[i];
        if (!isalnum(c) && c != '_' && c != '.' && c != ' ' &&
            c != '/' && c != '-' && c != ':') return 0;
    }
    return 1;
}
static void trim(char *s)
{
    char *t = s;
    size_t n;
    while (*t == ' ' || *t == '\t') ++t;
    if (s != t) memmove(s, t, strlen(t) + 1);
    n = strlen(s);
    while (n && (s[n-1] == ' ' || s[n-1] == '\t' ||
                 s[n-1] == '\r' || s[n-1] == '\n')) s[--n] = 0;
}
static int choice(const char *s, int allow_auto)
{
    if (allow_auto && !strcmp(s, "AUTO")) return ND_MUSIC_AUTO;
    if (!strcmp(s, "EAGLEPLAYER")) return ND_MUSIC_EAGLE;
    if (!strcmp(s, "AMIGAAMP")) return ND_MUSIC_AMP;
    if (!strcmp(s, "HIPPOPLAYER")) return ND_MUSIC_HIPPO;
    return -1;
}
static int yesno(const char *s)
{
    if (!strcmp(s, "YES")) return 1;
    if (!strcmp(s, "NO")) return 0;
    return -1;
}
static int bounded_number(const char *s, unsigned long minimum,
                          unsigned long maximum, unsigned int *dst)
{
    char *end;
    unsigned long value;
    if (!s[0] || s[0] == '-' || s[0] == '+') return 0;
    value = strtoul(s, &end, 10);
    if (*end || value < minimum || value > maximum) return 0;
    *dst = (unsigned int)value;
    return 1;
}
static int set_entry(struct NDMusicConfig *tmp, char *key, char *val)
{
    struct NDMusicProfile *p;
    char *field;
    int id, n;
    static const char *names[ND_MUSIC_ACTIONS] = {
        "PLAY", "PAUSE", "STOP", "PREVIOUS", "NEXT"
    };
    static const char *extras[ND_MUSIC_EXTRAS] = {
        "QUERY_TITLE", "QUERY_TIME", "VOL_UP", "VOL_DOWN",
        "QUERY_VOLUME", "SET_VOLUME", "QUERY_PLAYING"
    };
    if (!strcmp(key, "MODE")) {
        n = choice(val, 1);
        if (n < 0) return 0;
        tmp->mode = n;
        return 1;
    }
    if (!strcmp(key, "PREFER")) {
        n = choice(val, 0);
        if (n < 0) return 0;
        tmp->prefer = n;
        return 1;
    }
    /* DEFAULT_APP belongs to the dock: it is used only when an idle Music
     * tile is clicked. Accept it here so the resident player can share the
     * same MUSIC option namespace without treating the dock-only field as a
     * malformed configuration entry. */
    if (!strcmp(key,"DEFAULT_APP")) return 1;
    if (!strcmp(key, "POLL_TICKS"))
        return bounded_number(val, 25UL, 3000UL, &tmp->poll_ticks);
    if (!strcmp(key, "QUERY_TICKS"))
        return bounded_number(val, 100UL, 30000UL, &tmp->query_ticks);
    if (!strcmp(key, "SCROLL_TICKS"))
        return bounded_number(val, 10UL, 3000UL, &tmp->scroll_ticks);
    if (!strcmp(key, "TITLE_WIDTH"))
        return bounded_number(val, 8UL, 30UL, &tmp->title_width);
    if (!strcmp(key, "AUTO_QUERY") || !strcmp(key, "SCROLL_TITLE")) {
        n = yesno(val);
        if (n < 0) return 0;
        if (!strcmp(key, "AUTO_QUERY")) tmp->auto_query = n;
        else tmp->scroll_title = n;
        return 1;
    }
    if (!strncmp(key, "EAGLE_", 6)) {
        p = &tmp->eagle; field = key + 6;
    } else if (!strncmp(key, "AMIGAAMP_", 9)) {
        p = &tmp->amp; field = key + 9;
    } else if (!strncmp(key, "HIPPO_", 6)) {
        p = &tmp->hippo; field = key + 6;
    } else return 0;
    if (!strcmp(field, "PORT") || !strcmp(field, "PORT_ALT")) {
        if (!nd_music_valid_port(val)) return 0;
        strcpy(!strcmp(field, "PORT") ? p->port : p->alt_port, val);
        return 1;
    }
    if (!strcmp(field, "VOLUME_MAX"))
        return bounded_number(val, 1UL, 65535UL, &p->volume_max);
    for (id = 0; id < ND_MUSIC_ACTIONS; ++id) {
        if (!strcmp(field, names[id])) {
            if (!nd_music_valid_command(val)) return 0;
            strcpy(p->commands[id], val);
            return 1;
        }
    }
    for (id = 0; id < ND_MUSIC_EXTRAS; ++id) {
        if (!strcmp(field, extras[id])) {
            if (val[0] && !nd_music_valid_command(val)) return 0;
            strcpy(p->extras[id], val);
            return 1;
        }
    }
    return 0;
}
int nd_music_apply_option(void *ctx, const char *key, const char *value)
{
    char k[96],v[ND_MUSIC_CONFIG_LINE];
    if (!ctx || !key || !value || strlen(key)>=sizeof(k) ||
        strlen(value)>=sizeof(v)) return 0;
    strcpy(k,key); strcpy(v,value);
    return set_entry((struct NDMusicConfig *)ctx,k,v);
}
int nd_music_load(struct NDMusicConfig *cfg, const char *filename)
{
    struct NDMusicConfig tmp;
    FILE *f;
    char buf[ND_MUSIC_CONFIG_LINE], *eq;
    int ok = 1;
    if (!cfg || !filename) return 0;
    nd_music_defaults(&tmp);
    f = fopen(filename, "r");
    if (!f) return 0;
    while (fgets(buf, sizeof(buf), f)) {
        size_t len = strlen(buf);
        if (len && buf[len-1] != '\n' && !feof(f)) {
            int c;
            while ((c = fgetc(f)) != EOF && c != '\n') { }
            ok = 0;
            break;
        }
        trim(buf);
        if (!buf[0] || buf[0] == '#' || buf[0] == ';') continue;
        eq = strchr(buf, '=');
        if (!eq) { ok = 0; break; }
        *eq++ = 0;
        trim(buf);
        trim(eq);
        if (!set_entry(&tmp, buf, eq)) { ok = 0; break; }
    }
    if (ferror(f)) ok = 0;
    fclose(f);
    if (ok) *cfg = tmp;
    return ok;
}
int nd_music_select(int mode, int prefer, int eagle_present, int amp_present,
                    int hippo_present)
{
    if (mode == ND_MUSIC_EAGLE) return eagle_present ? ND_MUSIC_EAGLE : ND_MUSIC_NONE;
    if (mode == ND_MUSIC_AMP) return amp_present ? ND_MUSIC_AMP : ND_MUSIC_NONE;
    if (mode == ND_MUSIC_HIPPO) return hippo_present ? ND_MUSIC_HIPPO : ND_MUSIC_NONE;
    if (mode != ND_MUSIC_AUTO) return ND_MUSIC_NONE;
    if (prefer == ND_MUSIC_EAGLE && eagle_present) return ND_MUSIC_EAGLE;
    if (prefer == ND_MUSIC_AMP && amp_present) return ND_MUSIC_AMP;
    if (prefer == ND_MUSIC_HIPPO && hippo_present) return ND_MUSIC_HIPPO;
    if (eagle_present) return ND_MUSIC_EAGLE;
    if (amp_present) return ND_MUSIC_AMP;
    if (hippo_present) return ND_MUSIC_HIPPO;
    return ND_MUSIC_NONE;
}
const char *nd_music_name(int player)
{
    if (player == ND_MUSIC_EAGLE) return "EaglePlayer";
    if (player == ND_MUSIC_AMP) return "AmigaAMP";
    if (player == ND_MUSIC_HIPPO) return "HippoPlayer";
    return "None";
}
const struct NDMusicProfile *nd_music_profile(const struct NDMusicConfig *cfg,
                                               int player)
{
    if (!cfg) return NULL;
    if (player == ND_MUSIC_EAGLE) return &cfg->eagle;
    if (player == ND_MUSIC_AMP) return &cfg->amp;
    if (player == ND_MUSIC_HIPPO) return &cfg->hippo;
    return NULL;
}
const char *nd_music_followup(int player, int action)
{
    if (player==ND_MUSIC_HIPPO &&
        (action==ND_MUSIC_PREVIOUS || action==ND_MUSIC_NEXT)) return "PLAY";
    return NULL;
}
void nd_music_clean_text(char *out, unsigned int capacity, const char *input)
{
    unsigned int len = 0;
    unsigned char c;
    if (!out || capacity == 0) return;
    if (!input) { out[0] = 0; return; }
    while (*input && len + 1U < capacity) {
        c = (unsigned char)*input++;
        if (c == '\n' || c == '\r' || c == '\t') c = ' ';
        if (c < 32 || c == 127) continue;
        out[len++] = (char)c;
    }
    while (len && out[len-1] == ' ') --len;
    out[len] = 0;
}
void nd_music_marquee(char *out, unsigned int capacity, const char *title,
                      unsigned int position, unsigned int width, int enabled)
{
    unsigned int i, n, cycle;
    if (!out || capacity == 0) return;
    out[0] = 0;
    if (!title) return;
    n = (unsigned int)strlen(title);
    if (!n) return;
    if (width + 1U > capacity) width = capacity - 1U;
    if (!enabled || n <= width) {
        if (n < width) width = n;
        memcpy(out, title, width);
        out[width] = 0;
        return;
    }
    cycle = n + 3U;
    position %= cycle;
    for (i = 0; i < width; ++i) {
        unsigned int index = (position + i) % cycle;
        out[i] = index < n ? title[index] : ' ';
    }
    out[width] = 0;
}
int nd_music_parse_volume(const struct NDMusicProfile *profile,
                          const char *text, unsigned int *percent)
{
    char *end;
    unsigned long raw;
    if (!profile || !text || !*text || !percent || !profile->volume_max)
        return 0;
    raw = strtoul(text, &end, 10);
    while (*end == ' ' || *end == '\t') ++end;
    if (*end || raw > profile->volume_max) return 0;
    *percent = (unsigned int)((raw * 100UL + profile->volume_max / 2U) /
                              profile->volume_max);
    if (*percent > 100U) *percent = 100U;
    return 1;
}
int nd_music_volume_command(const struct NDMusicProfile *profile,
                            unsigned int percent, char *out,
                            unsigned int capacity)
{
    unsigned long raw;
    int length;
    if (!profile || !out || !capacity || percent > 100U ||
        !profile->volume_max || !profile->extras[ND_MUSIC_SET_VOLUME][0])
        return 0;
    raw = (percent * (unsigned long)profile->volume_max + 50UL) / 100UL;
    length = sprintf(out, "%s %lu",
        profile->extras[ND_MUSIC_SET_VOLUME], raw);
    if (length <= 0 || (unsigned int)length >= capacity) {
        out[0] = 0;
        return 0;
    }
    return nd_music_valid_command(out);
}
int nd_music_parse_playing(const char *text, int *playing)
{
    int value;
    if (!text || !playing) return 0;
    while (*text==' ' || *text=='\t') ++text;
    if (*text=='0') value=0;
    else if (*text=='1') value=1;
    else return 0;
    ++text;
    while (*text==' ' || *text=='\t') ++text;
    if (*text) return 0;
    *playing=value;
    return 1;
}
