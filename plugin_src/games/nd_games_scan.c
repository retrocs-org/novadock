#include "nd_games_scan.h"
#include <stdlib.h>
#include <string.h>
#if defined(ND_PLUGIN_RESOURCES)
#include "nd_plugin_resources.h"
#endif
struct NDGamesScanFrame {
    struct NDGamesScanFrame *parent;
    void *handle;
    char path[ND_GAMES_SCAN_PATH],name[ND_GAMES_SCAN_NAME],slave[ND_GAMES_SCAN_NAME];
};
static int is_slave(const char *name)
{
    static const char suffix[]=".slave";
    unsigned int n=(unsigned int)strlen(name),i;int c;
    if(n<=6)return 0;
    for(i=0;i<6;++i){c=(unsigned char)name[n-6+i];if(c>='A' && c<='Z')c+='a'-'A';if(c!=suffix[i])return 0;}
    return 1;
}
void nd_games_scan_cancel(struct NDGamesScan *scan)
{
    struct NDGamesScanFrame *frame;
    while((frame=scan->head)!=NULL){scan->head=frame->parent;scan->ops->close(scan->context,frame->handle);free(frame);}
}
static int push(struct NDGamesScan *scan,const char *path,const char *name)
{
    struct NDGamesScanFrame *frame;
    if(strlen(path)>=ND_GAMES_SCAN_PATH || strlen(name)>=ND_GAMES_SCAN_NAME){++scan->errors;return 0;}
    frame=(struct NDGamesScanFrame *)calloc(1,sizeof(*frame));
    if(!frame){++scan->errors;return 0;}
    frame->handle=scan->ops->open(scan->context,path);
    if(!frame->handle){free(frame);++scan->errors;return 0;}
    strcpy(frame->path,path);strcpy(frame->name,name);frame->parent=scan->head;scan->head=frame;
    ++scan->visited;return 1;
}
int nd_games_scan_begin(struct NDGamesScan *scan,const struct NDGamesScanOps *ops,
                         void *context,const char *root)
{
    const char *part,*p;char name[ND_GAMES_SCAN_NAME];unsigned int n;
    nd_games_scan_cancel(scan);scan->ops=ops;scan->context=context;scan->visited=0;scan->errors=0;
    if(!root || !*root)return 0;
    part=root;for(p=root;*p;++p)if((*p==':' || *p=='/') && p[1])part=p+1;
    n=(unsigned int)strlen(part);if(n && part[n-1]==':')--n;
    if(n>=sizeof(name)){++scan->errors;return 0;}
    memcpy(name,part,n);name[n]=0;
    return push(scan,root,name);
}
int nd_games_scan_step(struct NDGamesScan *scan,unsigned int budget)
{
    char name[ND_GAMES_SCAN_NAME],path[ND_GAMES_SCAN_PATH];
    struct NDGamesScanFrame *frame;int kind,keep;unsigned int n,len;
    while(scan->head && budget--){
        frame=scan->head;name[0]=0;kind=scan->ops->next(scan->context,frame->handle,name,sizeof(name));
        name[sizeof(name)-1]=0;
        if(kind<=0){
            if(kind<0)++scan->errors;
            scan->head=frame->parent;scan->ops->close(scan->context,frame->handle);
            keep=!frame->slave[0] || scan->ops->found(scan->context,frame->path,frame->name,frame->slave);
            free(frame);if(!keep){++scan->errors;nd_games_scan_cancel(scan);}
        }else if(kind==1 && !frame->slave[0] && is_slave(name))strcpy(frame->slave,name);
        else if(kind==2 && name[0] && strcmp(name,".") && strcmp(name,"..")){
            n=(unsigned int)strlen(frame->path);len=(unsigned int)strlen(name);
            if(n+len+2>sizeof(path)){++scan->errors;continue;}
            strcpy(path,frame->path);if(n && path[n-1]!=':' && path[n-1]!='/')strcat(path,"/");strcat(path,name);
            (void)push(scan,path,name);
        }
    }
    return scan->head!=NULL;
}
