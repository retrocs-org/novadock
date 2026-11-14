#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_markdown.h"
static struct NDMarkdown v,again;
static char raw[ND_NOTE_TEXT],metadata[ND_NOTE_FORMAT];
static struct NDMDStyle style(unsigned int n)
{struct NDMDStyle f;memset(&f,0,sizeof(f));f.style=n%4;f.face=(n/4)%3;f.color=n/12;return f;}
int main(void)
{
    struct NDNoteText edit;unsigned int a,b,i;struct NDMDStyle f;
    nd_md_parse("Alpha beta\ngamma",&v,0,0);edit.anchor=0;edit.cursor=5;nd_md_format(&v,&edit,1,0);
    edit.anchor=6;edit.cursor=10;nd_md_format(&v,&edit,3,2);nd_md_format(&v,&edit,4,1);nd_md_format(&v,&edit,2,0);
    edit.anchor=edit.cursor=12;nd_md_format(&v,&edit,5,2);
    assert(nd_md_write(&v,raw));assert(strstr(raw,"**Alpha**"));assert(strstr(raw,"## gamma"));assert(!strstr(raw,"<span"));assert(nd_md_formats_write(&v,metadata,sizeof(metadata)));
    nd_md_parse(raw,&again,0,0);assert(nd_md_formats_read(&again,metadata));assert(!strcmp(v.text,again.text));
    for(i=0;v.text[i];++i)assert(!memcmp(&v.format[i],&again.format[i],sizeof(f)));
    edit.anchor=6;edit.cursor=10;nd_md_format(&v,&edit,6,0);assert(v.format[0].style==1&&v.format[6].face==0&&v.format[6].color==0);
    /* Adjacent spans and every supported style/font/colour combination must
     * survive Markdown writing and parsing, including stacked marker runs. */
    for(a=0;a<72;++a)for(b=0;b<72;++b){
        memset(&v,0,sizeof(v));strcpy(v.text,"ab");v.format[0]=style(a);v.format[1]=style(b);
        assert(nd_md_write(&v,raw));assert(nd_md_formats_write(&v,metadata,sizeof(metadata)));nd_md_parse(raw,&again,0,0);assert(nd_md_formats_read(&again,metadata));
        if(strcmp(again.text,"ab") || memcmp(&v.format[0],&again.format[0],sizeof(f)) || memcmp(&v.format[1],&again.format[1],sizeof(f))){
            printf("Roundtrip failed %u %u: %s -> %s\n",a,b,raw,again.text);return 1;}
    }
    nd_md_parse("a_b unmatched ** and literal <html> & text",&v,0,0);assert(!strcmp(v.text,"a_b unmatched ** and literal <html> & text"));
    assert(nd_md_write(&v,raw));nd_md_parse(raw,&again,0,0);assert(!strcmp(v.text,again.text));
    nd_md_parse("# One\n## Two\n### Three\n#### Four\nNormal",&v,0,0);
    assert(v.format[0].heading==1&&v.format[4].heading==2&&v.format[8].heading==3&&v.format[14].heading==4&&v.format[19].heading==0);
    assert(nd_md_write(&v,raw));nd_md_parse(raw,&again,0,0);assert(!strcmp(v.text,again.text));
    nd_md_parse("Text",&v,0,0);nd_md_parse("# Text",&again,0,0);nd_md_reconcile(&v,&again);assert(again.format[0].face==2);
    edit.anchor=0;edit.cursor=4;nd_md_format(&again,&edit,3,0);assert(nd_md_formats_write(&again,metadata,sizeof(metadata)));
    assert(nd_md_write(&again,raw));nd_md_parse(raw,&v,0,0);assert(v.format[0].face==2);assert(nd_md_formats_read(&v,metadata)&&v.format[0].face==0);
    nd_md_parse("Hello",&v,0,0);edit.cursor=5;edit.anchor=0;f=style(71);
    assert(nd_md_insert(&v,&edit,"Replacement",f));assert(!strcmp(v.text,"Replacement"));assert(v.format[0].color==5&&v.format[0].face==2&&v.format[0].style==3);
    assert(nd_md_write(&v,raw));assert(nd_md_position(&v,v.source[5])==5);
    nd_md_backspace(&v,&edit,0);assert(!strcmp(v.text,"Replacemen"));
    memset(&v,0,sizeof(v));memset(v.text,'x',ND_NOTE_TEXT-1);v.text[ND_NOTE_TEXT-1]=0;v.format[0].style=1;
    strcpy(raw,"original");assert(!nd_md_write(&v,raw));assert(!strcmp(raw,"original"));
    puts("Markdown: 5184 inline format combinations, H1-H4, separate colour/font metadata, escaping, editing and atomic bounds pass.");return 0;
}
