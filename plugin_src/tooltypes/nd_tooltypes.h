#ifndef ND_TOOLTYPES_H
#define ND_TOOLTYPES_H
#define ND_TT_MAX 64
#define ND_TT_LINE 160
struct NDToolTypes {char path[240],tool[160];unsigned long stack,hash,size;unsigned int count,type;char lines[ND_TT_MAX][ND_TT_LINE];};
int nd_tt_path(const char *input,char out[240]);
int nd_tt_line(const char *line);
int nd_tt_protected(const char *line);
int nd_tt_preserves_art(const struct NDToolTypes *original,const struct NDToolTypes *edited);
int nd_tt_stack(const char *text,unsigned long *stack);
unsigned long nd_tt_hash(unsigned long hash,const unsigned char *data,unsigned int length);
#endif
