#ifndef ND_COMMAND_H
#define ND_COMMAND_H
/* Command-output tile: portable settings, script and output handling.
 * The plugin runs the command in an asynchronous shell whose output goes to
 * a T: file, so neither the dock nor the plugin ever blocks on it. */
#define ND_CMD_TEXT_LEN 32       /* matches the dock's tile text */
#define ND_CMD_COMMAND_LEN 96    /* matches ND_OPT_VALUE_LEN */
#define ND_CMD_METER_NONE 0
#define ND_CMD_METER_PERCENT 1
#define ND_CMD_CLICK_RERUN 0
#define ND_CMD_CLICK_NONE 1
#define ND_CMD_CLICK_COMMAND 2

struct NDCommandConfig {
    char command[ND_CMD_COMMAND_LEN];
    char caption[ND_CMD_TEXT_LEN];
    char click_command[ND_CMD_COMMAND_LEN];
    unsigned int interval;  /* seconds between runs */
    unsigned int line;      /* 1-based non-empty output line to show */
    unsigned int timeout;   /* seconds before the tile reports TIMEOUT */
    int meter;              /* ND_CMD_METER_* */
    int click;              /* ND_CMD_CLICK_* */
};

void nd_cmd_defaults(struct NDCommandConfig *cfg);
/* NDOptionApply-compatible: 1 accepted, 0 invalid. */
int nd_cmd_apply_option(void *ctx,const char *key,const char *value);
/* Temporary file names for plugin ID: T:NovaDockCmd-<ID>.out / .done */
int nd_cmd_file_names(const char *id,char *out_file,char *done_file,
                      unsigned int size);
/* Shell script: never stops on errors, sets a 16 KB stack, runs COMMAND,
 * then records its return code in DONE_FILE. */
int nd_cmd_build_script(const char *command,const char *done_file,
                        char *script,unsigned int size);
/* Pick output line LINE (counting non-empty lines), remove console escape
 * sequences and control characters, collapse spaces and fit VALUE.  With
 * ND_CMD_METER_PERCENT the first number in that line (clamped 0..100)
 * becomes *METER, otherwise *METER is 65535.  Returns 1 if a line exists. */
int nd_cmd_extract(const char *output,unsigned int line,int meter_mode,
                   char *value,unsigned int size,unsigned int *meter);
/* Return code written by the script's final Echo, or -1. */
long nd_cmd_parse_rc(const char *text);
#endif
