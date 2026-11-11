/* Unified Network tile. Provider control runs in an independent process;
 * the sampler keeps answering dock IPC during DHCP and shutdown waits. */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <exec/io.h>
#include <devices/sana2.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/expansion.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_client.h"
#include "nd_option_runtime.h"
#include "nd_version.h"
#include "nd_netcontrol.h"
const char nd_version[]=ND_VER_STRING("Network");
long __stack=16384;
struct ExpansionBase *ExpansionBase=NULL;
#define NC_MAGIC 0x4e434e54UL
#define NC_WORKER_ABI "2"
#define NC_NOTICE_PENDING 3
#define NC_NOTICE_BUSY 4
#define NC_NOTICE_ONLINE_TIMEOUT 5
#define NC_NOTICE_OFFLINE_TIMEOUT 6
#define NC_NOTICE_DEVICE_BUSY 7
#define NC_NOTICE_QUIESCE_FAILED 8
#define NC_NOTICE_LOG_FAILED 9
struct NCMessage {struct Message msg;ULONG magic,job;int kind,action,installed,online,ok,result;struct NCConfig config;};
struct Sampler {struct MsgPort *port;struct IOSana2Req *io;struct Sana2DeviceStats stats;ULONG rx,tx,when,age,next_open;int opened,pending,prior,closing,aborted,fresh;};
static ULONG now_ticks(void)
{struct DateStamp d;DateStamp(&d);return ((ULONG)d.ds_Days*1440UL+d.ds_Minute)*3000UL+d.ds_Tick;}
static int exists(const char *path)
{BPTR lock=Lock((STRPTR)path,ACCESS_READ);if(lock)UnLock(lock);return lock!=0;}
static int stack_resident(void)
{
#ifdef ND_NETWORK_TEST
    return exists("Share:mock-network-up");
#else
    int present;Forbid();present=FindName(&SysBase->LibList,"bsdsocket.library")!=NULL;Permit();return present;
#endif
}
static int roadshow_available(void)
{return exists("C:RoadshowControl") && exists("C:GetNetStatus") && exists("C:AddNetInterface") && exists("C:NetShutdown");}
static int network_stopping(void)
{struct MsgPort *p;Forbid();p=FindPort("NovaDock.NetworkShutdown");Permit();return p!=NULL;}
static void shutdown_guard(struct MsgPort *guard,int active)
{
    Forbid();
    if(active && !guard->mp_Node.ln_Name && !FindPort("NovaDock.NetworkShutdown")){
        guard->mp_Node.ln_Name="NovaDock.NetworkShutdown";AddPort(guard);
    }else if(!active && guard->mp_Node.ln_Name){RemPort(guard);guard->mp_Node.ln_Name=NULL;}
    Permit();
}
static LONG command(const char *text)
{
    BPTR in=Open("NIL:",MODE_OLDFILE),out=Open("NIL:",MODE_NEWFILE);LONG rc;
    if(!in || !out){if(in)Close(in);if(out)Close(out);return 20;}
    rc=SystemTags((STRPTR)text,SYS_Input,in,SYS_Output,out,TAG_DONE);Close(in);Close(out);return rc;
}
/* Roadshow's syslog console is independent of command stdout/QUIET.
 * Follow its documented Network-Startup pattern: start NetLogViewer hidden
 * before control operations. Re-running the viewer raises its window, so
 * quiet operations must reuse an existing process without relaunching it. */
