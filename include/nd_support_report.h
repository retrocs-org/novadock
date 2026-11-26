#ifndef ND_SUPPORT_REPORT_H
#define ND_SUPPORT_REPORT_H
struct NDSupportMachine {
    char cpu[40],os[64],kickstart[32],exec[32],chip_ram[64],fast_ram[64];
    char app[32],graphics[80],display_mode[80];
};
struct NDSupportReport {
    char name[80],email[256],summary[256],hardware[256];
    char steps[512],more_steps[512],expected[512],actual[512],changes[512];
    char description[8192],system[1536];
    struct NDSupportMachine machine;
    unsigned int frequency,type;int recording; /* 0 unavailable, 1 partial/existing, 2 complete */
};
int nd_support_report_has_summary(const char *summary);
void nd_support_report_format_system(struct NDSupportReport *report);
const char *nd_support_report_type(unsigned int type);
int nd_support_report_valid(const struct NDSupportReport *report);
const char *nd_support_report_frequency(unsigned int frequency);
int nd_support_report_write(const char *path,const struct NDSupportReport *report);
/* 1 saved, 0 cancelled, -1 could not open. Never writes files itself. */
int nd_support_report_form(struct NDSupportReport *report);
int nd_support_report_form_at(struct NDSupportReport *report,unsigned int page);
#endif
