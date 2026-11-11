#ifndef ND_NETWORK_H
#define ND_NETWORK_H
/* Portable config and packet-rate helper. No Amiga-only headers here. */
#define ND_NET_DEVICE_LEN 160
#define ND_NET_TICKS_PER_SECOND 50UL
struct NDNetConfig {
    char device[ND_NET_DEVICE_LEN];
    unsigned long unit;
    unsigned long scale_pps;
    unsigned long poll_ticks;
};
void nd_net_defaults(struct NDNetConfig *cfg);
int nd_net_apply_option(void *ctx, const char *key, const char *value);
int nd_net_parse_line(struct NDNetConfig *cfg, const char *line);
/* -1 means no file, otherwise count of invalid lines. */
int nd_net_load_config(struct NDNetConfig *cfg, const char *path);
/* Return false for corrupt/implausible/reset samples; handles 32-bit wraps. */
int nd_net_rates(unsigned long old_rx, unsigned long old_tx,
                 unsigned long new_rx, unsigned long new_tx,
                 unsigned long elapsed_ticks, unsigned long *rx_pps,
                 unsigned long *tx_pps);
unsigned int nd_net_channel_meter(unsigned long pps, unsigned long scale_pps);
unsigned int nd_net_meter(unsigned long rx_pps, unsigned long tx_pps,
                          unsigned long scale_pps);
#endif