static int log_name(const char *text)
{
    const char *base=text,*p;char name[13];unsigned int n=0;
    if(!text)return 0;
    for(p=text;*p;++p)if(*p=='/' || *p==':')base=p+1;
    for(p=base;*p && n<sizeof(name)-1;++p){
        unsigned char c=(unsigned char)*p;
        name[n++]=(char)(c>='a' && c<='z' ? c-'a'+'A':c);
    }
    name[n]=0;return !*p && !strcmp(name,"NETLOGVIEWER");
}
static int log_task(struct Task *task)
{
    struct Process *process;struct CommandLineInterface *cli;
    unsigned char *name;char command_name[96];unsigned int n;
    if(log_name(task->tc_Node.ln_Name))return 1;
    if(task->tc_Node.ln_Type!=NT_PROCESS)return 0;
    process=(struct Process *)task;if(!process->pr_CLI)return 0;
    cli=(struct CommandLineInterface *)BADDR(process->pr_CLI);
    if(!cli->cli_CommandName)return 0;
    name=(unsigned char *)BADDR(cli->cli_CommandName);n=name[0];
    if(n>=sizeof(command_name))return 0;
    memcpy(command_name,name+1,n);command_name[n]=0;
    return log_name(command_name);
}
static int log_viewer_present(void)
{
    struct Node *node;int found=0;
    Forbid();
    for(node=SysBase->TaskReady.lh_Head;node->ln_Succ;node=node->ln_Succ)
        if(log_task((struct Task *)node)){found=1;break;}
    if(!found)for(node=SysBase->TaskWait.lh_Head;node->ln_Succ;node=node->ln_Succ)
        if(log_task((struct Task *)node)){found=1;break;}
    Permit();return found;
}
static int prepare_network_log(void)
{
    char setting[8],cmd[64];BPTR in,out;LONG rc;int present,show,i;
    (void)nd_plugin_option_string("NET","DEBUG_LOG","NO",setting,sizeof(setting));
    show=!strcmp(setting,"YES");present=log_viewer_present();
    if(present && !show)return 1;
    if(!exists("C:NetLogViewer"))return 0;
    sprintf(cmd,"C:NetLogViewer CX_POPUP=%s",show ? "YES":"NO");
    in=Open("NIL:",MODE_OLDFILE);out=Open("NIL:",MODE_NEWFILE);
    if(!in || !out){if(in)Close(in);if(out)Close(out);return 0;}
    rc=SystemTags(cmd,SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,
                  NP_StackSize,32768UL,TAG_DONE);
    if(rc==-1){Close(in);Close(out);return 0;}
    if(present)return 1;
    for(i=0;i<250;++i){
        if(log_viewer_present()){Delay(10);return 1;}
        Delay(1);
    }
    return 0;
}
static int send_to(const char *name,struct NCMessage *m)
{struct MsgPort *p;Forbid();p=FindPort((STRPTR)name);if(p)PutMsg(p,(struct Message *)m);Permit();return p!=NULL;}
static int active_interface(char *name)
{
    struct NCInterfaces interfaces;char path[80],line[512];BPTR in,out;FILE *file;LONG rc=20;
    if(network_stopping() || !stack_resident() || !exists("C:ShowNetStatus"))return 0;
    memset(&interfaces,0,sizeof(interfaces));
    sprintf(path,"T:NovaDockNetInterfaces-%08lx",(ULONG)FindTask(NULL));
    in=Open("NIL:",MODE_OLDFILE);out=Open(path,MODE_NEWFILE);
    if(in && out)rc=SystemTags("C:ShowNetStatus INTERFACES",SYS_Input,in,SYS_Output,out,TAG_DONE);
    if(in)Close(in);if(out)Close(out);
    file=fopen(path,"r");
    if(file){while(fgets(line,sizeof(line),file))nc_status_line(&interfaces,line);fclose(file);}
    DeleteFile(path);
    if(rc!=0 || !interfaces.active[0])return 0;
    strcpy(name,interfaces.active);return 1;
}
static void runtime_device(struct NCConfig *cfg)
{
    struct NCDeviceStatus status;char path[80],cmd[128],line[384];BPTR in,out;FILE *file;LONG rc=20;
    if(cfg->traffic.device[0] || !strcmp(cfg->interface_name,"AUTO") ||
       !rd_valid_interface(cfg->interface_name) || network_stopping() || !stack_resident() || !exists("C:ShowNetStatus"))return;
    memset(&status,0,sizeof(status));
    sprintf(path,"T:NovaDockNetDevice-%08lx",(ULONG)FindTask(NULL));
    sprintf(cmd,"C:ShowNetStatus INTERFACE=\"%s\"",cfg->interface_name);
    in=Open("NIL:",MODE_OLDFILE);out=Open(path,MODE_NEWFILE);
    if(in && out)rc=SystemTags(cmd,SYS_Input,in,SYS_Output,out,TAG_DONE);
    if(in)Close(in);if(out)Close(out);
    file=fopen(path,"r");
    if(file){while(fgets(line,sizeof(line),file))nc_device_status_line(&status,line);fclose(file);}
    DeleteFile(path);if(rc==0)(void)nc_device_status_apply(&cfg->traffic,&status);
}
static void resolve_metadata(struct NCConfig *cfg)
{
    char path[96];
    sprintf(path,"DEVS:NetInterfaces/%s",cfg->interface_name);(void)nc_interface_file(&cfg->traffic,path);
    runtime_device(cfg);
    if(!cfg->traffic.device[0]){
        sprintf(path,"SYS:Storage/NetInterfaces/%s",cfg->interface_name);(void)nc_interface_file(&cfg->traffic,path);
    }
}
static int roadshow_command(const struct NCConfig *cfg,int action,char *out,unsigned int size)
{
    char path[96],cmd[256];
    if(!nc_command(cfg,action,out,size))return 0;
    if(action!=NC_ONLINE)return 1;
    sprintf(path,"DEVS:NetInterfaces/%s",cfg->interface_name);
    if(!exists(path)){
        sprintf(path,"SYS:Storage/NetInterfaces/%s",cfg->interface_name);
        if(!exists(path))return 0;
    }
    sprintf(cmd,"C:AddNetInterface %s QUIET TIMEOUT=%u",path,cfg->timeout);
    if(strlen(cmd)>=size)return 0;strcpy(out,cmd);return 1;
}
static int control_execute(void *context,const char *text)
{(void)context;return (int)command(text);}
static int roadshow_offline(const struct NCConfig *cfg)
{return nc_shutdown(cfg,control_execute,NULL);}
static int a2065_present(void)
{
    int found=0;
    /* Commodore A2065 autoconfig IDs.
     * Inspect the board list only; do not open or initialize every driver. */
    ExpansionBase=(struct ExpansionBase *)OpenLibrary("expansion.library",0);
    if(ExpansionBase){found=FindConfigDev(NULL,514L,112L)!=NULL;CloseLibrary((struct Library *)ExpansionBase);ExpansionBase=NULL;}
    return found;
}
static void resolve_interface(struct NCConfig *cfg)
{
    struct FileInfoBlock *fib;BPTR lock;char selected[16]="",path[96];struct NDNetConfig test;
    if(strcmp(cfg->interface_name,"AUTO")){
        resolve_metadata(cfg);return;
    }
    if(active_interface(selected)){
        strcpy(cfg->interface_name,selected);
        resolve_metadata(cfg);
        return;
    }
    if(a2065_present() && exists("DEVS:Networks/a2065.device") &&
       (exists("DEVS:NetInterfaces/A2065") || exists("SYS:Storage/NetInterfaces/A2065"))){
        strcpy(cfg->interface_name,"A2065");resolve_metadata(cfg);return;
    }
    lock=Lock("DEVS:NetInterfaces",ACCESS_READ);if(!lock)return;
    fib=AllocDosObject(DOS_FIB,NULL);
    if(fib && Examine(lock,fib))while(ExNext(lock,fib)){
        if(fib->fib_DirEntryType>=0 || !rd_interface_filename(fib->fib_FileName))continue;
        if(selected[0] && strcmp(fib->fib_FileName,selected)>=0)continue;
        nd_net_defaults(&test);sprintf(path,"DEVS:NetInterfaces/%s",fib->fib_FileName);
        if(nc_interface_file(&test,path))strcpy(selected,fib->fib_FileName);
    }
    if(fib)FreeDosObject(DOS_FIB,fib);UnLock(lock);
    if(selected[0]){strcpy(cfg->interface_name,selected);sprintf(path,"DEVS:NetInterfaces/%s",selected);(void)nc_interface_file(&cfg->traffic,path);}
}
/* A provider owns control, detection and interface metadata; the controller
 * and SANA-II sampler remain independent of the concrete stack. */
