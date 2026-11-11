#ifndef ND_TASKS_H
#define ND_TASKS_H
/* Taskbar mode: running applications that have no launcher in the dock are
 * shown as temporary tiles.  The AmigaOS side walks the Intuition screen and
 * window lists and groups windows by owning task; this portable module picks
 * labels, filters and builds the runtime-only tiles. */
#include "nd_config.h"

#define ND_TASK_MAX 6
#define ND_TASK_NAME_LEN 32

struct NDTaskEntry {
    unsigned long key;            /* owning task address; identifies the tile */
    char label[ND_TASK_NAME_LEN]; /* name shown on the tile */
    char path[ND_COMMAND_LEN];    /* absolute program path if known, else "" */
    int windows;                  /* number of windows the task owns */
};

/* Best tile label: the CLI command's base name when the process runs one,
 * otherwise the Exec task name unless it is a generic shell/handler name,
 * otherwise the first window title.  Returns 0 if nothing usable exists. */
int nd_task_label(const char *task_name,const char *command_name,
                  const char *window_title,char *out,unsigned int size);
/* 1 if LABEL appears (case-insensitively) in the comma-separated LIST. */
int nd_task_ignored(const char *label,const char *list);
/* Sort by label (then key) and drop duplicate keys; returns the count. */
int nd_tasks_normalize(struct NDTaskEntry *tasks,int count);
int nd_tasks_same(const struct NDTaskEntry *a,int a_count,
                  const struct NDTaskEntry *b,int b_count);
/* Append ND_TRANSIENT_TASK tiles to a compacted runtime CFG, before any
 * media tiles that are appended later.  The tile's id holds the task key as
 * "T<hex>", launch_args the program path ("" when unknown).  Returns the
 * number of tiles added. */
int nd_tasks_append(struct NDConfig *cfg,const struct NDTaskEntry *tasks,
                    int count);
/* Task key encoded in a task tile, or 0. */
unsigned long nd_task_tile_key(const struct NDItem *item);
#endif
