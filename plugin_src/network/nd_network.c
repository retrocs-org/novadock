/* Portable network configuration parser and packet rate calculations (C89). */
#include "nd_network.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *trim(char *p)
{
    char *e;
    while (*p == ' ' || *p == '\t') ++p;
    e = p + strlen(p);
    while (e > p && (e[-1] == ' ' || e[-1] == '\t' ||
                     e[-1] == '\r' || e[-1] == '\n')) --e;
    *e = '\0';
    return p;
}
static int unsigned_value(const char *s, unsigned long lo,
                          unsigned long hi, unsigned long *val)
{
    char *end;
    unsigned long n;
    if (!*s || !isdigit((unsigned char)*s)) return 0;
    n = strtoul(s, &end, 10);
    if (*end || n < lo || n > hi) return 0;
    *val = n;
    return 1;
}
void nd_net_defaults(struct NDNetConfig *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    cfg->scale_pps = 250UL;
    cfg->poll_ticks = 50UL;
}
int nd_net_parse_line(struct NDNetConfig *cfg, const char *line)
{
    char buf[256], *key, *value;
    unsigned long n;
    if (strlen(line) >= sizeof(buf)) return 0;
    strcpy(buf, line);
    key = trim(buf);
    if (!*key || *key == '#' || *key == ';') return 1;
    value = strchr(key, '=');
    if (!value) return 0;
    *value++ = '\0';
    key = trim(key);
    value = trim(value);
    if (!strcmp(key, "DEVICE")) {
        /* Spaces are allowed in an AmigaDOS path, but never line breaks. */
        if (strlen(value) >= sizeof(cfg->device)) return 0;
        strcpy(cfg->device, value);
        return 1;
    }
    if (!strcmp(key, "UNIT")) {
        if (!unsigned_value(value, 0UL, 255UL, &n)) return 0;
        cfg->unit = n;
        return 1;
    }
    if (!strcmp(key, "SCALE_PPS")) {
        if (!unsigned_value(value, 1UL, 1000000UL, &n)) return 0;
        cfg->scale_pps = n;
        return 1;
    }
    if (!strcmp(key, "POLL_TICKS")) {
        if (!unsigned_value(value, 10UL, 250UL, &n)) return 0;
        cfg->poll_ticks = n;
        return 1;
    }
    return 0;
}
int nd_net_load_config(struct NDNetConfig *cfg, const char *path)
{
    FILE *f;
    char buf[256];
    int invalid = 0;
    nd_net_defaults(cfg);
    f = fopen(path, "r");
    if (!f) return -1;
    while (fgets(buf, sizeof(buf), f)) {
        if (!strchr(buf, '\n') && strlen(buf) == sizeof(buf) - 1U) {
            int ch;
            while ((ch = fgetc(f)) != EOF && ch != '\n') {}
            ++invalid;
        } else if (!nd_net_parse_line(cfg, buf)) ++invalid;
    }
    fclose(f);
    return invalid;
}
int nd_net_apply_option(void *ctx, const char *key, const char *value)
{
    char line[260];
    if (!key || !value || strlen(key)+strlen(value)+2>=sizeof(line)) return 0;
    sprintf(line,"%s=%s",key,value);
    return nd_net_parse_line((struct NDNetConfig *)ctx,line);
}
int nd_net_rates(unsigned long old_rx, unsigned long old_tx,
                 unsigned long new_rx, unsigned long new_tx,
                 unsigned long elapsed_ticks, unsigned long *rx_pps,
                 unsigned long *tx_pps)
{
    unsigned long dr, dt;
    if (!rx_pps || !tx_pps || !elapsed_ticks ||
        elapsed_ticks > 5000UL) return 0;
    /* 68000 ULONG is 32 bits. Unsigned subtraction handles natural wrap. */
    dr = (new_rx - old_rx) & 0xffffffffUL;
    dt = (new_tx - old_tx) & 0xffffffffUL;
    /* Discard counter resets / nonsensical spikes, don't display fake rates. */
    if (dr > 50000000UL || dt > 50000000UL) return 0;
    *rx_pps = (dr * ND_NET_TICKS_PER_SECOND) / elapsed_ticks;
    *tx_pps = (dt * ND_NET_TICKS_PER_SECOND) / elapsed_ticks;
    return 1;
}
unsigned int nd_net_channel_meter(unsigned long pps,
                                  unsigned long scale_pps)
{
    if (!scale_pps || pps >= scale_pps) return 100U;
    /* Config limits scale to 1,000,000; pps < scale here. */
    return (unsigned int)((pps * 100UL) / scale_pps);
}
unsigned int nd_net_meter(unsigned long rx_pps, unsigned long tx_pps,
                          unsigned long scale_pps)
{
    return nd_net_channel_meter(rx_pps > tx_pps ? rx_pps : tx_pps,
                                scale_pps);
}
