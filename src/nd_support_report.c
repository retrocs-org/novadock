#include "nd_build_id.h"
/* Chosen report details and detected system data; never written to trace records. */
#include "nd_support_report.h"
#include <stdio.h>
#include <ctype.h>
int nd_support_report_has_summary(const char *summary)
{const unsigned char *p=(const unsigned char *)summary;while(*p){if(!isspace(*p))return 1;++p;}return 0;}
int nd_support_report_valid(const struct NDSupportReport *r)
{return nd_support_report_has_summary(r->summary);}
void nd_support_report_format_system(struct NDSupportReport *r)
{
    struct NDSupportMachine *m=&r->machine;
    sprintf(r->system,"CPU: %.39s\nAmigaOS / Workbench: %.63s\nKickstart: %.31s\nExec library: %.31s\nChip RAM: %.63s\nFast RAM: %.63s\nNovaDock: %.31s\nBuild ID: " ND_BUILD_ID "\nGraphics: %.79s\nDisplay mode: %.79s\n",
        m->cpu,m->os,m->kickstart,m->exec,m->chip_ram,m->fast_ram,m->app,m->graphics,m->display_mode);
}
const char *nd_support_report_frequency(unsigned int n)
{static const char *const names[]={"Not sure","Every time","Sometimes","Once"};return names[n<4?n:0];}
const char *nd_support_report_type(unsigned int n)
{static const char *const names[]={"Display / layout","Slow / unresponsive","Crash / freeze","Other / suggestion","Animation / magnify"};return names[n<5?n:3];}
static int report_field(FILE *f,const char *label,const char *value)
{
    const unsigned char *p=(const unsigned char *)value;
    if(fprintf(f,"%s\n",label)<0)return 0;
    if(!*p)return fputs("(not supplied)\n\n",f)>=0;
    while(*p){unsigned char c=*p++;if(c<32&&c!='\n'&&c!='\t')c='?';if(fputc(c,f)==EOF)return 0;}
    return fputs("\n\n",f)>=0;
}
int nd_support_report_write(const char *path,const struct NDSupportReport *r)
{
    FILE *f;int ok;
    if(!nd_support_report_valid(r))return 0;f=fopen(path,"w");if(!f)return 0;
    ok=fputs("NovaDock problem report\nDetails and detected system information; saved locally with the diagnostic bundle.\n\n",f)>=0;
    if(!report_field(f,"Problem type",nd_support_report_type(r->type)))ok=0;
    if(!report_field(f,"New recording",r->recording==2?"Completed countdown (see description for whether the issue occurred)":r->recording==1?"Partial recording / existing logs":"Unavailable; available earlier logs are included"))ok=0;
    if(!report_field(f,"Name (optional)",r->name))ok=0;
    if(!report_field(f,"Email (optional)",r->email))ok=0;
    if(!report_field(f,"Problem summary",r->summary))ok=0;
    if(!report_field(f,"Detailed description",r->description))ok=0;
    if(!report_field(f,"Detected system at report start",r->system))ok=0;
    if(!report_field(f,"How often",nd_support_report_frequency(r->frequency)))ok=0;
    if(!report_field(f,"Hardware / setup (optional)",r->hardware))ok=0;
    if(!report_field(f,"Steps to reproduce",r->steps))ok=0;
    if(!report_field(f,"Additional steps / timing / marker",r->more_steps))ok=0;
    if(!report_field(f,"Expected behaviour",r->expected))ok=0;
    if(!report_field(f,"Actual behaviour",r->actual))ok=0;
    if(!report_field(f,"Recent changes (optional)",r->changes))ok=0;
    if(fclose(f))ok=0;return ok;
}
