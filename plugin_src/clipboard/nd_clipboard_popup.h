#ifndef ND_CLIPBOARD_POPUP_H
#define ND_CLIPBOARD_POPUP_H
#include "nd_clipboard.h"
struct NDClipQuick;
struct NDClipQuick *nd_clip_quick_create(void);
int nd_clip_quick_open(struct NDClipQuick *q,const struct NDClipHistory *history);
unsigned long nd_clip_quick_signals(const struct NDClipQuick *q);
int nd_clip_quick_active(const struct NDClipQuick *q);
const char *nd_clip_quick_events(struct NDClipQuick *q);
void nd_clip_quick_labels(struct NDClipQuick *q,const char *heading,const char *empty,
                           const char *title,const char *footer,const char *item);
int nd_clip_quick_selected(const struct NDClipQuick *q);
void nd_clip_quick_close(struct NDClipQuick *q);
void nd_clip_quick_free(struct NDClipQuick *q);
#endif