struct StackBackend {
    const char *name;int (*available)(void);int (*resident)(void);
    void (*resolve)(struct NCConfig *);int (*command)(const struct NCConfig *,int,char *,unsigned int);
    int (*offline)(const struct NCConfig *);
};
static const struct StackBackend backends[]={
    {"ROADSHOW",roadshow_available,stack_resident,resolve_interface,roadshow_command,roadshow_offline},
    {NULL,NULL,NULL,NULL,NULL,NULL}
};
static const struct StackBackend *backend_for(const struct NCConfig *cfg)
{
    int i;for(i=0;backends[i].name;++i)
        if(!strcmp(cfg->stack,backends[i].name) || (!strcmp(cfg->stack,"AUTO") && backends[i].available()))return &backends[i];
    return NULL;
}
static void load_config(struct NCConfig *cfg);
static int worker(const char *port,ULONG job)
{
    struct NCMessage *m=AllocVec(sizeof(*m),MEMF_PUBLIC|MEMF_CLEAR);struct MsgPort *reply=CreateMsgPort();char cmd[256];const struct StackBackend *backend;
    if(!m || !reply){if(m)FreeVec(m);if(reply)DeleteMsgPort(reply);return 20;}
    ((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
    m->msg.mn_Length=sizeof(*m);m->msg.mn_ReplyPort=reply;m->magic=NC_MAGIC;m->kind=1;m->job=job;
    if(!send_to(port,m)){DeleteMsgPort(reply);FreeVec(m);return 10;}
    WaitPort(reply);GetMsg(reply);DeleteMsgPort(reply);m->msg.mn_ReplyPort=NULL;
    if(m->kind!=1 || m->action<0){FreeVec(m);return 10;}
    /* A resident controller may predate a preferences save (or executable
     * update). Online must use the current selection, never its cached
     * discovery/last-online snapshot. Resolve AUTO afresh below as well. */
    if(m->action==NC_ONLINE)load_config(&m->config);
    backend=backend_for(&m->config);m->installed=backend && backend->available();m->online=0;m->ok=1;
    if(m->installed){
        if(m->action && !prepare_network_log()){
            m->result=NC_LOG_FAILED;m->ok=0;goto completed;
        }
        /* Shutdown does not need interface discovery, and querying a stack
         * during shutdown can acquire the very resources it is draining. */
        if(m->action!=NC_OFFLINE)backend->resolve(&m->config);
        if(m->action){
            m->result=m->action==NC_OFFLINE && backend->offline ? backend->offline(&m->config):
                backend->command(&m->config,m->action,cmd,sizeof(cmd)) ? (int)command(cmd):20;
            m->ok=m->result==0;
        }
        if(m->action!=NC_OFFLINE && backend->resident() && backend->command(&m->config,NC_PROBE,cmd,sizeof(cmd)))m->online=command(cmd)==0;
    }else {m->ok=m->action==NC_PROBE;m->result=20;}
completed:
    m->kind=2;if(!send_to(port,m))FreeVec(m);return 0;
}
static int quote(char *out,unsigned int size,const char *path)
{
    unsigned int n=0;unsigned char ch;out[n++]='"';
    while((ch=(unsigned char)*path++)!=0){if(ch<32 || ch==127 || n+4>=size)return 0;if(ch=='"' || ch=='*')out[n++]='*';out[n++]=(char)ch;}
    out[n++]='"';out[n]=0;return 1;
}
static int launch_worker(const char *port,const char *program,ULONG job)
{
    char path[512],cmd[1100],number[24];BPTR in,out;LONG rc;
    if(!GetProgramDir() || !NameFromLock(GetProgramDir(),path,sizeof(path)) || !AddPart(path,FilePart((STRPTR)program),sizeof(path)) || !quote(cmd,sizeof(cmd)-100,path))return 0;
    if(port){strcat(cmd," WORK ");strcat(cmd,port);sprintf(number," %lu ",job);strcat(cmd,number);strcat(cmd,NC_WORKER_ABI);}
    else strcat(cmd,job==NC_OFFLINE ? " OFFLINE_ERROR":
        job==NC_NOTICE_PENDING ? " SHUTDOWN_PENDING":
        job==NC_NOTICE_BUSY ? " NETWORK_BUSY":
        job==NC_NOTICE_ONLINE_TIMEOUT ? " ONLINE_TIMEOUT":
        job==NC_NOTICE_OFFLINE_TIMEOUT ? " OFFLINE_TIMEOUT":
        job==NC_NOTICE_DEVICE_BUSY ? " DEVICE_BUSY":
        job==NC_NOTICE_QUIESCE_FAILED ? " QUIESCE_ERROR":
        job==NC_NOTICE_LOG_FAILED ? " LOG_ERROR":" ONLINE_ERROR");
    in=Open("NIL:",MODE_OLDFILE);out=Open("NIL:",MODE_NEWFILE);
    if(!in || !out){if(in)Close(in);if(out)Close(out);return 0;}
    rc=SystemTags(cmd,SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,32768UL,TAG_DONE);
    if(rc==-1){Close(in);Close(out);return 0;}return 1;
}
static void close_sampler(struct Sampler *s)
{
    s->closing=1;
    if(s->pending){
        if(!s->aborted){AbortIO((struct IORequest *)s->io);s->aborted=1;}
        if(!CheckIO((struct IORequest *)s->io))return;
        WaitIO((struct IORequest *)s->io);s->pending=0;
    }
    if(s->opened)CloseDevice((struct IORequest *)s->io);
    s->opened=0;s->prior=0;s->closing=0;s->aborted=0;s->next_open=0;
}
static int open_sampler(struct Sampler *s,const struct NDNetConfig *cfg)
{
    if(!cfg->device[0] || s->pending)return 0;
    if(s->next_open && now_ticks()-s->next_open>0x80000000UL)return 0;
    if(!s->port)s->port=CreateMsgPort();
    if(s->port && !s->io)s->io=(struct IOSana2Req *)CreateIORequest(s->port,sizeof(*s->io));
    if(!s->io)return 0;s->io->ios2_BufferManagement=NULL;
    s->opened=OpenDevice((STRPTR)cfg->device,cfg->unit,(struct IORequest *)s->io,0)==0;
    s->next_open=s->opened ? 0:now_ticks()+250;return s->opened;
}
static void sample(struct Sampler *s,const struct NDNetConfig *cfg,unsigned int elapsed,char *value,unsigned int *meter,unsigned int *a,unsigned int *b)
{
    ULONG now,dt;unsigned long rx,tx;
    if(s->closing){close_sampler(s);return;}
    if(s->pending && s->age>=250){strcpy(value,"NO RESPONSE");*meter=ND_NO_METER;*a=*b=ND_GRAPH_NO_SAMPLE;}
    s->age+=elapsed;
    if(s->pending && CheckIO((struct IORequest *)s->io)){
        WaitIO((struct IORequest *)s->io);s->pending=0;s->fresh=1;now=now_ticks();
        if(s->io->ios2_Req.io_Error){strcpy(value,"NO STATS");s->prior=0;*meter=ND_NO_METER;*a=*b=ND_GRAPH_NO_SAMPLE;}
        else{
            dt=now-s->when;
            if(s->prior && nd_net_rates(s->rx,s->tx,s->stats.PacketsReceived,s->stats.PacketsSent,dt,&rx,&tx)){
                sprintf(value,"%lu/%lu p/s",rx,tx);*meter=nd_net_meter(rx,tx,cfg->scale_pps);
                *a=nd_net_channel_meter(rx,cfg->scale_pps);*b=nd_net_channel_meter(tx,cfg->scale_pps);
            }
            s->rx=s->stats.PacketsReceived;s->tx=s->stats.PacketsSent;s->when=now;s->prior=1;
        }
    }
    if(!s->pending && s->age>=cfg->poll_ticks){
        s->age=0;s->io->ios2_Req.io_Command=S2_GETGLOBALSTATS;s->io->ios2_Req.io_Error=0;
        s->io->ios2_Req.io_Flags=0;s->io->ios2_StatData=&s->stats;
        s->pending=1;s->aborted=0;SendIO((struct IORequest *)s->io);
    }
}
struct NCReadConfig {struct NCConfig *cfg;int override,have_interface,have_timeout;};
static int read_config_option(void *context,const char *key,const char *value)
{
    struct NCReadConfig *r=(struct NCReadConfig *)context;
    if(!nc_option(r->cfg,key,value))return 0;
    if(!strcmp(key,"SAMPLING_OVERRIDE"))r->override=!strcmp(value,"YES");
    if(!strcmp(key,"INTERFACE"))r->have_interface=1;
    if(!strcmp(key,"DHCP_TIMEOUT"))r->have_timeout=1;
    return 1;
}
static void load_config(struct NCConfig *cfg)
{
    struct NCReadConfig read;
    nc_defaults(cfg);memset(&read,0,sizeof(read));read.cfg=cfg;
    if(nd_plugin_options_effective("NET",read_config_option,&read)!=1) {
        nc_defaults(cfg);read.override=read.have_interface=read.have_timeout=0;
    }
    nc_sampling_mode(cfg,read.override);
    if(!read.have_interface || !read.have_timeout) {
        char interface_name[64],timeout[64],path[96];struct NDOptionString fields[2];
        fields[0].key="INTERFACE";fields[0].fallback="";fields[0].out=interface_name;fields[0].limit=sizeof(interface_name);
        fields[1].key="DHCP_TIMEOUT";fields[1].fallback="";fields[1].out=timeout;fields[1].limit=sizeof(timeout);
        nd_plugin_option_strings("ROADSHOW",fields,2);
        if(!read.have_interface && rd_valid_interface(interface_name)) {
            sprintf(path,"DEVS:NetInterfaces/%s",interface_name);
            if(exists(path))strcpy(cfg->interface_name,interface_name);
        }
        if(!read.have_timeout && timeout[0])(void)nc_option(cfg,"DHCP_TIMEOUT",timeout);
    }
}
#include "nd_netdetails.h"

int main(int argc,char **argv)
{
    struct NDClient client;struct NCConfig cfg,active,last_online,previous;struct Sampler *sampler;
    struct MsgPort *port,*guard;struct NCMessage *m;char portname[64],value[32]="CHECKING",traffic[32]="SAMPLING";
    unsigned int event,elapsed=0,meter=ND_NO_METER,a=ND_GRAPH_NO_SAMPLE,b=ND_GRAPH_NO_SAMPLE;
    ULONG poll=250,busy_age=0,busy_started=0,job=0,discovery_age=1500,drain_age=0;
    int installed=0,online=0,busy=0,claimed=0,action=0,queued=0,reload=1,quit=0,notice=0,timed_out=0,shutdown_pending=0;
    if(argc==2 && (!strcmp(argv[1],"ONLINE_ERROR") || !strcmp(argv[1],"OFFLINE_ERROR")))return network_error(!strcmp(argv[1],"OFFLINE_ERROR"));
    if(argc==2 && !strcmp(argv[1],"SHUTDOWN_PENDING"))return network_error(2);
    if(argc==2 && !strcmp(argv[1],"NETWORK_BUSY"))return network_error(3);
    if(argc==2 && !strcmp(argv[1],"ONLINE_TIMEOUT"))return network_error(4);
    if(argc==2 && !strcmp(argv[1],"OFFLINE_TIMEOUT"))return network_error(5);
    if(argc==2 && !strcmp(argv[1],"DEVICE_BUSY"))return network_error(6);
    if(argc==2 && !strcmp(argv[1],"QUIESCE_ERROR"))return network_error(7);
    if(argc==2 && !strcmp(argv[1],"LOG_ERROR"))return network_error(8);
    if((argc==2 && !strcmp(argv[1],"DETAILS")) ||
       (argc==3 && (!strcmp(argv[1],"NET") || !strcmp(argv[1],"ROADSHOW")) &&
        !strcmp(argv[2],"DETAILS")))return network_details();
    if(argc>1 && !strcmp(argv[1],"WORK")){
        if(argc==5 && !strcmp(argv[4],NC_WORKER_ABI))return worker(argv[2],strtoul(argv[3],NULL,10));
        return 20;
    }
    if(argc<2 || !nd_client_init(&client,argv[1]))return 20;
    ((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
    sampler=AllocVec(sizeof(*sampler),MEMF_PUBLIC|MEMF_CLEAR);port=CreateMsgPort();guard=CreateMsgPort();
    if(!sampler || !port || !guard){if(sampler)FreeVec(sampler);if(port)DeleteMsgPort(port);if(guard)DeleteMsgPort(guard);nd_client_close(&client);return 20;}
    sprintf(portname,"NovaDock.Net.%08lx.%08lx",(ULONG)FindTask(NULL),now_ticks());port->mp_Node.ln_Name=portname;AddPort(port);
    nc_defaults(&cfg);nc_defaults(&active);nc_defaults(&last_online);
    while(!quit){
        if(reload){
            previous=cfg;load_config(&cfg);
            if(strcmp(previous.stack,cfg.stack) || strcmp(previous.interface_name,cfg.interface_name) ||
               strcmp(previous.traffic.device,cfg.traffic.device) || previous.traffic.unit!=cfg.traffic.unit)
                nc_defaults(&last_online);
            close_sampler(sampler);poll=250;discovery_age=1500;reload=0;
            strcpy(traffic,"SAMPLING");meter=ND_NO_METER;a=b=ND_GRAPH_NO_SAMPLE;
        }
        if(notice>0)notice=notice>(int)elapsed ? notice-(int)elapsed:0;
        poll+=elapsed;if(busy)busy_age=now_ticks()-busy_started;
        if(discovery_age<1500)discovery_age+=elapsed;
        while((m=(struct NCMessage *)GetMsg(port))!=NULL){
            if(m->magic!=NC_MAGIC || m->msg.mn_Length!=sizeof(*m)){if(m->msg.mn_ReplyPort)ReplyMsg((struct Message *)m);continue;}
            if(m->kind==1){
                m->action=busy && !claimed && m->job==job ? action:-1;m->config=cfg;
                if(m->action==NC_PROBE && discovery_age<1500 && !strcmp(cfg.interface_name,"AUTO"))m->config=active;
                else if(m->action>=0)discovery_age=0;
                if(m->action>NC_PROBE && !strcmp(cfg.interface_name,"AUTO") && strcmp(last_online.interface_name,"AUTO")){
                    strcpy(m->config.interface_name,last_online.interface_name);
                    if(!m->config.traffic.device[0]){
                        strcpy(m->config.traffic.device,last_online.traffic.device);m->config.traffic.unit=last_online.traffic.unit;
                    }
                }
                if(m->action>=0)claimed=1;ReplyMsg((struct Message *)m);
            }else if(m->kind==2){
                if(m->job!=job || !busy || !claimed){FreeVec(m);continue;}
                if(strcmp(active.traffic.device,m->config.traffic.device) || active.traffic.unit!=m->config.traffic.unit)close_sampler(sampler);
                if(m->action==NC_OFFLINE){
                    if(m->ok){shutdown_pending=0;online=0;}
                    else if(m->result==5){
                        shutdown_pending=1;
                        if(!timed_out)(void)launch_worker(NULL,argv[0],NC_NOTICE_PENDING);
                    }else shutdown_pending=0;
                }else {if(online!=m->online)discovery_age=1500;online=m->online;if(m->online)last_online=m->config;}
                installed=m->installed;active=m->config;busy=0;claimed=0;poll=0;
                if(!m->ok && m->action && !(m->action==NC_OFFLINE && m->result==5)){
                    notice=250;strcpy(value,"ACTION FAILED");
                    if(!timed_out)(void)launch_worker(NULL,argv[0],m->result==NC_QUIESCE_FAILED ? NC_NOTICE_QUIESCE_FAILED:
                        m->result==NC_LOG_FAILED ? NC_NOTICE_LOG_FAILED:(ULONG)m->action);
                }
                timed_out=0;
                if(!online){close_sampler(sampler);meter=ND_NO_METER;a=b=ND_GRAPH_NO_SAMPLE;strcpy(traffic,"SAMPLING");}
                FreeVec(m);
            }
        }
        if(shutdown_pending && !stack_resident()){
            online=0;if(!busy){shutdown_pending=0;poll=0;discovery_age=1500;}
        }
        if(queued==NC_OFFLINE+1){
            close_sampler(sampler);
            if(sampler->pending){
                drain_age+=elapsed;
                if(drain_age>=750){queued=0;notice=250;strcpy(value,"DEVICE BUSY");(void)launch_worker(NULL,argv[0],NC_NOTICE_DEVICE_BUSY);}
            }
        }else drain_age=0;
        if(!busy && !shutdown_pending && (poll>=250 || queued) && !(queued==NC_OFFLINE+1 && sampler->pending)){
            action=queued ? queued-1:NC_PROBE;queued=0;
            if(action==NC_OFFLINE){close_sampler(sampler);shutdown_pending=1;shutdown_guard(guard,1);}
            ++job;if(!job)++job;busy=launch_worker(portname,argv[0],job);busy_started=now_ticks();busy_age=0;claimed=0;timed_out=0;
            if(!busy){shutdown_pending=0;notice=250;strcpy(value,"WORKER FAILED");poll=0;if(action)(void)launch_worker(NULL,argv[0],(ULONG)action);}
        }
        if(busy && !timed_out && busy_age>(claimed ? nc_job_deadline(&cfg,action):1500UL)){
            timed_out=1;
            if(!claimed){busy=0;shutdown_pending=0;notice=250;strcpy(value,"WORKER FAILED");poll=0;if(action)(void)launch_worker(NULL,argv[0],(ULONG)action);}
            else if(action)(void)launch_worker(NULL,argv[0],action==NC_OFFLINE ? NC_NOTICE_OFFLINE_TIMEOUT:NC_NOTICE_ONLINE_TIMEOUT);
            else if(queued)(void)launch_worker(NULL,argv[0],NC_NOTICE_BUSY);
            queued=0;
        }
        shutdown_guard(guard,shutdown_pending);
        if(online && !busy && !shutdown_pending && !sampler->opened && !sampler->pending){
            if(!open_sampler(sampler,&active.traffic))strcpy(traffic,active.traffic.device[0] ? "NO STATS":"SET DEVICE");
        }
        sampler->fresh=0;
        if(online && !shutdown_pending && sampler->opened && action!=NC_OFFLINE && queued!=NC_OFFLINE+1)sample(sampler,&active.traffic,elapsed,traffic,&meter,&a,&b);
        if(!notice){
            if(busy && action && !timed_out)strcpy(value,action==NC_ONLINE ? "STARTING":"STOPPING");
            else if(shutdown_pending)strcpy(value,"WAIT STACK");
            else if(busy && timed_out)strcpy(value,"NO REPLY");
            else if(!installed)strcpy(value,"NO ROADSHOW");
            else if(!online)strcpy(value,"OFFLINE");else strcpy(value,traffic);
        }
        event=online && sampler->fresh ? nd_client_update_graph(&client,"NETWORK",value,meter,a,b):
            online ? nd_client_update_graph_quiet(&client,"NETWORK",value,meter,a,b,elapsed,500):
            nd_client_update_quiet(&client,"NETWORK",value,ND_NO_METER,elapsed,500);
        if(!(event&ND_EVENT_QUIT))event|=nd_client_set_badge(&client,online ? "*":"");
        if(event&ND_EVENT_QUIT)quit=1;
        if(event&ND_EVENT_SETTINGS)reload=1;
        if(event&(ND_EVENT_NETWORK_ONLINE|ND_EVENT_NETWORK_OFFLINE)){
            if(shutdown_pending || (busy && (action!=NC_PROBE || timed_out)))
                (void)launch_worker(NULL,argv[0],NC_NOTICE_BUSY);
            else {queued=(event&ND_EVENT_NETWORK_OFFLINE) ? NC_OFFLINE+1:NC_ONLINE+1;reload=1;}
        }
        if(!quit)elapsed=nd_client_wait(&client,sampler->pending || busy ? 5:nd_client_delay_ticks(&client,(unsigned int)cfg.traffic.poll_ticks));
    }
    RemPort(port);
    while((m=(struct NCMessage *)GetMsg(port))!=NULL){if(m->kind==1){m->action=-1;ReplyMsg((struct Message *)m);}else FreeVec(m);}
    DeleteMsgPort(port);shutdown_guard(guard,0);DeleteMsgPort(guard);nd_client_close(&client);close_sampler(sampler);
    if(sampler->pending){WaitIO((struct IORequest *)sampler->io);sampler->pending=0;close_sampler(sampler);}
    if(sampler->io)DeleteIORequest((struct IORequest *)sampler->io);if(sampler->port)DeleteMsgPort(sampler->port);FreeVec(sampler);
    return 0;
}
