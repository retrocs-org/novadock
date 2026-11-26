#ifndef ND_SUPPORT_SAVE_H
#define ND_SUPPORT_SAVE_H
/* NULL closes the local progress window. No worker/dock is blocked by it. */
void nd_support_save_progress(const char *text);
int nd_support_find_lha(char *path,unsigned int size);
/* 1 selected, 0 cancelled, -1 unavailable. Does not create output files. */
int nd_support_select_save(const char *initial,const char *name,int archive,char *path,unsigned int size);
int nd_support_unique_drawer(const char *parent,const char *name,char *path,unsigned int size);
/* Publish a new archive only after successful compression; never overwrite. */
int nd_support_pack_report(const char *tool,const char *source,const char *destination);
void nd_support_discard_report(const char *path);
#endif
