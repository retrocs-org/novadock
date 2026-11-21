#ifndef ND_TEXT_H
#define ND_TEXT_H
int nd_text_copy(const char *text);
#define ND_TEXT_EXISTS (-4)
#define ND_TEXT_PROTECTED (-5)
#define ND_TEXT_FULL (-6)
#define ND_TEXT_PATH (-7)
#define ND_TEXT_IO (-8)
const char *nd_text_export_problem(int result);
int nd_text_export(const char *path,const char *text,unsigned int length); /* 1 saved; negative ND_TEXT_* failures, no existing file is replaced. */
#endif
