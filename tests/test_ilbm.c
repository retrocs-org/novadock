#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_ilbm.h"
#include "nd_collection_icons.h"
int main(void)
{
    struct NDILBM bm;
    unsigned int i,j,painted;
    for (i=0;i<ND_COLLECTION_ICON_COUNT;++i) {
        assert(nd_ilbm_load(nd_collection_icons[i].path,&bm)==1);
        assert(bm.width==32 && bm.height==32);
        assert(bm.colors==16);
        assert(bm.transparent==0);
        painted=0;
        for (j=0;j<32*32;++j) {
            if (bm.pixels[(j/32)*ND_ICON_MAX_DIM+j%32]!=0) ++painted;
        }
        assert(painted>75);
    }
    assert(nd_collection_icon_index("Demos")>=0);
    assert(nd_collection_icon_index("invalid")<0);
    assert(nd_ilbm_load("Icons/Workbench.iff",&bm));
    {
        struct NDILBM packed;
        assert(nd_ilbm_load("tests/Workbench-ByteRun1.iff",&packed));
        assert(packed.width==bm.width && packed.height==bm.height);
        assert(memcmp(packed.pixels,bm.pixels,sizeof(bm.pixels))==0);
    }
    assert(nd_ilbm_load("Icons/missing.iff",&bm)==0);
    assert(nd_ilbm_load("novadock.cfg",&bm)==0);
    puts("Classic IFF/ILBM launcher artwork passed.");
    return 0;
}
