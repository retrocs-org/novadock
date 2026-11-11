#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_folder.h"
#include "nd_search.h"

static void set(struct NDFolderEntry *e,const char *name,int drawer,
                unsigned long size,long days)
{
    memset(e,0,sizeof(*e));
    strcpy(e->name,name);e->drawer=drawer;e->size=size;e->days=days;
}
int main(void)
{
    struct NDFolderEntry e[8];
    struct NDFolderLayout l;
    char path[ND_FOLDER_PATH_LEN];
    int i;
    assert(nd_search_contains("MultiView documents","VIEW"));
    assert(!nd_search_contains("MultiView","longer text"));
    assert(nd_search_contains("anything",""));
    assert(nd_folder_visible_name("Prefs"));
    assert(!nd_folder_visible_name("Prefs.info"));
    assert(!nd_folder_visible_name("Disk.INFO"));
    assert(!nd_folder_visible_name(".backdrop"));
    assert(nd_folder_visible_name("info"));
    set(&e[0],"zeta",0,10,5);
    set(&e[1],"Alpha",0,500,1);
    set(&e[2],"Tools",1,0,3);
    set(&e[3],"beta",0,90,9);
    set(&e[4],"Apps",1,0,7);
    nd_folder_sort(e,5,ND_FOLDER_SORT_NAME);
    assert(!strcmp(e[0].name,"Apps") && !strcmp(e[1].name,"Tools"));
    assert(!strcmp(e[2].name,"Alpha") && !strcmp(e[3].name,"beta") &&
           !strcmp(e[4].name,"zeta"));
    nd_folder_sort(e,5,ND_FOLDER_SORT_DATE);
    assert(!strcmp(e[0].name,"Apps") && !strcmp(e[2].name,"beta") &&
           !strcmp(e[4].name,"Alpha"));
    nd_folder_sort(e,5,ND_FOLDER_SORT_SIZE);
    assert(e[0].drawer && e[1].drawer && !strcmp(e[2].name,"Alpha") &&
           !strcmp(e[4].name,"zeta"));
    assert(!strcmp(nd_folder_sort_name(ND_FOLDER_SORT_SIZE),"Size"));
    nd_folder_sort(e,5,ND_FOLDER_SORT_NAME);
    assert(nd_folder_find_prefix(e,5,"t",0)==1);
    assert(nd_folder_find_prefix(e,5,"A",0)==0);
    assert(nd_folder_find_prefix(e,5,"a",1)==2);   /* wraps on from start */
    assert(nd_folder_find_prefix(e,5,"BE",0)==3);
    assert(nd_folder_find_prefix(e,5,"q",0)==-1);
    /* Paths and nested navigation bounded by the tile's drawer. */
    assert(nd_folder_join(path,sizeof(path),"Work:","Apps") && !strcmp(path,"Work:Apps"));
    assert(nd_folder_join(path,sizeof(path),"Work:Apps","GoldED") &&
           !strcmp(path,"Work:Apps/GoldED"));
    assert(nd_folder_parent(path,"Work:") && !strcmp(path,"Work:Apps"));
    assert(nd_folder_parent(path,"Work:") && !strcmp(path,"Work:"));
    assert(!nd_folder_parent(path,"Work:"));
    strcpy(path,"Work:Apps/GoldED");
    assert(nd_folder_parent(path,"work:apps") && !strcmp(path,"Work:Apps"));
    assert(!nd_folder_parent(path,"work:apps/"));
    /* Layout: list and grid. */
    nd_folder_layout(ND_FOLDER_VIEW_LIST,5,400,&l);
    assert(l.columns==1 && l.visible_rows==5 && l.width==240);
    nd_folder_layout(ND_FOLDER_VIEW_LIST,60,200,&l);
    assert(l.visible_rows==(200-18-8)/15 && l.total_rows==60);
    nd_folder_layout(ND_FOLDER_VIEW_ICONS,12,400,&l);
    assert(l.columns==5 && l.total_rows==3 && l.visible_rows==3 && l.width==328);
    nd_folder_layout(ND_FOLDER_VIEW_ICONS,2,400,&l);
    assert(l.columns==2 && l.width==240);
    /* Hit testing: header zones and cells. */
    nd_folder_layout(ND_FOLDER_VIEW_ICONS,12,400,&l);
    assert(nd_folder_hit(&l,12,0,1,5,5)==ND_FOLDER_HIT_UP);
    assert(nd_folder_hit(&l,12,0,0,5,5)==ND_FOLDER_HIT_TITLE);
    assert(nd_folder_hit(&l,12,0,1,l.width-5,5)==ND_FOLDER_HIT_VIEW);
    assert(nd_folder_hit(&l,12,0,1,l.width-40,5)==ND_FOLDER_HIT_SORT);
    assert(nd_folder_hit(&l,12,0,1,4+64+1,18+4+48+1)==6);
    assert(nd_folder_hit(&l,12,0,1,4+4*64+1,18+4+2*48+1)==ND_FOLDER_HIT_NONE);
    assert(nd_folder_hit(&l,12,0,1,-1,30)==ND_FOLDER_HIT_NONE);
    /* Scrolling keeps the focused entry visible. */
    nd_folder_layout(ND_FOLDER_VIEW_LIST,60,200,&l);
    i=nd_folder_scroll_to(&l,40,0);
    assert(40>=i && 40<i+l.visible_rows);
    assert(nd_folder_scroll_to(&l,0,i)==0);
    assert(nd_folder_scroll_to(&l,59,0)==60-l.visible_rows);
    assert(nd_folder_hit(&l,60,i,1,10,18+4+1)==i);
    /* Cursor movement. */
    nd_folder_layout(ND_FOLDER_VIEW_ICONS,12,400,&l);
    assert(nd_folder_move(&l,12,0,1,0)==1);
    assert(nd_folder_move(&l,12,1,0,1)==6);
    assert(nd_folder_move(&l,12,8,0,1)==8);   /* no row below: stay */
    assert(nd_folder_move(&l,12,11,1,0)==11);
    assert(nd_folder_move(&l,12,2,0,-1)==2);
    assert(nd_folder_move(&l,12,-1,1,0)==0);
    puts("Drawer popup sorting, type-ahead, navigation and layout passed.");
    return 0;
}
