#ifndef ND_NOTE_EDITOR_H
#define ND_NOTE_EDITOR_H
#include "nd_notes.h"
struct NDNoteEditor;
struct NDNoteEditor *nd_note_editor_open(struct NDNote *note);
/* 1 Save, 2 Save and close, 3 Discard, 4 move note to Deleted. */
int nd_note_editor_events(struct NDNoteEditor *editor);
int nd_note_editor_prepare_save(struct NDNoteEditor *editor);
unsigned long nd_note_editor_signals(const struct NDNoteEditor *editor);
void nd_note_editor_notice(struct NDNoteEditor *editor,const char *message);
void nd_note_editor_raise(struct NDNoteEditor *editor);
void nd_note_editor_close(struct NDNoteEditor *editor);
#endif
