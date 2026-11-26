/* Offline regression for portable and split Install component lookup. */
#include "native_qa_ui.h"
#include <proto/wb.h>
struct Library *WorkbenchBase;
long __stack=65536;
int main(void)
{
    static const char *programs[]={"QATests:Dock/NovaDock",
        "QATests:Bin/NovaDock","QATests:Metadata/NovaDock"};
    static const char *layouts[]={"portable","installed empty Plugins",
        "installed metadata-only Plugins"};
    FILE *f;
    int phase,i,j,ok=1;
    struct NDDiagnosticsSnapshot d;
    if (!qa_init()) return 20;
    WorkbenchBase=OpenLibrary("workbench.library",44);
    if (!WorkbenchBase || qa_port(ND_CONTROL_PORT)) return 20;
    SystemTags("MakeDir RAM:LookupEnv RAM:LookupEnv/NovaDock RAM:LookupArc",TAG_DONE);
    SystemTags("Assign ENV: RAM:LookupEnv",TAG_DONE);
    SystemTags("Assign ENVARC: RAM:LookupArc",TAG_DONE);
    SystemTags("Assign NovaDock: REMOVE",TAG_DONE);
    f=fopen("ENV:NovaDock/novadock.cfg","w");
    if (!f) return 20;
    fputs("SIDE=BOTTOM\nHOVER_MAGNIFICATION=0\nDRAG_BAR=NO\n"
        "REMOVABLE_MEDIA=NO\nTOOLTIPS=NO\n"
        "PLUGIN=CPU|Plugins/Activity|CPU|METER|ON\n"
        "PLUGIN=RAM|Plugins/Memory|RAM|METER|ON\n"
        "PLUGIN=Clock|Plugins/Clock|CLOCK|METER|ON\n"
        "PLUGIN=Archive|Plugins/ArchiveExtractor|ARCHIVE|METER|ON\n"
        "PLUGIN=Pet|Plugins/Pet|PET|METER|ON\n",f);
    fclose(f);
    for (phase=0;phase<6;++phase) {
        printf("MODE %s %s\n",layouts[phase/2],phase&1 ? "Workbench":"Shell");
        if (phase==2 && SystemTags("Assign NovaDock: QATests:Dock",TAG_DONE)!=0) {
            ok=0;break;
        }
        if (phase&1 ? !OpenWorkbenchObjectA((STRPTR)programs[phase/2],NULL):
                       !qa_start(programs[phase/2])) {
            puts("Launch failed");ok=0;break;
        }
        for (i=0;i<500&&!qa_port(ND_CONTROL_PORT);++i) Delay(1);
        /* Allow slow physical-style guests to finish starting all five clients. */
        for (i=0;i<100;++i) {
            memset(&d,0,sizeof(d));
            if (nd_control_diagnostics(&d)==ND_CONTROL_OK && d.online_count==5) break;
            Delay(5);
        }
        printf("PLUGINS total=%u online=%u\n",d.plugin_count,d.online_count);
        if (d.plugin_count!=5 || d.online_count!=5 || d.listed_plugins!=5) ok=0;
        for (j=0;j<d.listed_plugins;++j) {
            printf("PLUGIN %s online=%u updates=%lu\n",d.plugins[j].id,
                d.plugins[j].online,d.plugins[j].updates);
            if (!d.plugins[j].online || !d.plugins[j].updates) ok=0;
        }
        if (!qa_stop()) {ok=0;break;}
        Delay(40);
    }
    CloseLibrary(WorkbenchBase);
    qa_finish();
    if (ok) puts("PASS: component lookup, 6 Shell/Workbench launch cases, 30 live plugins");
    return ok ? 0:10;
}
