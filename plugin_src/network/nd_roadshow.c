#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "nd_roadshow.h"

void rd_config_defaults(struct RDRoadshowConfig *c)
{
    if (!c) return;
    memset(c, 0, sizeof(*c));
    c->poll_ticks = 250;
    c->dhcp_timeout = 60;
    c->ping_count = 4;
    strcpy(c->ping_host, "1.1.1.1");
}
int rd_valid_interface(const char *s)
{
    size_t i, n;
    if (!s) return 0;
    n = strlen(s);
    if (!n || n > RD_INTERFACE_MAX) return 0;
    for (i = 0; i < n; ++i) {
        unsigned char x = (unsigned char)s[i];
        if (!((x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z') ||
              (x >= '0' && x <= '9') || x == '-' || x == '_')) return 0;
    }
    return 1;
}
int rd_valid_host(const char *s)
{
    size_t i, n;
    int last_dot = 1;
    if (!s) return 0;
    n = strlen(s);
    if (!n || n > RD_HOST_MAX) return 0;
    for (i = 0; i < n; ++i) {
        unsigned char x = (unsigned char)s[i];
        if ((x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z') ||
            (x >= '0' && x <= '9')) { last_dot = 0; continue; }
        if (x == '.' && !last_dot) { last_dot = 1; continue; }
        if (x == '-' && i > 0 && s[i - 1] != '.' && i + 1 < n &&
            s[i + 1] != '.') { last_dot = 0; continue; }
        return 0;
    }
    return !last_dot;
}
static int text_equal_ci(const char *a, const char *b)
{
    while (*a && *b) {
        if (toupper((unsigned char)*a++) != toupper((unsigned char)*b++))
            return 0;
    }
    return *a == *b;
}
int rd_interface_filename(const char *filename)
{
    size_t n;
    if (!rd_valid_interface(filename)) return 0;
    n = strlen(filename);
    /* Valid names exclude dots already; no .info files can pass. */
    return n > 0;
}
static char *trim(char *s)
{
    char *end;
    while (*s && isspace((unsigned char)*s)) ++s;
    end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) --end;
    *end = '\0';
    return s;
}
int rd_interface_uses_dhcp(const char *path)
{
    char buf[256], *line, *eq, *key, *val;
    FILE *f;
    int enabled = 0;
    if (!path) return 0;
    f = fopen(path, "r");
    if (!f) return 0;
    while (fgets(buf, sizeof(buf), f)) {
        line = trim(buf);
        if (!*line || *line == '#' || *line == ';') continue;
        eq = strchr(line, '=');
        if (!eq) continue;
        *eq++ = '\0';
        key = trim(line); val = trim(eq);
        if (text_equal_ci(key, "configure"))
            enabled = text_equal_ci(val, "dhcp");
    }
    fclose(f);
    return enabled;
}
static int parse_uint(const char *s, unsigned int lo, unsigned int hi,
                      unsigned int *out)
{
    unsigned long n;
    char *end;
    if (!s || !*s || !isdigit((unsigned char)*s)) return 0;
    n = strtoul(s, &end, 10);
    if (*end || n < lo || n > hi) return 0;
    *out = (unsigned int)n;
    return 1;
}
int rd_apply_option(void *ctx, const char *key, const char *val)
{
    struct RDRoadshowConfig *c=(struct RDRoadshowConfig *)ctx;
    unsigned int v;
    if (!c || !key || !val) return 0;
    if (!strcmp(key,"INTERFACE")) {
        if (!rd_valid_interface(val)) return 0;
        strcpy(c->interface_name,val); return 1;
    }
    if (!strcmp(key,"POLL_TICKS")) {
        if (!parse_uint(val,100,3000,&v)) return 0;
        c->poll_ticks=v; return 1;
    }
    if (!strcmp(key,"DHCP_TIMEOUT")) {
        if (!parse_uint(val,10,300,&v)) return 0;
        c->dhcp_timeout=v;return 1;
    }
    if (!strcmp(key,"PING_HOST")) {
        if (!rd_valid_host(val)) return 0;
        strcpy(c->ping_host,val);return 1;
    }
    if (!strcmp(key,"PING_COUNT")) {
        if (!parse_uint(val,1,10,&v)) return 0;
        c->ping_count=v;return 1;
    }
    if (!strcmp(key,"ALLOW_SHUTDOWN")) {
        if (!strcmp(val,"YES")) c->allow_shutdown=1;
        else if (!strcmp(val,"NO")) c->allow_shutdown=0;
        else return 0;
        return 1;
    }
    return 0;
}
int rd_load_config(struct RDRoadshowConfig *cfg, const char *path)
{
    struct RDRoadshowConfig work;
    FILE *f;
    char buf[256], *line, *eq, *key, *val;
    int valid = 1, saw_if = 0;
    if (!cfg || !path) return 0;
    rd_config_defaults(&work);
    f = fopen(path, "r");
    if (!f) return 0;
    while (fgets(buf, sizeof(buf), f)) {
        if (!strchr(buf, '\n') && !feof(f)) { valid = 0; break; }
        line = trim(buf);
        if (!*line || *line == '#' || *line == ';') continue;
        eq = strchr(line, '=');
        if (!eq) { valid = 0; break; }
        *eq++ = '\0'; key = trim(line); val = trim(eq);
        if (!strcmp(key, "INTERFACE")) {
            if (!rd_valid_interface(val)) { valid = 0; break; }
            strcpy(work.interface_name, val); saw_if = 1;
        } else if (!strcmp(key, "POLL_TICKS")) {
            if (!parse_uint(val, 100, 3000, &work.poll_ticks)) { valid = 0; break; }
        } else if (!strcmp(key, "DHCP_TIMEOUT")) {
            if (!parse_uint(val, 10, 300, &work.dhcp_timeout)) { valid = 0; break; }
        } else if (!strcmp(key, "PING_HOST")) {
            if (!rd_valid_host(val)) { valid = 0; break; }
            strcpy(work.ping_host, val);
        } else if (!strcmp(key, "PING_COUNT")) {
            if (!parse_uint(val, 1, 10, &work.ping_count)) { valid = 0; break; }
        } else if (!strcmp(key, "ALLOW_SHUTDOWN")) {
            if (!strcmp(val, "YES")) work.allow_shutdown = 1;
            else if (!strcmp(val, "NO")) work.allow_shutdown = 0;
            else { valid = 0; break; }
        } else { valid = 0; break; }
    }
    if (ferror(f)) valid = 0;
    fclose(f);
    if (!valid || !saw_if) return 0;
    *cfg = work;
    return 1;
}
int rd_save_config(const struct RDRoadshowConfig *cfg, const char *path)
{
    char temp[256], backup[256];
    FILE *f;
    int had_original = 0, write_ok;
    if (!cfg || !path || strlen(path) > 240 ||
        !rd_valid_interface(cfg->interface_name) ||
        !rd_valid_host(cfg->ping_host) || cfg->poll_ticks < 100 ||
        cfg->poll_ticks > 3000 || cfg->dhcp_timeout < 10 ||
        cfg->dhcp_timeout > 300 || cfg->ping_count < 1 ||
        cfg->ping_count > 10 ||
        (cfg->allow_shutdown != 0 && cfg->allow_shutdown != 1)) return 0;
    sprintf(temp, "%s.new", path);
    sprintf(backup, "%s.bak", path);
    f = fopen(temp, "w");
    if (!f) return 0;
    write_ok = fprintf(f, "; NovaDock Roadshow 0.1\n"
        "INTERFACE=%s\nPOLL_TICKS=%u\nDHCP_TIMEOUT=%u\n"
        "PING_HOST=%s\nPING_COUNT=%u\nALLOW_SHUTDOWN=%s\n",
        cfg->interface_name, cfg->poll_ticks, cfg->dhcp_timeout,
        cfg->ping_host, cfg->ping_count,
        cfg->allow_shutdown ? "YES" : "NO") >= 0;
    if (fclose(f) != 0) write_ok = 0;
    if (!write_ok) { remove(temp); return 0; }
    f = fopen(path, "r");
    if (f) { fclose(f); had_original = 1; }
    if (had_original) {
        remove(backup);
        if (rename(path, backup) != 0) { remove(temp); return 0; }
    }
    if (rename(temp, path) != 0) {
        if (had_original) rename(backup, path);
        remove(temp);
        return 0;
    }
    return 1;
}
int rd_command(const struct RDRoadshowConfig *cfg, enum RDCommand action,
               char *dst, size_t size)
{
    char buf[192];
    if (!dst || !size || !cfg) return 0;
    dst[0] = '\0';
    switch (action) {
    case RD_CMD_STATUS:
        strcpy(buf, "C:GetNetStatus CHECK=INTERFACES QUIET"); break;
    case RD_CMD_DETAILS:
        strcpy(buf, "C:ShowNetStatus INTERFACES ROUTES DNS"); break;
    case RD_CMD_CONNECT:
        if (!rd_valid_interface(cfg->interface_name)) return 0;
        sprintf(buf, "C:AddNetInterface %s QUIET TIMEOUT=%u",
                cfg->interface_name, cfg->dhcp_timeout); break;
    case RD_CMD_DISCONNECT:
        if (!rd_valid_interface(cfg->interface_name)) return 0;
        sprintf(buf, "C:RemoveNetInterface %s", cfg->interface_name); break;
    case RD_CMD_DHCP:
        if (!rd_valid_interface(cfg->interface_name)) return 0;
        sprintf(buf, "C:ConfigureNetInterface %s CONFIGURE=DHCP TIMEOUT=%u QUIET",
                cfg->interface_name, cfg->dhcp_timeout); break;
    case RD_CMD_PING:
        if (!rd_valid_host(cfg->ping_host) ||
            cfg->ping_count < 1 || cfg->ping_count > 10) return 0;
        sprintf(buf, "C:Ping -c %u %s", cfg->ping_count, cfg->ping_host); break;
    case RD_CMD_SHUTDOWN:
        if (!cfg->allow_shutdown) return 0;
        strcpy(buf, "C:NetShutdown TIMEOUT=5 QUIET"); break;
    default: return 0;
    }
    if (strlen(buf) >= size) return 0;
    strcpy(dst, buf);
    return 1;
}
