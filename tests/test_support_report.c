#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_support_report.h"
#include "nd_build_id.h"
static struct NDSupportReport report;
int main(void)
{
    FILE *f;char text[18000];size_t n;int i;
    memset(&report,0,sizeof(report));assert(!nd_support_report_valid(&report));
    strcpy(report.summary," \t ");assert(!nd_support_report_valid(&report));
    assert(!nd_support_report_write("problem.txt",&report));
    strcpy(report.summary,"Network preferences become slow");
    strcpy(report.steps,"1. Start DOpus fullscreen\n2. Open plugin settings");
    strcpy(report.more_steps,"Marked the event before clicking Edit");
    strcpy(report.expected,"Settings should respond immediately");
    strcpy(report.actual,"The field waits several seconds");
    strcpy(report.changes,"Updated NovaDock");
    strcpy(report.description,"A detailed description\nwith a second line\nand a third.");
    strcpy(report.machine.cpu,"Motorola 68060");strcpy(report.machine.kickstart,"47.115");
    strcpy(report.machine.os,"3.2.3 (Workbench 47.42)");strcpy(report.machine.app,"0.6");
    strcpy(report.machine.graphics,"1024 x 600 / 32-bit / ID 50011000");
    strcpy(report.machine.display_mode,"UAEGFX: 1024x600");nd_support_report_format_system(&report);
    assert(strstr(report.system,"Build ID: " ND_BUILD_ID));
    report.type=1;report.recording=2;
    assert(nd_support_report_valid(&report)&&nd_support_report_write("problem.txt",&report));
    f=fopen("problem.txt","r");assert(f);n=fread(text,1,sizeof(text)-1,f);text[n]=0;fclose(f);
    assert(strstr(text,"(not supplied)")&&strstr(text,report.summary)&&strstr(text,report.steps)&&strstr(text,"Not sure"));
    assert(strstr(text,report.description)&&strstr(text,"Motorola 68060")&&strstr(text,"Slow / unresponsive"));
    assert(strstr(text,"NovaDock: 0.6")&&strstr(text,"32-bit")&&strstr(text,"UAEGFX")&&strstr(text,"Completed countdown"));
    strcpy(report.name,"QA Test");strcpy(report.email,"qa@example.invalid");report.frequency=2;
    strcpy(report.hardware,"Disposable AmigaOS 3.2.3 guest");report.actual[0]=27;report.actual[1]='X';report.actual[2]=0;
    memset(report.steps,'S',sizeof(report.steps)-1);report.steps[sizeof(report.steps)-1]=0;
    memset(report.description,'D',sizeof(report.description)-1);report.description[sizeof(report.description)-1]=0;
    assert(nd_support_report_write("problem.txt",&report));
    f=fopen("problem.txt","r");assert(f);n=fread(text,1,sizeof(text)-1,f);text[n]=0;fclose(f);
    assert(strstr(text,"qa@example.invalid")&&strstr(text,"Sometimes")&&strstr(text,"?X"));
    assert(strstr(text,report.description));
    assert(!strcmp(nd_support_report_type(999),"Other / suggestion"));
    for(i=0;text[i];++i)assert((unsigned char)text[i]!=27);
    assert(!strcmp(nd_support_report_frequency(999),"Not sure"));
    assert(!nd_support_report_write("missing/problem.txt",&report));
    remove("problem.txt");puts("Support report: optional contact, required summary, reproduction context, long fields, control-byte escaping and write failures passed.");return 0;
}
