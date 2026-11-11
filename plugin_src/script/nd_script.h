#ifndef ND_SCRIPT_H
#define ND_SCRIPT_H
/* Portable helpers for Script tiles: building the ARexx command line and
 * reading the script's RESULT. */
#define ND_SCRIPT_TEXT_LEN 32   /* as ND_TEXT_LEN */
#define ND_SCRIPT_BADGE_LEN 8
#define ND_SCRIPT_NO_METER 255U
struct NDScriptResult {
    int has_caption,has_meter,has_badge;
    char caption[ND_SCRIPT_TEXT_LEN];
    char value[ND_SCRIPT_TEXT_LEN];
    unsigned int meter;            /* 0..100, or ND_SCRIPT_NO_METER */
    char badge[ND_SCRIPT_BADGE_LEN];
};
/* RESULT forms: "value", "caption|value", "caption|value|meter" and
 * "caption|value|meter|badge".  An empty caption keeps the configured one;
 * an empty or "-" meter shows none; an empty badge removes it.  Control
 * characters become spaces and long fields are cut.  Returns 0 only for
 * more than four fields or a meter that is not 0-100. */
int nd_script_parse(const char *result,struct NDScriptResult *out);
/* "<script>" VERB ID, the script quoted when it contains a space.
 * Returns 0 if it does not fit. */
int nd_script_command(char *out,unsigned int size,const char *script,
                      const char *verb,const char *id);
/* Keep IPC heartbeats independent of the script refresh interval. */
unsigned int nd_script_wait_ticks(int running,unsigned int since,
                                   unsigned int interval,int immediate);
#endif
