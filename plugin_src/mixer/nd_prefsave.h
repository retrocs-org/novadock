#ifndef ND_PREFSAVE_H
#define ND_PREFSAVE_H
/* Transaction policy, shared by native DOS I/O and failure-injection tests.
 * Return 1 on success, 0 on failure with the original restored/untouched,
 * -1 when the complete original remains in .bak after a failed rollback. */
struct NDPrefIO {
    int (*exists)(void *,const char *);
    int (*write)(void *,const char *,const unsigned char *,unsigned long);
    int (*move)(void *,const char *,const char *);
    int (*remove)(void *,const char *);
};
static int nd_pref_replace(const char *path,const unsigned char *data,
                           unsigned long length,const struct NDPrefIO *io,void *ctx)
{
    char stage[384],backup[384];
    int old,backup_exists;
    if(!path || !data || !length || strlen(path)+5>=sizeof(stage))return 0;
    strcpy(stage,path);strcat(stage,".new");
    strcpy(backup,path);strcat(backup,".bak");
    old=io->exists(ctx,path);
    if(old<0)return 0;
    if(!io->write(ctx,stage,data,length)){
        (void)io->remove(ctx,stage);return 0;
    }
    if(old){
        backup_exists=io->exists(ctx,backup);
        if(backup_exists<0 || (backup_exists && !io->remove(ctx,backup)) ||
           !io->move(ctx,path,backup)){
            (void)io->remove(ctx,stage);return 0;
        }
    }
    if(!io->move(ctx,stage,path)){
        int restored=!old || io->move(ctx,backup,path);
        (void)io->remove(ctx,stage);
        return restored ? 0 : -1;
    }
    return 1;
}
#endif
