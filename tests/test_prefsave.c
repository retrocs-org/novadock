#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../plugin_src/mixer/nd_prefsave.h"
struct FS {int present[3],value[3],fail,step,rollback_fail;};
static int index_of(const char *s){return strstr(s,".new") ? 1 : strstr(s,".bak") ? 2 : 0;}
static int failure(struct FS *f){return ++f->step==f->fail;}
static int exists(void *ctx,const char *p){struct FS *f=ctx;return failure(f) ? -1 : f->present[index_of(p)];}
static int write_file(void *ctx,const char *p,const unsigned char *b,unsigned long n)
{struct FS *f=ctx;int i=index_of(p);(void)n;f->present[i]=1;f->value[i]=*b;return !failure(f);}
static int move_file(void *ctx,const char *a,const char *b)
{struct FS *f=ctx;int x=index_of(a),y=index_of(b);
 if(failure(f) || (f->rollback_fail && x==2))return 0;
 if(!f->present[x] || f->present[y])return 0;
 f->present[y]=1;f->value[y]=f->value[x];f->present[x]=0;return 1;}
static int remove_file(void *ctx,const char *p)
{struct FS *f=ctx;int i=index_of(p);if(failure(f))return 0;f->present[i]=0;return 1;}
int main(void)
{
    const struct NDPrefIO io={exists,write_file,move_file,remove_file};
    struct FS f;unsigned char fresh=42;int fail,result;
    for(fail=0;fail<=6;++fail){
        memset(&f,0,sizeof(f));f.present[0]=f.present[2]=1;
        f.value[0]=7;f.value[2]=3;f.fail=fail;
        result=nd_pref_replace("prefs",&fresh,1,&io,&f);
        if(result==1){assert(f.present[0] && f.value[0]==42);assert(f.present[2] && f.value[2]==7);}
        else assert((f.present[0] && f.value[0]==7) || (f.present[2] && f.value[2]==7));
    }
    memset(&f,0,sizeof(f));f.present[0]=1;f.value[0]=7;f.fail=5;f.rollback_fail=1;
    assert(nd_pref_replace("prefs",&fresh,1,&io,&f)==-1);
    assert(f.present[2] && f.value[2]==7);
    puts("Mixer preference transactions: failures preserve original or recovery backup.");
    return 0;
}
