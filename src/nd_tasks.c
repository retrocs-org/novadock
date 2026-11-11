/* Portable taskbar policy; see nd_tasks.h. */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_tasks.h"

static int same_nocase(const char *a,const char *b)
{
    while (*a && *b) {
        if (tolower((unsigned char)*a)!=tolower((unsigned char)*b)) return 0;
        ++a;++b;
    }
    return !*a && !*b;
}
static int printable(const char *s)
{
    for (;*s;++s) if ((unsigned char)*s<32 || *s=='|') return 0;
    return 1;
}
static int generic_name(const char *name)
{
    static const char *generic[]={"Background CLI","Shell Process",
        "Initial CLI","New CLI","CON","RAW","ConHandler","AmigaShell",
        "Shell","CLI",NULL};
    int i;
    for (i=0;generic[i];++i) if (same_nocase(generic[i],name)) return 1;
    return 0;
}
static void copy_label(char *out,unsigned int size,const char *text)
{
    unsigned int n=0;
    while (*text==' ') ++text;
    while (text[n] && n+1<size) { out[n]=text[n]; ++n; }
    out[n]='\0';
    while (n && out[n-1]==' ') out[--n]='\0';
}
int nd_task_label(const char *task_name,const char *command_name,
                  const char *window_title,char *out,unsigned int size)
{
    const char *base,*p;
    if (!out || size<2) return 0;
    out[0]='\0';
    if (command_name && *command_name && printable(command_name)) {
        base=command_name;
        for (p=command_name;*p;++p) if (*p==':' || *p=='/') base=p+1;
        if (*base && !generic_name(base)) { copy_label(out,size,base); return 1; }
    }
    if (task_name && *task_name && printable(task_name) &&
        !generic_name(task_name)) {
        copy_label(out,size,task_name);
        return out[0]!='\0';
    }
    if (window_title && *window_title && printable(window_title)) {
        copy_label(out,size,window_title);
        return out[0]!='\0';
    }
    return 0;
}
int nd_task_ignored(const char *label,const char *list)
{
    const char *p,*end;
    char entry[ND_TASK_NAME_LEN];
    size_t length;
    if (!label || !list) return 0;
    for (p=list;*p;p=end) {
        while (*p==' ' || *p==',') ++p;
        end=p;
        while (*end && *end!=',') ++end;
        length=(size_t)(end-p);
        while (length && p[length-1]==' ') --length;
        if (length && length<sizeof(entry)) {
            memcpy(entry,p,length);entry[length]='\0';
            if (same_nocase(entry,label)) return 1;
        }
    }
    return 0;
}
static int compare(const struct NDTaskEntry *a,const struct NDTaskEntry *b)
{
    int c=strcmp(a->label,b->label);
    if (c) return c;
    return a->key<b->key ? -1 : a->key>b->key;
}
int nd_tasks_normalize(struct NDTaskEntry *tasks,int count)
{
    int i,j,w;
    struct NDTaskEntry t;
    if (!tasks || count<=0) return 0;
    for (i=1;i<count;++i) {
        t=tasks[i];
        for (j=i;j>0 && compare(&tasks[j-1],&t)>0;--j) tasks[j]=tasks[j-1];
        tasks[j]=t;
    }
    for (i=0,w=0;i<count;++i) {
        for (j=0;j<w;++j) if (tasks[j].key==tasks[i].key) break;
        if (j==w && tasks[i].key && tasks[i].label[0]) tasks[w++]=tasks[i];
    }
    return w;
}
int nd_tasks_same(const struct NDTaskEntry *a,int a_count,
                  const struct NDTaskEntry *b,int b_count)
{
    int i;
    if (a_count!=b_count) return 0;
    for (i=0;i<a_count;++i)
        if (a[i].key!=b[i].key || strcmp(a[i].label,b[i].label) ||
            strcmp(a[i].path,b[i].path)) return 0;
    return 1;
}
int nd_tasks_append(struct NDConfig *cfg,const struct NDTaskEntry *tasks,
                    int count)
{
    int i,added=0;
    struct NDItem *item;
    if (!cfg || !tasks || !cfg->running_section) return 0;
    for (i=0;i<count && i<ND_TASK_MAX && cfg->count<ND_MAX_ITEMS;++i) {
        const struct NDTaskEntry *t=&tasks[i];
        if (!t->key || !t->label[0] || strlen(t->label)>=ND_LABEL_LEN ||
            !printable(t->label) || strlen(t->path)>=ND_COMMAND_LEN) continue;
        item=&cfg->items[cfg->count];
        memset(item,0,sizeof(*item));
        item->kind=ND_ITEM_LAUNCH;
        strcpy(item->label,t->label);
        strcpy(item->caption,t->label);
        strcpy(item->command,"@task");
        if (!nd_item_set_text(cfg,item,ND_TEXT_ARGS,t->path)) break;
        sprintf(item->id,"T%08lx",t->key);
        strcpy(item->value,"RUNNING");
        item->meter=65535U;
        item->enabled=1;
        item->launch_mode=ND_LAUNCH_AUTO;
        item->show_volume_meter=0;
        item->transient=ND_TRANSIENT_TASK;
        ++cfg->count;
        ++added;
    }
    return added;
}
unsigned long nd_task_tile_key(const struct NDItem *item)
{
    char *end;
    unsigned long key;
    if (!item || item->transient!=ND_TRANSIENT_TASK || item->id[0]!='T') return 0;
    key=strtoul(item->id+1,&end,16);
    return *end ? 0 : key;
}
