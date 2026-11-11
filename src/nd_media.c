/* Portable removable-media policy; see nd_media.h. */
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "nd_media.h"

static int same_nocase(const char *a,const char *b)
{
    while (*a && *b) {
        if (tolower((unsigned char)*a)!=tolower((unsigned char)*b)) return 0;
        ++a;++b;
    }
    return !*a && !*b;
}
int nd_media_device_removable(const char *device,const char *list)
{
    const char *p,*end;
    char entry[ND_MEDIA_DEVICE_LEN];
    size_t length;
    if (!device || !*device) return 0;
    p=(list && *list) ? list : ND_MEDIA_DEFAULT_DEVICES;
    while (*p) {
        while (*p==' ' || *p==',') ++p;
        end=p;
        while (*end && *end!=',') ++end;
        length=(size_t)(end-p);
        while (length && p[length-1]==' ') --length;
        if (length && length<sizeof(entry)) {
            memcpy(entry,p,length);entry[length]='\0';
            if (same_nocase(entry,device)) return 1;
        }
        p=end;
    }
    return 0;
}
static int compare(const struct NDMediaVolume *a,const struct NDMediaVolume *b)
{
    int c=strcmp(a->drive,b->drive);
    return c ? c : strcmp(a->volume,b->volume);
}
int nd_media_normalize(struct NDMediaVolume *media,int count)
{
    int i,j,w;
    struct NDMediaVolume t;
    if (!media || count<=0) return 0;
    for (i=1;i<count;++i) {
        t=media[i];
        for (j=i;j>0 && compare(&media[j-1],&t)>0;--j) media[j]=media[j-1];
        media[j]=t;
    }
    for (i=1,w=1;i<count;++i)
        if (compare(&media[w-1],&media[i])) media[w++]=media[i];
    return w;
}
int nd_media_same(const struct NDMediaVolume *a,int a_count,
                  const struct NDMediaVolume *b,int b_count)
{
    int i;
    if (a_count!=b_count) return 0;
    for (i=0;i<a_count;++i)
        if (compare(&a[i],&b[i])) return 0;
    return 1;
}
int nd_media_persistent_count(const struct NDConfig *cfg)
{
    int i;
    if (!cfg) return 0;
    for (i=0;i<cfg->count;++i) if (cfg->items[i].transient) break;
    return i;
}
static int pinned(const struct NDConfig *cfg,const char *target)
{
    int i;
    for (i=0;i<cfg->count;++i)
        if (cfg->items[i].kind==ND_ITEM_DRAWER && !cfg->items[i].transient &&
            same_nocase(cfg->items[i].command,target)) return 1;
    return 0;
}
int nd_media_append(struct NDConfig *cfg,const struct NDMediaVolume *media,
                    int count,const char *system_volume)
{
    int i,added=0;
    char target[ND_MEDIA_NAME_LEN+2],icon[ND_MEDIA_NAME_LEN+8];
    struct NDItem *item;
    if (!cfg || !media || !cfg->removable_media) return 0;
    for (i=0;i<count && i<ND_MEDIA_MAX && cfg->count<ND_MAX_ITEMS;++i) {
        const struct NDMediaVolume *m=&media[i];
        if (!m->volume[0] || strlen(m->volume)>=ND_LABEL_LEN ||
            strchr(m->volume,':') || strchr(m->volume,'/')) continue;
        if (system_volume && same_nocase(system_volume,m->volume)) continue;
        sprintf(target,"%s:",m->volume);
        if (pinned(cfg,target)) continue;
        sprintf(icon,"%s:Disk",m->volume);
        item=&cfg->items[cfg->count];
        memset(item,0,sizeof(*item));
        item->kind=ND_ITEM_DRAWER;
        strcpy(item->label,m->volume);
        strcpy(item->caption,m->volume);
        strcpy(item->command,target);
        if (!nd_item_set_text(cfg,item,ND_TEXT_ICON,icon)) break;
        strncpy(item->id,m->drive,sizeof(item->id)-1);
        strcpy(item->value,"OPEN");
        item->meter=65535U;
        item->enabled=1;
        item->launch_mode=ND_LAUNCH_AUTO;
        item->show_volume_meter=1;
        item->transient=ND_TRANSIENT_MEDIA;
        ++cfg->count;
        ++added;
    }
    return added;
}
