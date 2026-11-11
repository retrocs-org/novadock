#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_amirc.h"
#include "nd_manifest.h"
#include "nd_options.h"
static void manifest_test(void)
{
    struct NDManifest m;struct NDConfig cfg;struct NDOptionDef defs[10];
    char line[300];FILE *f=fopen("Plugins/AmIRC.ndplugin","r");int n;
    assert(f);nd_manifest_init(&m,"AmIRC");
    while(fgets(line,sizeof(line),f)) assert(nd_manifest_line(&m,line));
    fclose(f);assert(nd_manifest_package_ready(&m));
    assert(m.option_count==6 && !strcmp(m.id,"AMIRC"));
    assert(!strcmp(m.options[0].type,"FILE"));
    nd_default_config(&cfg);n=cfg.count;
    assert(nd_manifest_merge(&cfg,&m,"Plugins/AmIRC")==ND_MANIFEST_ADDED);
    assert(cfg.count==n+1 && !cfg.items[n].enabled);
    assert(nd_option_collect(&cfg,"AMIRC",defs,8)==6);
    assert(nd_manifest_bundled_id("AMIRC"));
    nd_manifest_init(&m,"Clipboard");f=fopen("Plugins/Clipboard.ndplugin","r");assert(f);
    while(fgets(line,sizeof(line),f))assert(nd_manifest_line(&m,line));
    fclose(f);assert(nd_manifest_package_ready(&m));
    assert(nd_manifest_bundled_id("CLIPBOARD"));
    assert(nd_manifest_merge(&cfg,&m,"Plugins/Clipboard")==ND_MANIFEST_ADDED);
    assert(nd_option_collect(&cfg,"CLIPBOARD",defs,8)==2);
    assert(nd_option_set(&cfg,"CLIPBOARD","LIMIT","32"));
    assert(!nd_option_set(&cfg,"CLIPBOARD","LIMIT","33"));
    assert(nd_option_set(&cfg,"AMIRC","ALERTS","HIGHLIGHT"));
    assert(!nd_option_set(&cfg,"AMIRC","ALERTS","unknown"));
    nd_manifest_init(&m,"Network");f=fopen("Plugins/Network.ndplugin","r");assert(f);
    while(fgets(line,sizeof(line),f))assert(nd_manifest_line(&m,line));
    fclose(f);assert(nd_manifest_package_ready(&m));
    assert(m.option_count==10 && !strcmp(m.id,"NET"));
    assert(nd_option_collect(&cfg,"NET",defs,10)==10);
    assert(nd_option_group_settings("NET",defs,10)==7);
    assert(!strcmp(defs[7].key,"SAMPLING_OVERRIDE"));
    assert(!strcmp(defs[8].key,"DEVICE") && !strcmp(defs[9].key,"UNIT"));
    assert(!strcmp(defs[0].key,"VIEW") && !strcmp(defs[2].key,"INTERFACE"));
    assert(!strcmp(nd_option_get(&cfg,"NET","SAMPLING_OVERRIDE"),"NO"));
    assert(!strcmp(nd_option_get(&cfg,"NET","DEBUG_LOG"),"NO"));
    assert(nd_option_set(&cfg,"NET","DEBUG_LOG","YES"));
    assert(!nd_option_set(&cfg,"NET","DEBUG_LOG","MAYBE"));
}
int main(void)
{
    struct NDAmIRCConfig cfg;char caption[32],value[32],quoted[64];
    assert(!nd_amirc_connection_indicator(0,0,0)); /* app absent */
    assert(!nd_amirc_connection_indicator(0,1,0)); /* stale cached state */
    assert(!nd_amirc_connection_indicator(1,-1,0)); /* checking */
    assert(!nd_amirc_connection_indicator(1,0,0)); /* disconnected */
    assert(!nd_amirc_connection_indicator(1,1,1)); /* stalled client */
    assert(nd_amirc_connection_indicator(1,1,0)); /* connected (also away) */
    nd_amirc_defaults(&cfg);
    assert(!cfg.program[0] && !strcmp(cfg.port,"AUTO") && cfg.poll_ticks==100);
    assert(nd_amirc_port_name("AMIRC") && nd_amirc_port_name("AMIRC.1"));
    assert(nd_amirc_port_name("AmIRC.12"));
    assert(!nd_amirc_port_name("AMIRCevil") && !nd_amirc_port_name("AMIRC.DCC"));
    assert(!nd_amirc_port_name("AMIRC.") && !nd_amirc_port_name("AMIR"));
    assert(!nd_amirc_port_name(0));
    assert(nd_amirc_apply_option(&cfg,"PROGRAM","Work:Chat Tools/AmIRC"));
    assert(!nd_amirc_apply_option(&cfg,"PROGRAM","bad\ncommand"));
    assert(!nd_amirc_apply_option(&cfg,"DISPLAY","CONNECT"));
    assert(nd_amirc_apply_option(&cfg,"DISPLAY","CHANNEL"));
    assert(!strcmp(nd_amirc_display_query(cfg.display),"GETCHANNEL"));
    assert(!strcmp(nd_amirc_display_query(ND_AMIRC_NICK),"GETMYNICK"));
    assert(!strcmp(nd_amirc_display_query(ND_AMIRC_SERVER),"GETSERVERNAME"));
    assert(!nd_amirc_display_query(ND_AMIRC_STATUS));
    assert(!nd_amirc_apply_option(&cfg,"POLL_TICKS","0"));
    assert(!nd_amirc_apply_option(&cfg,"POLL_TICKS","100oops"));
    assert(nd_amirc_apply_option(&cfg,"POLL_TICKS","3000"));
    nd_amirc_status(caption,32,value,32,0,1,0,ND_AMIRC_NICK,"nick",0);
    assert(!strcmp(value,"OFFLINE"));
    nd_amirc_status(caption,32,value,32,1,0,0,ND_AMIRC_NICK,"oldnick",0);
    assert(!strcmp(value,"DISCONNECTED"));
    nd_amirc_status(caption,32,value,32,1,1,1,ND_AMIRC_NICK,"nick",0);
    assert(!strcmp(caption,"AMIRC AWAY") && !strcmp(value,"nick"));
    nd_amirc_status(caption,32,value,32,1,1,0,ND_AMIRC_CHANNEL,"#amiga",0);
    assert(!strcmp(value,"#amiga"));
    nd_amirc_status(caption,32,value,32,1,1,0,ND_AMIRC_STATUS,"ignored",0);
    assert(!strcmp(value,"CONNECTED"));
    nd_amirc_status(caption,32,value,32,1,1,0,ND_AMIRC_NICK,"oldnick",1);
    assert(!strcmp(value,"NO RESPONSE"));
    nd_amirc_status(caption,32,value,32,1,-1,0,ND_AMIRC_NICK,"oldnick",0);
    assert(!strcmp(value,"CHECKING"));
    nd_amirc_clean(value,32,"\033[31m#amiga\033[0m\n\t ");
    assert(!strcmp(value,"#amiga"));
    nd_amirc_clean(value,4,"123456");assert(!strcmp(value,"123"));
    assert(nd_amirc_quote(quoted,sizeof(quoted),"Work:Chat Tools/AmIRC"));
    assert(!strcmp(quoted,"\"Work:Chat Tools/AmIRC\""));
    assert(nd_amirc_quote(quoted,sizeof(quoted),"Work:a*b\"c"));
    assert(!strcmp(quoted,"\"Work:a**b*\"c\""));
    assert(!nd_amirc_quote(quoted,4,"too long"));
    assert(!nd_amirc_quote(quoted,sizeof(quoted),"bad\ncommand"));
    manifest_test();puts("AmIRC settings, port matching, status, quoting and manifest passed.");
    {
        struct NDAmIRCAlerts alerts;char badge[8];alerts.count=0;
        assert(cfg.alerts==3 && cfg.clear_on_focus==1);
        assert(nd_amirc_alert_command("NOTIFY PRIVATE AmIRC.9","AmIRC.9",3));
        assert(nd_amirc_alert_command("NOTIFY PRIVATE AMIRC.9","AmIRC.9",3));
        assert(nd_amirc_alert_command("NOTIFY HIGHLIGHT AmIRC.9","AmIRC.9",2));
        assert(!nd_amirc_alert_command("NOTIFY PRIVATE AmIRC.8","AmIRC.9",3));
        assert(!nd_amirc_alert_command("NOTIFY PRIVATE AmIRC.9","AmIRC.9",2));
        assert(!nd_amirc_alert_command("NOTIFY HIGHLIGHT AmIRC.9 extra","AmIRC.9",3));
        assert(!nd_amirc_alert_command("BAD PRIVATE AmIRC.9","AmIRC.9",3));
        nd_amirc_alert_add(&alerts);nd_amirc_badge(badge,sizeof(badge),alerts.count,1);
        assert(!strcmp(badge,"1"));nd_amirc_badge(badge,sizeof(badge),0,1);assert(!strcmp(badge,"*"));
        nd_amirc_badge(badge,sizeof(badge),0,0);assert(!badge[0]);
        alerts.count=10000;nd_amirc_alert_add(&alerts);assert(alerts.count==10000);
        nd_amirc_badge(badge,sizeof(badge),alerts.count,1);assert(!strcmp(badge,"999+"));
        assert(nd_amirc_apply_option(&cfg,"ALERTS","PRIVATE") && cfg.alerts==1);
        assert(nd_amirc_apply_option(&cfg,"CLEAR_ON_FOCUS","NO") && !cfg.clear_on_focus);
        assert(!nd_amirc_apply_option(&cfg,"ALERTS","ALL"));
    }
    return 0;
}
