#ifndef ND_CLIPBOARD_POPUP_H
#define ND_CLIPBOARD_POPUP_H
#include "nd_clipboard.h"
struct NDClipQuick;
struct NDClipQuick *nd_clip_quick_create(void);
int nd_clip_quick_open(struct NDClipQuick *q,const struct NDClipHistory *history);
int nd_clip_quick_active(const struct NDClipQuick *q);
const char *nd_clip_quick_events(struct NDClipQuick *q);
void nd_clip_quick_close(struct NDClipQuick *q);
void nd_clip_quick_free(struct NDClipQuick *q);
#endif
