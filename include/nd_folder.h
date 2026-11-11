#ifndef ND_FOLDER_H
#define ND_FOLDER_H
/* Drawer popups: portable model for listing, sorting, type-ahead, nested
 * navigation and the list/icon-grid layout.  The AmigaOS side reads the
 * directory, loads icons and draws. */
#define ND_FOLDER_MAX 64
#define ND_FOLDER_NAME_LEN 108   /* full AmigaDOS file name */
#define ND_FOLDER_PATH_LEN 240   /* ND_COMMAND_LEN */

#define ND_FOLDER_SORT_NAME 0    /* drawers first, then A-Z */
#define ND_FOLDER_SORT_DATE 1    /* drawers first, then newest first */
#define ND_FOLDER_SORT_SIZE 2    /* drawers first, then largest first */
#define ND_FOLDER_SORT_COUNT 3
#define ND_FOLDER_VIEW_ICONS 0
#define ND_FOLDER_VIEW_LIST 1

/* Hit-test results other than an entry index. */
#define ND_FOLDER_HIT_NONE (-1)
#define ND_FOLDER_HIT_UP (-2)
#define ND_FOLDER_HIT_TITLE (-3) /* opens the shown drawer in Workbench */
#define ND_FOLDER_HIT_SORT (-4)
#define ND_FOLDER_HIT_VIEW (-5)

struct NDFolderEntry {
    char name[ND_FOLDER_NAME_LEN];
    int drawer;
    unsigned long size;
    long days, minutes, ticks;   /* AmigaDOS DateStamp */
};

struct NDFolderLayout {
    int view;
    int width, height;
    int header_height;
    int cell_width, cell_height; /* one entry */
    int columns;                 /* 1 in list view */
    int visible_rows;
    int total_rows;
    int icon_width, icon_height; /* icon box inside a grid cell */
};

/* Workbench-style visibility: no .info files and no names starting '.'. */
int nd_folder_visible_name(const char *name);
void nd_folder_sort(struct NDFolderEntry *entries,int count,int mode);
const char *nd_folder_sort_name(int mode);
/* First entry from START (wrapping) whose name begins with PREFIX, ignoring
 * case; -1 if none. */
int nd_folder_find_prefix(const struct NDFolderEntry *entries,int count,
                          const char *prefix,int start);
/* Join DIR and NAME (AmigaDOS rules). */
int nd_folder_join(char *out,unsigned int size,const char *dir,const char *name);
/* Go one level up from PATH, but never above ROOT.  Returns 0 at ROOT. */
int nd_folder_parent(char *path,const char *root);
/* Size the popup for COUNT entries; MAX_HEIGHT limits visible rows. */
void nd_folder_layout(int view,int count,int max_height,
                      struct NDFolderLayout *layout);
/* Map popup-relative X,Y to an entry index (first visible row is OFFSET),
 * a header zone, or ND_FOLDER_HIT_NONE.  CAN_GO_UP enables the Up zone. */
int nd_folder_hit(const struct NDFolderLayout *layout,int count,int offset,
                  int can_go_up,int x,int y);
/* Top row offset that keeps entry INDEX visible. */
int nd_folder_scroll_to(const struct NDFolderLayout *layout,int index,
                        int offset);
/* Entry reached from CURRENT by a cursor key: dx/dy in cells. */
int nd_folder_move(const struct NDFolderLayout *layout,int count,int current,
                   int dx,int dy);
#endif
