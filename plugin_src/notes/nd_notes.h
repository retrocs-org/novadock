#ifndef ND_NOTES_H
#define ND_NOTES_H
#define ND_NOTE_TEXT 8192
#define ND_NOTE_RAW 12288
#define ND_NOTE_FORMAT 2048
#define ND_NOTE_MAX 32
#define ND_NOTE_BOLD 1
#define ND_NOTE_ITALIC 2
struct NDNote {char title[80],font[80],text[ND_NOTE_TEXT],original[ND_NOTE_RAW],format[ND_NOTE_FORMAT];unsigned int size,style;};
struct NDNoteText {unsigned int cursor,anchor;};
void nd_note_init(struct NDNote *note);
/* 1 read, 2 new/missing, 0 I/O failure, -1 invalid/too large. */
int nd_note_load(const char *path,struct NDNote *note);
/* Uses a caller-owned reserved drawer beside PATH. -2 means external edit. */
int nd_note_save(const char *path,const char *stage,struct NDNote *note);
int nd_note_archive(const char *path,const char *destination,const char *original);
int nd_note_dirty(const struct NDNote *note);
/* Selection-aware bounded editing; rejects controls/oversize input atomically. */
int nd_note_insert(char *text,struct NDNoteText *edit,const char *insert);
void nd_note_backspace(char *text,struct NDNoteText *edit,int forward);
#endif
