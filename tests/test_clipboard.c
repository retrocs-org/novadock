#include "../plugin_src/clipboard/nd_clipboard.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
int main(void)
{
    struct NDClipHistory h;unsigned char raw[128];char text[64],preview[8];unsigned long n;
    unsigned char multi[]={ 'F','O','R','M',0,0,0,38,'F','T','X','T',
        'C','S','E','T',0,0,0,2,0,1,
        'C','H','R','S',0,0,0,3,'o','n','e',0,
        'C','H','R','S',0,0,0,3,'t','w','o',0};
    nd_clip_history_init(&h,3);
    assert(nd_clip_history_add(&h,"first"));assert(nd_clip_history_add(&h,"second"));
    assert(!nd_clip_history_add(&h,"second") && h.count==2);
    assert(nd_clip_history_add(&h,"first") && h.count==2 && !strcmp(h.text[0],"first"));
    assert(nd_clip_history_add(&h,"third"));assert(nd_clip_history_add(&h,"fourth"));
    assert(h.count==3 && !strcmp(h.text[2],"first"));
    nd_clip_history_limit(&h,1);assert(h.count==1 && !strcmp(h.text[0],"fourth"));
    assert(!nd_clip_history_delete(&h,2));assert(nd_clip_history_delete(&h,0));
    assert(!nd_clip_history_add(&h,""));
    n=nd_clip_encode("one\ntwo",raw,sizeof(raw));assert(n==28);
    assert(nd_clip_decode(raw,n,text,sizeof(text))==1 && !strcmp(text,"one\ntwo"));
    assert(nd_clip_decode(multi,sizeof(multi),text,sizeof(text))==1 && !strcmp(text,"onetwo"));
    assert(nd_clip_decode(multi,sizeof(multi),text,6)==-2);
    assert(nd_clip_decode(raw,n-1,text,sizeof(text))==-1);
    raw[16]=0xff;assert(nd_clip_decode(raw,n,text,sizeof(text))==-1);
    memcpy(raw,"FORM\0\0\0\4ILBM",12);assert(nd_clip_decode(raw,12,text,sizeof(text))==0);
    n=nd_clip_encode("abc",raw,sizeof(raw));raw[21]=0;assert(nd_clip_decode(raw,n,text,sizeof(text))==-1);
    nd_clip_preview(preview,sizeof(preview),"abc\ndef ghi");assert(!strcmp(preview,"abc ..."));
    nd_clip_preview(preview,sizeof(preview),"a");assert(!strcmp(preview,"a"));
    nd_clip_preview(preview,1,"test");assert(!preview[0]);
    {
        static char big[ND_CLIP_TEXT_MAX+2];
        memset(big,'x',sizeof(big)-1);big[sizeof(big)-1]=0;
        assert(!nd_clip_history_add(&h,big));
        big[ND_CLIP_TEXT_MAX]=0;assert(nd_clip_history_add(&h,big));
        assert(!nd_clip_encode(big,raw,sizeof(raw)));
    }
    nd_clip_history_clear(&h);assert(h.count==0);
    nd_clip_history_init(&h,2);
    assert(nd_clip_history_add(&h,"keep"));assert(nd_clip_history_pin(&h,0));
    assert(nd_clip_history_add(&h,"replace"));assert(nd_clip_history_add(&h,"new"));
    assert(!strcmp(h.text[1],"keep") && h.pinned[1]);
    assert(nd_clip_history_pin(&h,0));assert(!nd_clip_history_add(&h,"all pinned"));
    assert(nd_clip_history_add(&h,"keep") && h.pinned[0]);
    assert(nd_clip_history_delete(&h,0) && h.pinned[0]);
    nd_clip_history_clear(&h);assert(!h.count && !h.pinned[0]);
    puts("Bounded clipboard history, deduplication, IFF chunks, malformed/oversized clips passed.");return 0;
}
