#ifndef ND_LIST_H
#define ND_LIST_H
#define ND_LIST_MAX 64
struct NDList;
struct NDList *nd_list_open(const char *title,const char **buttons,unsigned int count);
void nd_list_button(struct NDList *p,unsigned int index,const char *label);
void nd_list_follow(struct NDList *p,int following);
int nd_list_following(const struct NDList *p);
void nd_list_close(struct NDList *p);
void nd_list_title(struct NDList *p,const char *status);
void nd_list_select(struct NDList *p,int index);
void nd_list_raise(struct NDList *p);
void nd_list_rows(struct NDList *p,const char *const *rows,unsigned int count);
int nd_list_event(struct NDList *p); /* -1 close, 100+button, 0 idle */
int nd_list_selected(const struct NDList *p); /* underlying row, or -1 */
unsigned long nd_list_signals(const struct NDList *p);
#endif
