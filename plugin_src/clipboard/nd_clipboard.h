#ifndef ND_CLIPBOARD_H
#define ND_CLIPBOARD_H
#define ND_CLIP_MAX 32
#define ND_CLIP_TEXT_MAX 8192
#define ND_CLIP_RAW_MAX 65536UL
struct NDClipHistory {char *text[ND_CLIP_MAX];unsigned int count,limit;unsigned char pinned[ND_CLIP_MAX];};
void nd_clip_history_init(struct NDClipHistory *h,unsigned int limit);
int nd_clip_history_pin(struct NDClipHistory *h,unsigned int index);
void nd_clip_history_clear(struct NDClipHistory *h);
int nd_clip_history_add(struct NDClipHistory *h,const char *text);
int nd_clip_history_delete(struct NDClipHistory *h,unsigned int index);
void nd_clip_history_limit(struct NDClipHistory *h,unsigned int limit);
/* 1 text, 0 unsupported/empty, -1 malformed, -2 too large. */
int nd_clip_decode(const unsigned char *data,unsigned long size,char *text,unsigned int limit);
unsigned long nd_clip_encode(const char *text,unsigned char *data,unsigned long size);
void nd_clip_preview(char *out,unsigned int size,const char *text);
#endif
