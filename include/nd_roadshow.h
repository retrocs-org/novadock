#ifndef ND_ROADSHOW_H
#define ND_ROADSHOW_H
/* Portable, shell-safe Roadshow config/command layer; C89, no Amiga headers. */
#include <stddef.h>
#define RD_INTERFACE_MAX 15
#define RD_HOST_MAX 63
#define RD_MAX_INTERFACES 16
#define RD_MAX_DETAIL_LINES 48
#define RD_DETAIL_COLS 82
struct RDRoadshowConfig {
    char interface_name[RD_INTERFACE_MAX + 1];
    char ping_host[RD_HOST_MAX + 1];
    unsigned int poll_ticks;
    unsigned int dhcp_timeout;
    unsigned int ping_count;
    int allow_shutdown;
};
enum RDCommand {
    RD_CMD_STATUS = 0, RD_CMD_CONNECT, RD_CMD_DISCONNECT,
    RD_CMD_SHUTDOWN, RD_CMD_DETAILS, RD_CMD_DHCP, RD_CMD_PING
};
void rd_config_defaults(struct RDRoadshowConfig *cfg);
int rd_apply_option(void *ctx, const char *key, const char *value);
int rd_valid_interface(const char *s);
int rd_valid_host(const char *s);
/* Only valid, ordinary config files; skip Workbench .info and directories. */
int rd_interface_filename(const char *filename);
/* Determine whether a Roadshow interface text file requests DHCP. */
int rd_interface_uses_dhcp(const char *path);
/* Load strictly; invalid existing configs must not be silently overwritten. */
int rd_load_config(struct RDRoadshowConfig *cfg, const char *path);
int rd_save_config(const struct RDRoadshowConfig *cfg, const char *path);
/* Generate command using a fixed C: program and validated arguments only. */
int rd_command(const struct RDRoadshowConfig *cfg, enum RDCommand action,
               char *dst, size_t size);
#endif
