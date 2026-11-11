#include <string.h>
#include "nd_layout_profiles.h"

int nd_layout_name_valid(const char *name)
{
    unsigned int i,n;
    if (!name) return 0;
    n=(unsigned int)strlen(name);
    if (!n || n>=ND_LAYOUT_NAME_LEN || name[0]==' ' || name[n-1]==' ')
        return 0;
    for (i=0;i<n;++i) {
        char c=name[i];
        if (!((c>='a' && c<='z') || (c>='A' && c<='Z') ||
              (c>='0' && c<='9') || c==' ' || c=='-' || c=='_')) return 0;
    }
    return 1;
}

int nd_layout_profile_path(char *path,unsigned int path_size,
                           const char *root,const char *name)
{
    unsigned int needed;
    if (!path || !path_size || !root || !nd_layout_name_valid(name)) return 0;
    needed=(unsigned int)(strlen(root)+strlen(name)+6);
    if (needed>path_size) return 0;
    strcpy(path,root);
    if (*root && root[strlen(root)-1]!='/' && root[strlen(root)-1]!=':')
        strcat(path,"/");
    strcat(path,name);strcat(path,".cfg");
    return 1;
}

int nd_layout_apply(struct NDConfig *target,const struct NDConfig *saved)
{
    int i;
    if (!target || !saved || saved->count<1 || saved->count>ND_MAX_ITEMS)
        return 0;
    target->side=saved->side;
    target->position=saved->position;
    target->font_mode=saved->font_mode;
    target->label_mode=saved->label_mode;
    target->item_size=saved->item_size;
    /* By ID: an imported theme has no built-in index of its own. */
    if (!nd_config_set_theme(target,nd_config_theme_id(saved)))
        target->theme=saved->theme;
    target->shape=saved->shape;
    target->tile_width=saved->tile_width;
    target->tile_height=saved->tile_height;
    target->gap=saved->gap;
    target->rows=saved->rows;
    target->row_layout=saved->row_layout;
    for (i=0;i<ND_ROWS_MAX;++i) target->row_counts[i]=saved->row_counts[i];
    target->left_click=saved->left_click;
    target->tooltips=saved->tooltips;
    target->always_on_top=saved->always_on_top;
    target->running_apps=saved->running_apps;
    target->refresh_mode=saved->refresh_mode;
    target->auto_hide=saved->auto_hide;
    target->hide_delay=saved->hide_delay;
    target->removable_media=saved->removable_media;
    target->folder_view=saved->folder_view;
    target->folder_sort=saved->folder_sort;
    target->count=saved->count;
    for (i=0;i<saved->count;++i) target->items[i]=saved->items[i];
    /* The items' text offsets refer to the saved configuration's pool. */
    target->text_used=saved->text_used;
    memcpy(target->text_pool,saved->text_pool,sizeof(target->text_pool));
    return 1;
}
