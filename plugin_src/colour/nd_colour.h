#ifndef ND_COLOUR_H
#define ND_COLOUR_H
#define ND_COLOUR_MAX 32
struct NDColour {unsigned long rgb;char name[32];};
struct NDColourPalette {unsigned int count;struct NDColour colours[ND_COLOUR_MAX];};
int nd_colour_hex(const char *text,unsigned long *rgb);
void nd_colour_text(unsigned long rgb,char hex[8],char decimal[32]);
int nd_colour_add(struct NDColourPalette *p,const char *name,unsigned long rgb);
int nd_colour_remove(struct NDColourPalette *p,unsigned int index);
int nd_colour_parse(const char *text,struct NDColourPalette *p);
unsigned int nd_colour_format(const struct NDColourPalette *p,char out[2048]);
int nd_colour_read(const char *path,struct NDColourPalette *p); /* -1 absent, 0 invalid, 1 valid */
const char *nd_colour_save_problem(void);
int nd_colour_save(const char *path,const struct NDColourPalette *p);
#endif
