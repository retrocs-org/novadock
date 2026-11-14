#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_notes.h"
#include "nd_markdown.h"
static struct NDNote note,loaded;
static void write_file(const char *path,const char *text)
{FILE *f=fopen(path,"wb");assert(f);assert(fwrite(text,1,strlen(text),f)==strlen(text));assert(!fclose(f));}
int main(void)
{
    struct NDNoteText edit;char text[ND_NOTE_TEXT];FILE *f;unsigned int i;
    assert(nd_note_load("new.ndnote",&note)==2);
    strcpy(note.title,"Shopping");strcpy(note.text,"Bread\nMilk\nCoffee");note.style=ND_NOTE_BOLD|ND_NOTE_ITALIC;
    assert(nd_note_dirty(&note));assert(nd_note_save("new.ndnote","stage",&note)==1);
    assert(!nd_note_dirty(&note));assert(nd_note_load("new.ndnote",&loaded)==1);
    assert(!strcmp(loaded.title,"Shopping")&&!strcmp(loaded.text,note.text)&&loaded.style==3);
    strcpy(loaded.font,"courier.font");loaded.size=13;assert(nd_note_save("new.ndnote","stage",&loaded)==1);
    assert(nd_note_load("new.ndnote",&note)==1&&!strcmp(note.font,"courier.font")&&note.size==13);
    strcpy(note.text,"Unsaved local edit");write_file("new.ndnote","External file changed\n");
    assert(nd_note_save("new.ndnote","stage",&note)==-2);assert(nd_note_load("new.ndnote",&loaded)==-1);
    f=fopen("new.ndnote","rb");assert(f);fgets(text,sizeof(text),f);fclose(f);assert(!strcmp(text,"External file changed\n"));
    nd_note_init(&note);strcpy(note.text,"New note after restart");assert(nd_note_save("another.ndnote","nonexistent/stage",&note)==0);
    assert(nd_note_save("another.ndnote","stage",&note)==1);assert(nd_note_load("another.ndnote",&loaded)==1);
    strcpy(loaded.font,"SYS:Something");assert(nd_note_save("another.ndnote","stage",&loaded)==-1);
    memset(text,'x',sizeof(text));f=fopen("huge.ndnote","wb");assert(f);for(i=0;i<3;++i)fwrite(text,1,sizeof(text),f);fclose(f);
    assert(nd_note_load("huge.ndnote",&loaded)==-1);
    strcpy(text,"Hello world");edit.cursor=edit.anchor=5;assert(nd_note_insert(text,&edit,"\nnew"));assert(!strcmp(text,"Hello\nnew world"));
    edit.anchor=0;edit.cursor=5;assert(nd_note_insert(text,&edit,"Hi"));assert(!strcmp(text,"Hi\nnew world"));
    nd_note_backspace(text,&edit,0);assert(!strcmp(text,"H\nnew world"));
    nd_note_backspace(text,&edit,1);assert(!strcmp(text,"Hnew world"));
    edit.cursor=edit.anchor=0;assert(!nd_note_insert(text,&edit,"\001"));assert(!strcmp(text,"Hnew world"));
    memset(text,'x',ND_NOTE_TEXT-1);text[ND_NOTE_TEXT-1]=0;edit.cursor=edit.anchor=0;assert(!nd_note_insert(text,&edit,"x"));
    assert(strlen(text)==ND_NOTE_TEXT-1);
    nd_note_init(&note);strcpy(note.title,"Title --> 100% <done>");strcpy(note.text,"# Markdown\n**Bold** text");
    assert(nd_note_save("markdown.md","stage",&note)==1);assert(nd_note_load("markdown.md",&loaded)==1);
    assert(!strcmp(note.title,loaded.title)&&!strcmp(note.text,loaded.text));
    assert(nd_note_archive("markdown.md","stage/deleted.md",loaded.original)==1);
    assert(nd_note_load("markdown.md",&note)==2);assert(nd_note_load("stage/deleted.md",&loaded)==1);
    nd_note_init(&note);strcpy(note.title,"Formatting metadata");strcpy(note.text,"**Bold** rest");strcpy(note.format,"0,4,2,1;");
    assert(nd_note_save("metadata.md","stage",&note)==1);assert(nd_note_load("metadata.md",&loaded)==1);
    assert(!strcmp(loaded.format,note.format)&&!strstr(loaded.original,"<span"));
    {static struct NDMarkdown view;nd_md_parse(loaded.text,&view,0,0);assert(nd_md_formats_read(&view,loaded.format));assert(view.format[0].face==2&&view.format[0].color==1&&view.format[0].style==1);}
    strcpy(text,loaded.original);strcat(text," changed");write_file("metadata.md",text);
    assert(nd_note_load("metadata.md",&note)==1&&!note.format[0]&&strstr(note.text,"changed"));
    puts("Notes: permanent title/text/font/style, restart reads, conflicts, I/O errors and bounded multiline edits pass.");return 0;
}
