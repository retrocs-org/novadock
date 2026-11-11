#ifndef ND_INTERACTION_H
#define ND_INTERACTION_H
#include <stddef.h>
#define ND_RX_INVALID 0
#define ND_RX_PING 1
#define ND_RX_VERSION 2
#define ND_RX_STATUS 3
#define ND_RX_LIST 4
#define ND_RX_OPEN 5
#define ND_RX_MOVE 6
#define ND_RX_RELOAD 7
#define ND_RX_PREFS 8
#define ND_RX_QUIT 9
#define ND_RX_HELP 10
#define ND_RX_SHOW 11
#define ND_RX_HIDE 12
#define ND_RX_TOGGLE 13
#define ND_RX_LOCK 14
#define ND_RX_UNLOCK 15
#define ND_RX_CLICK 16    /* CLICK n: send a click to a plugin tile */
#define ND_RX_REMOVE 17   /* REMOVE n: as dragging the tile out (Edit mode rules) */
#define ND_RX_ADD 18      /* ADD path: add an application or drawer launcher */
#define ND_RX_SETVALUE 19 /* SETVALUE id "caption" "value" [meter] */
#define ND_RX_SETBADGE 20 /* SETBADGE id ["text"]: tile badge; none clears */
#define ND_RX_SUSPEND 21   /* SUSPEND: close the dock's windows (Workbench reset) */
#define ND_RX_RESUME 22    /* RESUME: reopen them */
#define ND_RX_BADGE_LEN 8  /* as ND_BADGE_LEN in nd_protocol.h */
#define ND_RX_TEXT_LEN 240
#define ND_RX_ID_LEN 20
#define ND_RX_STEM_LEN 32 /* LIST STEM name, including the final '.' */
/* first/second are 1-based indices; OPEN accepts a label instead of an
 * index (first is then 0 and text holds the label). */
struct NDRexxCommand {
    int action; int first; int second;
    int meter;                 /* SETVALUE: 0..100, or -1 for none */
    char id[ND_RX_ID_LEN];     /* SETVALUE/SETBADGE plugin id */
    char text[ND_RX_TEXT_LEN]; /* OPEN label, ADD path, SETVALUE caption, SETBADGE text, LIST stem */
    char text2[ND_RX_TEXT_LEN];/* SETVALUE value */
};
/* C89/testable boundary for ARexx parsing; numeric indices are 1-based. */
int nd_parse_rexx(const char *text, struct NDRexxCommand *command);
/* Return 1 for safe shell invocations: quoted tool and one dropped file. */
int nd_build_openwith(char *dest, size_t limit, const char *tool,
                      const char *file);
/* Extended Launcher Action form: stored arguments remain between the quoted
 * tool path and the safely quoted dropped object. */
int nd_build_openwith_action(char *dest,size_t limit,const char *tool,
                             const char *arguments,const char *file);
/* The middle three fifths of a tile are its action drop target.  The outer
 * fifths remain insertion zones while Edit mode is active. */
int nd_drop_opens_item(int position,int extent);
/* Return 1 when an insertion pointer is on the trailing half of a tile.
 * Horizontal docks pass X/width; vertical docks pass Y/height. */
int nd_drop_after_item(int position,int extent);
/* Convert a tile edge choice to the final list position expected by the
 * config mover. This also preserves row-boundary moves whose list order does
 * not change (for example, moving the last item of row 1 before row 2). */
int nd_drop_destination(int from,int target,int after);
/* Validate the exact Classic Amiga raw-input event used by the inactive-dock
 * context-menu fallback. Pointer motion and button-up events must not match. */
int nd_right_button_down(unsigned int event_class,unsigned int event_code);
/* NewShell is only a short-lived starter.  The persistent window/process is
 * normally titled AmigaShell, so task-name tracking needs this narrow alias. */
int nd_launcher_matches_shell_window(const char *command,const char *title);
/* Case-insensitive '*'/'?' matcher used by launcher window identities. */
int nd_text_matches_pattern(const char *pattern,const char *text);
/* Quote PATH for the AmigaDOS shell and ReadArgs(): the result is wrapped in
 * double quotes, '*' becomes "**" and '"' becomes "*\"" (the AmigaDOS escape
 * character). Control characters are rejected. Returns 0 if DEST is too
 * small or PATH is unusable. */
int nd_quote_path(char *dest,size_t limit,const char *path);
/* COMMAND EVENT "DETAIL" for a dock event hook (DETAIL may be empty).
 * Returns 0 if it does not fit in LIMIT bytes. */
int nd_hook_command(char *dest,size_t limit,const char *command,
                    const char *event,const char *detail);
/* Join DIR and NAME with '/' unless DIR already ends in ':' or '/'. */
int nd_join_path(char *dest,size_t limit,const char *dir,const char *name);
/* C:Copy command line that copies SOURCE (a file or drawer) into DRAWER.
 * A drawer is copied as a new sub-drawer of the same name. */
int nd_build_copy_command(char *dest,size_t limit,const char *source,
                          int source_is_drawer,const char *drawer);
/* Compare the program part of a CLI command name (which may be a full path,
 * as stored in cli_CommandName) with WANTED, ignoring case. */
int nd_command_name_matches(const char *command_name,const char *wanted);
#endif
