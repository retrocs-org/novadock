/* Portable drawer-popup model; see nd_folder.h. */
#include <ctype.h>
#include <string.h>
#include "nd_folder.h"
#include "nd_search.h"

#define LIST_WIDTH 240
#define LIST_ROW 15
#define GRID_CELL_W 64
#define GRID_CELL_H 48
#define GRID_MAX_COLUMNS 5
#define HEADER_H 18
#define BORDER 4
#define UP_ZONE 30
#define VIEW_ZONE 24
#define SORT_ZONE 44

static int compare_names(const char *a,const char *b)
{
    int ca,cb;
    for (;;) {
        ca=tolower((unsigned char)*a++);
        cb=tolower((unsigned char)*b++);
        if (ca!=cb || !ca) return ca-cb;
    }
}
int nd_folder_visible_name(const char *name)
{
    size_t n;
    if (!name || !*name || name[0]=='.') return 0;
    n=strlen(name);
    if (n>=5) {
        const char *s=name+n-5;
        if (s[0]=='.' && tolower((unsigned char)s[1])=='i' &&
            tolower((unsigned char)s[2])=='n' &&
            tolower((unsigned char)s[3])=='f' &&
            tolower((unsigned char)s[4])=='o') return 0;
    }
    return 1;
}
static int compare(const struct NDFolderEntry *a,const struct NDFolderEntry *b,
                   int mode)
{
    if (a->drawer!=b->drawer) return a->drawer ? -1 : 1;
    if (mode==ND_FOLDER_SORT_DATE) {
        if (a->days!=b->days) return a->days>b->days ? -1 : 1;
        if (a->minutes!=b->minutes) return a->minutes>b->minutes ? -1 : 1;
        if (a->ticks!=b->ticks) return a->ticks>b->ticks ? -1 : 1;
    } else if (mode==ND_FOLDER_SORT_SIZE && !a->drawer) {
        if (a->size!=b->size) return a->size>b->size ? -1 : 1;
    }
    return compare_names(a->name,b->name);
}
void nd_folder_sort(struct NDFolderEntry *entries,int count,int mode)
{
    /* Insertion sort: tiny lists, stable, no recursion on a small stack. */
    int i,j;
    struct NDFolderEntry t;
    if (!entries) return;
    for (i=1;i<count;++i) {
        t=entries[i];
        for (j=i;j>0 && compare(&entries[j-1],&t,mode)>0;--j)
            entries[j]=entries[j-1];
        entries[j]=t;
    }
}
const char *nd_folder_sort_name(int mode)
{
    if (mode==ND_FOLDER_SORT_DATE) return "Date";
    if (mode==ND_FOLDER_SORT_SIZE) return "Size";
    return "Name";
}
int nd_folder_find_prefix(const struct NDFolderEntry *entries,int count,
                          const char *prefix,int start)
{
    int i,k;
    size_t n,j;
    if (!entries || count<=0 || !prefix || !*prefix) return -1;
    n=strlen(prefix);
    if (start<0 || start>=count) start=0;
    for (k=0;k<count;++k) {
        i=(start+k)%count;
        for (j=0;j<n;++j)
            if (tolower((unsigned char)entries[i].name[j])!=
                tolower((unsigned char)prefix[j])) break;
        if (j==n) return i;
    }
    for(k=0;k<count;++k){i=(start+k)%count;if(nd_search_contains(entries[i].name,prefix))return i;}
    return -1;
}
int nd_folder_join(char *out,unsigned int size,const char *dir,const char *name)
{
    size_t a,b;
    int sep;
    if (!out || !dir || !name || !*dir || !*name) return 0;
    a=strlen(dir);b=strlen(name);
    sep=dir[a-1]!=':' && dir[a-1]!='/';
    if (a+b+(size_t)sep+1>size) return 0;
    strcpy(out,dir);
    if (sep) strcat(out,"/");
    strcat(out,name);
    return 1;
}
static int same_path(const char *a,const char *b)
{
    size_t la=strlen(a),lb=strlen(b);
    while (la && a[la-1]=='/') --la;
    while (lb && b[lb-1]=='/') --lb;
    if (la!=lb) return 0;
    while (la--) if (tolower((unsigned char)*a++)!=tolower((unsigned char)*b++))
        return 0;
    return 1;
}
int nd_folder_parent(char *path,const char *root)
{
    char *cut;
    size_t n;
    if (!path || !root || same_path(path,root)) return 0;
    n=strlen(path);
    while (n && path[n-1]=='/') path[--n]='\0';
    cut=strrchr(path,'/');
    if (cut) *cut='\0';
    else {
        cut=strchr(path,':');
        if (!cut || !cut[1]) return 0;
        cut[1]='\0';
    }
    return 1;
}
void nd_folder_layout(int view,int count,int max_height,
                      struct NDFolderLayout *layout)
{
    int rows,fit;
    memset(layout,0,sizeof(*layout));
    layout->view=view;
    layout->header_height=HEADER_H;
    if (count<1) count=1;
    if (view==ND_FOLDER_VIEW_ICONS) {
        layout->cell_width=GRID_CELL_W;
        layout->cell_height=GRID_CELL_H;
        layout->columns=count<GRID_MAX_COLUMNS ? count : GRID_MAX_COLUMNS;
        if (layout->columns<4 && count>=4) layout->columns=4;
        layout->icon_width=GRID_CELL_W-8;
        layout->icon_height=GRID_CELL_H-16;
    } else {
        layout->cell_width=LIST_WIDTH-2*BORDER;
        layout->cell_height=LIST_ROW;
        layout->columns=1;
    }
    rows=(count+layout->columns-1)/layout->columns;
    layout->total_rows=rows;
    fit=(max_height-HEADER_H-2*BORDER)/layout->cell_height;
    if (fit<1) fit=1;
    layout->visible_rows=rows<fit ? rows : fit;
    layout->width=layout->columns*layout->cell_width+2*BORDER;
    if (layout->width<LIST_WIDTH) layout->width=LIST_WIDTH;
    layout->height=HEADER_H+layout->visible_rows*layout->cell_height+2*BORDER;
}
int nd_folder_hit(const struct NDFolderLayout *layout,int count,int offset,
                  int can_go_up,int x,int y)
{
    int row,column,index;
    if (!layout || x<0 || y<0 || x>=layout->width || y>=layout->height)
        return ND_FOLDER_HIT_NONE;
    if (y<layout->header_height) {
        if (can_go_up && x<UP_ZONE) return ND_FOLDER_HIT_UP;
        if (x>=layout->width-VIEW_ZONE) return ND_FOLDER_HIT_VIEW;
        if (x>=layout->width-VIEW_ZONE-SORT_ZONE) return ND_FOLDER_HIT_SORT;
        return ND_FOLDER_HIT_TITLE;
    }
    x-=BORDER;y-=layout->header_height+BORDER;
    if (x<0 || y<0) return ND_FOLDER_HIT_NONE;
    column=x/layout->cell_width;
    row=y/layout->cell_height;
    if (column>=layout->columns || row>=layout->visible_rows)
        return ND_FOLDER_HIT_NONE;
    index=(offset+row)*layout->columns+column;
    return index<count ? index : ND_FOLDER_HIT_NONE;
}
int nd_folder_scroll_to(const struct NDFolderLayout *layout,int index,
                        int offset)
{
    int row,max_offset;
    if (!layout || index<0) return offset;
    row=index/layout->columns;
    if (row<offset) offset=row;
    if (row>=offset+layout->visible_rows) offset=row-layout->visible_rows+1;
    max_offset=layout->total_rows-layout->visible_rows;
    if (offset>max_offset) offset=max_offset;
    if (offset<0) offset=0;
    return offset;
}
int nd_folder_move(const struct NDFolderLayout *layout,int count,int current,
                   int dx,int dy)
{
    int next;
    if (!layout || count<=0) return -1;
    if (current<0) return 0;
    next=current+dx+dy*layout->columns;
    if (next<0) next=dy<0 ? current%layout->columns : 0;
    if (next>=count) next=dy>0 ? current : count-1;
    if (next>=count) next=count-1;
    return next;
}
