#ifndef ND_TEXT_H
#define ND_TEXT_H
int nd_text_copy(const char *text);
int nd_text_export(const char *path,const char *text,unsigned int length); /* 1 saved, -4 already exists */
#endif
