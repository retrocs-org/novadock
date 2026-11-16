#ifndef ND_GAMES_SCAN_H
#define ND_GAMES_SCAN_H
#define ND_GAMES_SCAN_PATH 300
#define ND_GAMES_SCAN_NAME 108
/* next: 0=end, 1=file, 2=real drawer, -1=error; links are skipped by adapter. */
struct NDGamesScanOps {
    void *(*open)(void *context,const char *path);
    int (*next)(void *context,void *handle,char *name,unsigned int size);
    void (*close)(void *context,void *handle);
    int (*found)(void *context,const char *path,const char *name,const char *slave);
};
struct NDGamesScanFrame;
struct NDGamesScan {
    struct NDGamesScanFrame *head;
    const struct NDGamesScanOps *ops;
    void *context;
    unsigned long visited;
    unsigned int errors;
};
void nd_games_scan_cancel(struct NDGamesScan *scan);
int nd_games_scan_begin(struct NDGamesScan *scan,const struct NDGamesScanOps *ops,
                         void *context,const char *root);
/* Performs a bounded batch; callers remain free to service windows and IPC. */
int nd_games_scan_step(struct NDGamesScan *scan,unsigned int budget);
#endif
