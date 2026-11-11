#include "../plugin_src/network/nd_netcontrol.h"
#include "nd_config.h"
#include "nd_option_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static struct NDConfig cfg,previous;
static int calls,fail_at;
static int execute_shutdown(void *context,const char *command)
{
    (void)context;
    ++calls;
    assert(!strcmp(command,calls==1 ? "C:ConfigureNetInterface A2065 OFFLINE QUIET":
                                    "C:NetShutdown TIMEOUT=5 QUIET"));
    return calls==fail_at ? 5:0;
}
int main(void)
{
    struct NCConfig c;char command[256];FILE *f;int i,net=-1,road=-1,enabled=0;
    struct NCInterfaces interfaces;
    struct NCDeviceStatus status;
    nc_defaults(&c);
    strcpy(c.traffic.device,"wrong.device");c.traffic.unit=7;
    nc_sampling_mode(&c,1);assert(!strcmp(c.traffic.device,"wrong.device") && c.traffic.unit==7);
    nc_sampling_mode(&c,0);assert(!c.traffic.device[0] && c.traffic.unit==0);
    assert(!nc_quiesce_command(&c,command,sizeof(command)));
    assert(nc_shutdown(&c,execute_shutdown,NULL)==NC_QUIESCE_FAILED && calls==0);
    strcpy(c.interface_name,"A2065");
    assert(!nc_quiesce_command(&c,command,8));
    calls=0;fail_at=0;assert(nc_shutdown(&c,execute_shutdown,NULL)==0 && calls==2);
    calls=0;fail_at=1;assert(nc_shutdown(&c,execute_shutdown,NULL)==NC_QUIESCE_FAILED && calls==1);
    calls=0;fail_at=2;assert(nc_shutdown(&c,execute_shutdown,NULL)==5 && calls==2);
    strcpy(c.interface_name,"bad;cmd");calls=0;
    assert(nc_shutdown(&c,execute_shutdown,NULL)==NC_QUIESCE_FAILED && calls==0);
    memset(&status,0,sizeof(status));nc_defaults(&c);
    assert(nc_job_deadline(&c,NC_OFFLINE)==750 && nc_job_deadline(&c,NC_PROBE)==750);
    assert(nc_job_deadline(&c,NC_ONLINE)==3750);
    nc_device_status_line(&status,"Device name                    = a2065.device\n");
    assert(!nc_device_status_apply(&c.traffic,&status));
    nc_device_status_line(&status,"Device unit number             = 2\n");
    assert(nc_device_status_apply(&c.traffic,&status));
    assert(!strcmp(c.traffic.device,"a2065.device") && c.traffic.unit==2);
    nc_device_status_line(&status,"Device name = wrong.device");
    assert(!nc_device_status_apply(&c.traffic,&status) && !strcmp(c.traffic.device,"a2065.device"));
    nc_defaults(&c);nc_device_status_line(&status,"Device unit = -1");
    assert(!nc_device_status_apply(&c.traffic,&status));
    nc_device_status_line(&status,"Device unit = 999");
    assert(!nc_device_status_apply(&c.traffic,&status));
    nc_device_status_line(&status,"Device unit = 0");
    assert(nc_device_status_apply(&c.traffic,&status) && c.traffic.unit==0);
    memset(&interfaces,0,sizeof(interfaces));
    nc_status_line(&interfaces,"Name MTU Type Address Received");
    nc_status_line(&interfaces," Sent Dropped Overruns Unknown Status");
    nc_status_line(&interfaces,"ZZ9000Net 1500 Ethernet 10.0.2.20 2");
    nc_status_line(&interfaces,"5 0 0 0 Down");
    assert(!interfaces.active[0]);
    nc_status_line(&interfaces,"lo0 16384 Loopback 127.0.0.1 0 0 Up");
    assert(!interfaces.active[0]);
    nc_status_line(&interfaces,"A2065 1500 Ethernet 10.0.2.15 2");
    nc_status_line(&interfaces,"5 0 0 0 Up");
    assert(!strcmp(interfaces.active,"A2065"));
    nc_status_line(&interfaces,"eth0 1500 Ethernet 192.168.0.2 42 8 Up");
    assert(!strcmp(interfaces.active,"A2065"));
    memset(&interfaces,0,sizeof(interfaces));
    nc_status_line(&interfaces,"bad;name 1500 Ethernet 10.0.2.15 2 Up");
    nc_status_line(&interfaces,"notIPv4 1500 Ethernet 999.0.0.1 2 Up");
    nc_status_line(&interfaces,"negative -1 Ethernet 10.0.2.15 2 Up");
    nc_status_line(&interfaces,"verylonginterfacename 1500 Ethernet 10.0.2.15 2 Up");
    assert(!interfaces.active[0]);
    nc_defaults(&c);assert(!strcmp(c.interface_name,"AUTO"));assert(!nc_command(&c,NC_ONLINE,command,sizeof(command)));
    assert(nc_option(&c,"DEBUG_LOG","YES"));assert(nc_option(&c,"DEBUG_LOG","NO"));
    assert(!nc_option(&c,"DEBUG_LOG","MAYBE"));
    assert(nc_option(&c,"STACK","ROADSHOW"));assert(!nc_option(&c,"STACK","MIAMI"));
    assert(nc_option(&c,"INTERFACE","ZZ9000Net"));assert(!nc_option(&c,"INTERFACE","eth0;delete"));
    assert(nc_command(&c,NC_ONLINE,command,sizeof(command)) && !strcmp(command,"C:AddNetInterface ZZ9000Net QUIET TIMEOUT=60"));
    assert(nc_command(&c,NC_OFFLINE,command,sizeof(command)) && !strcmp(command,"C:NetShutdown TIMEOUT=5 QUIET"));
    assert(nc_command(&c,NC_PROBE,command,sizeof(command)) && !strcmp(command,"C:GetNetStatus CHECK=INTERFACES QUIET"));
    /* A preferences save after a prior ZZ9000 connection must replace the
     * cached choice, including when sampling options precede INTERFACE. */
    f=fopen("tests/net-interface.tmp","w");assert(f);
    fputs("OPTION.NET.DEVICE=\nOPTION.NET.UNIT=0\nOPTION.NET.STACK=ROADSHOW\nOPTION.NET.INTERFACE=A2065\nOPTION.NET.DEBUG_LOG=YES\nOPTION.NET.VIEW=METER\nOPTION.NET.SAMPLING_OVERRIDE=NO\n",f);fclose(f);
    nc_defaults(&c);
    assert(nd_plugin_options_read("tests/net-interface.tmp","NET",nc_option,&c)==1);
    assert(!strcmp(c.interface_name,"A2065") && !c.traffic.device[0]);
    assert(nc_command(&c,NC_ONLINE,command,sizeof(command)) && !strcmp(command,"C:AddNetInterface A2065 QUIET TIMEOUT=60"));
    remove("tests/net-interface.tmp");
    assert(!nc_option(&c,"DHCP_TIMEOUT","999"));assert(!nc_command(&c,NC_ONLINE,command,8));
    f=fopen("tests/net-interface.tmp","w");assert(f);fputs("# sample\n device = \"DEVS:Networks/test.device\"\nUnit=2 # note\nconfigure=dhcp\n",f);fclose(f);
    assert(nc_interface_file(&c.traffic,"tests/net-interface.tmp"));
    assert(!strcmp(c.traffic.device,"DEVS:Networks/test.device") && c.traffic.unit==2);
    strcpy(c.traffic.device,"override.device");c.traffic.unit=7;
    assert(nc_interface_file(&c.traffic,"tests/net-interface.tmp") && c.traffic.unit==7);
    remove("tests/net-interface.tmp");
    nd_default_config(&cfg);
    for(i=0;i<cfg.count;++i){if(!strcmp(cfg.items[i].id,"NET"))net=i;assert(strcmp(cfg.items[i].id,"ROADSHOW"));}
    assert(net>=0);
    assert(nd_parse_line(&cfg,"PLUGIN=Roadshow|Plugins/Roadshow|ROADSHOW|METER|ON"));
    assert(nd_option_set(&cfg,"ROADSHOW","INTERFACE","legacy.device"));
    nd_upgrade_network_config(&cfg);
    assert(!strcmp(cfg.items[nd_visible_to_full(&cfg,6)].id,"NET"));
    for(i=0;i<cfg.count;++i)assert(strcmp(cfg.items[i].id,"ROADSHOW"));
    assert(!strcmp(nd_option_get(&cfg,"ROADSHOW","INTERFACE"),"legacy.device"));
    previous=cfg;nd_upgrade_network_config(&cfg);assert(nd_config_runtime_equal(&cfg,&previous));
    nd_default_config(&cfg);cfg.items[net].enabled=1;
    assert(nd_parse_line(&cfg,"PLUGIN=Roadshow|Plugins/Roadshow|ROADSHOW|METER|ON"));
    cfg.rows=2;cfg.row_layout=1;cfg.row_counts[0]=4;cfg.row_counts[1]=4;
    nd_upgrade_network_config(&cfg);
    for(i=0;i<cfg.count;++i){assert(strcmp(cfg.items[i].id,"ROADSHOW"));if(cfg.items[i].enabled && !strcmp(cfg.items[i].id,"NET"))++enabled;}
    assert(enabled==1 && cfg.items[net].enabled);
    assert(cfg.row_counts[0]==4 && cfg.row_counts[1]==3);
    memset(&cfg,0,sizeof(cfg));cfg.rows=1;
    assert(nd_parse_line(&cfg,"PLUGIN=Roadshow|Plugins/Roadshow|ROADSHOW|GRAPH|ON"));
    nd_upgrade_network_config(&cfg);
    assert(cfg.count==1 && cfg.items[0].enabled && cfg.items[0].graph);
    assert(!strcmp(cfg.items[0].id,"NET") && !strcmp(cfg.items[0].command,"Plugins/Network"));
    nd_default_config(&cfg);
    assert(!strcmp(nd_option_get(&cfg,"NET","VIEW"),"METER"));
    cfg.items[net].graph=1;
    assert(!strcmp(nd_option_get(&cfg,"NET","VIEW"),"GRAPH"));
    previous=cfg;
    assert(nd_option_set(&cfg,"NET","VIEW","METER") && !cfg.items[net].graph);
    assert(!nd_options_changed(&previous,&cfg,"NET"));
    assert(nd_config_can_update_live(&previous,&cfg));
    assert(nd_option_set(&cfg,"NET","VIEW","GRAPH") && cfg.items[net].graph);
    assert(!nd_option_set(&cfg,"NET","VIEW","BAD"));
    f=fopen("tests/net-interface.tmp","w");assert(f);
    fputs("OPTION.NET.VIEW=GRAPH\nPLUGIN=Network|Plugins/Network|NET|METER|ON\n",f);fclose(f);
    assert(nd_load_config(&cfg,"tests/net-interface.tmp")==0);
    for(i=0;i<cfg.count;++i)if(!strcmp(cfg.items[i].id,"NET"))assert(cfg.items[i].graph);
    remove("tests/net-interface.tmp");
    assert(nc_option(&c,"VIEW","METER") && nc_option(&c,"VIEW","GRAPH"));
    assert(!nc_option(&c,"VIEW","BAD"));
    nd_default_config(&cfg);
    assert(nd_option_set(&cfg,"NET","DEVICE","DEVS:Networks/ZZ9000Net.device"));
    assert(nd_option_set(&cfg,"NET","UNIT","7"));
    nd_upgrade_network_config(&cfg);
    assert(!*nd_option_get(&cfg,"NET","DEVICE") && !strcmp(nd_option_get(&cfg,"NET","UNIT"),"0"));
    assert(nd_option_set(&cfg,"NET","SAMPLING_OVERRIDE","YES"));
    assert(nd_option_set(&cfg,"NET","DEVICE","custom.device"));
    assert(nd_option_set(&cfg,"NET","UNIT","2"));
    nd_upgrade_network_config(&cfg);
    assert(!strcmp(nd_option_get(&cfg,"NET","DEVICE"),"custom.device") && !strcmp(nd_option_get(&cfg,"NET","UNIT"),"2"));
    assert(nc_option(&c,"SAMPLING_OVERRIDE","NO") && nc_option(&c,"SAMPLING_OVERRIDE","YES"));
    assert(!nc_option(&c,"SAMPLING_OVERRIDE","BAD"));
    nd_upgrade_network_config(NULL);
    puts("Unified Network provider commands, automatic device metadata and legacy tile migration passed.");return 0;
}
