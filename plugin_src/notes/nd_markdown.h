#ifndef ND_MARKDOWN_H
#define ND_MARKDOWN_H
#include "nd_notes.h"
struct NDMDStyle {unsigned char style,face,color,heading;};
struct NDMarkdown {char text[ND_NOTE_TEXT];unsigned short source[ND_NOTE_TEXT];struct NDMDStyle format[ND_NOTE_TEXT];};
/* Supported Markdown: H1-H4, bold, italic, code, escapes and safe font/colour spans. */
void nd_md_parse(const char *source,struct NDMarkdown *view,unsigned int face,unsigned int style);
int nd_md_write(struct NDMarkdown *view,char *source);
int nd_md_insert(struct NDMarkdown *view,struct NDNoteText *edit,const char *text,struct NDMDStyle format);
void nd_md_backspace(struct NDMarkdown *view,struct NDNoteText *edit,int forward);
/* 1 bold, 2 italic, 3 font, 4 colour, 5 paragraph H0-H4, 6 clear formatting. */
void nd_md_format(struct NDMarkdown *view,const struct NDNoteText *edit,int action,unsigned int value);
int nd_md_formats_write(const struct NDMarkdown *view,char *out,unsigned int size);
int nd_md_formats_read(struct NDMarkdown *view,const char *metadata);
void nd_md_reconcile(const struct NDMarkdown *before,struct NDMarkdown *after);
unsigned int nd_md_position(const struct NDMarkdown *view,unsigned int source_offset);
#endif
