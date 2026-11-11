#include "nd_netcontrol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
unsigned long nc_job_deadline(const struct NCConfig *config,int action)
{return action==NC_ONLINE ? ((unsigned long)config->timeout+15UL)*50UL:750UL;}
void nc_device_status_line(struct NCDeviceStatus *status,const char *line)
{
    char text[384],*key,*value,*end,*tail;unsigned long unit;
    if(!status || !line)return;
    strncpy(text,line,sizeof(text)-1);text[sizeof(text)-1]=0;
    key=text;while(isspace((unsigned char)*key))++key;
    value=strchr(key,'=');if(!value)return;*value++=0;
    end=key+strlen(key);while(end>key && isspace((unsigned char)end[-1]))*--end=0;
    while(isspace((unsigned char)*value))++value;
    end=value+strlen(value);while(end>value && isspace((unsigned char)end[-1]))*--end=0;
    if(!strcmp(key,"Device name")){
        status->have_device=0;
        if(*value && strlen(value)<sizeof(status->device)){
            for(end=value;*end;++end)if((unsigned char)*end<32 || (unsigned char)*end==127)return;
            strcpy(status->device,value);status->have_device=1;
        }
    }else if(!strcmp(key,"Device unit") || !strcmp(key,"Device unit number")){
        status->have_unit=0;unit=strtoul(value,&tail,10);
        if(isdigit((unsigned char)*value) && !*tail && unit<=255){status->unit=unit;status->have_unit=1;}
    }
}
int nc_device_status_apply(struct NDNetConfig *traffic,const struct NCDeviceStatus *status)
{
    if(!traffic || !status || traffic->device[0] || !status->have_device || !status->have_unit)return 0;
    strcpy(traffic->device,status->device);traffic->unit=status->unit;return 1;
}
/* ShowNetStatus may wrap the counters/status onto the following line.
 * Recognize data rows by MTU and numeric IPv4 address, never by headings. */
void nc_status_line(struct NCInterfaces *interfaces,const char *line)
{
    char text[512],*tokens[24],*p,*end;int n=0,i,up=0;unsigned long mtu;
    unsigned int a,b,c,d;char extra;
    if(!interfaces || !line)return;
    strncpy(text,line,sizeof(text)-1);text[sizeof(text)-1]=0;p=text;
    while(*p && n<24){
        while(isspace((unsigned char)*p))++p;if(!*p)break;
        tokens[n++]=p;while(*p && !isspace((unsigned char)*p))++p;
        if(*p)*p++=0;
    }
    if(!n)return;
    for(i=0;i<n;++i)if(!strcmp(tokens[i],"Up"))up=1;
    if(n>=4){
        mtu=strtoul(tokens[1],&end,10);
        if(isdigit((unsigned char)*tokens[1]) && !*end && mtu>0 && mtu<=65535 &&
           sscanf(tokens[3],"%u.%u.%u.%u%c",&a,&b,&c,&d,&extra)==4 &&
           a<=255 && b<=255 && c<=255 && d<=255){
            interfaces->pending[0]=0;
            if(a!=127 && a!=0 && strcmp(tokens[2],"Loopback") && rd_valid_interface(tokens[0]))
                strcpy(interfaces->pending,tokens[0]);
        }
    }
    if(up && interfaces->pending[0]){
        if(!interfaces->active[0] || strcmp(interfaces->pending,interfaces->active)<0)
            strcpy(interfaces->active,interfaces->pending);
        interfaces->pending[0]=0;
    }
    for(i=0;i<n;++i)if(!strcmp(tokens[i],"Down"))interfaces->pending[0]=0;
}
void nc_sampling_mode(struct NCConfig *c,int override_enabled)
{
    if(c && !override_enabled){c->traffic.device[0]=0;c->traffic.unit=0;}
}
void nc_defaults(struct NCConfig *c)
{memset(c,0,sizeof(*c));nd_net_defaults(&c->traffic);strcpy(c->stack,"AUTO");strcpy(c->interface_name,"AUTO");c->timeout=60;}
int nc_option(void *context,const char *key,const char *value)
{
    struct NCConfig *c=context;char *end;unsigned long n;
    if(!strcmp(key,"VIEW"))return !strcmp(value,"METER") || !strcmp(value,"GRAPH");
    if(!strcmp(key,"SAMPLING_OVERRIDE"))return !strcmp(value,"YES") || !strcmp(value,"NO");
    if(!strcmp(key,"DEBUG_LOG"))return !strcmp(value,"YES") || !strcmp(value,"NO");
    if(!strcmp(key,"STACK")){
        if(strcmp(value,"AUTO") && strcmp(value,"ROADSHOW"))return 0;strcpy(c->stack,value);return 1;
    }
    if(!strcmp(key,"INTERFACE")){
        if(!rd_valid_interface(value))return 0;strcpy(c->interface_name,value);return 1;
    }
    if(!strcmp(key,"DHCP_TIMEOUT")){
        n=strtoul(value,&end,10);if(!*value || *end || n<10 || n>300)return 0;c->timeout=(unsigned int)n;return 1;
    }
    return nd_net_apply_option(&c->traffic,key,value);
}
/* Roadshow interface files use case-insensitive keys and quoted or plain
 * device names. Read device metadata only; no interface file is changed. */
int nc_interface_file(struct NDNetConfig *traffic,const char *path)
{
    FILE *f;char line[384],*key,*val,*end;unsigned long unit=0;char device[ND_NET_DEVICE_LEN]="";
    f=fopen(path,"r");if(!f)return 0;
    while(fgets(line,sizeof(line),f)){
        key=line;while(isspace((unsigned char)*key))++key;
        if(!*key || *key=='#' || *key==';')continue;
        val=strchr(key,'=');if(!val)continue;*val++=0;
        end=key+strlen(key);while(end>key && isspace((unsigned char)end[-1]))*--end=0;
        for(end=key;*end;++end)*end=(char)toupper((unsigned char)*end);
        while(isspace((unsigned char)*val))++val;
        if(*val=='"'){++val;end=strchr(val,'"');if(!end){fclose(f);return 0;}*end=0;}
        else {end=val;while(*end && !isspace((unsigned char)*end) && *end!='#' && *end!=';')++end;*end=0;}
        if(!strcmp(key,"DEVICE")){
            if(!*val || strlen(val)>=sizeof(device)){fclose(f);return 0;}strcpy(device,val);
        }else if(!strcmp(key,"UNIT")){
            char *tail;unit=strtoul(val,&tail,10);
            if(!*val || *tail || *val=='-' || unit>255){fclose(f);return 0;}
        }
    }
    fclose(f);if(!device[0])return 0;
    if(!traffic->device[0]){strcpy(traffic->device,device);traffic->unit=unit;}
    return 1;
}
int nc_command(const struct NCConfig *c,int action,char *out,unsigned int size)
{
    struct RDRoadshowConfig rd;
    if(!c || action<NC_PROBE || action>NC_OFFLINE || (strcmp(c->stack,"AUTO") && strcmp(c->stack,"ROADSHOW")))return 0;
    rd_config_defaults(&rd);rd.allow_shutdown=1;rd.dhcp_timeout=c->timeout;
    if(strlen(c->interface_name)>RD_INTERFACE_MAX)return 0;
    strcpy(rd.interface_name,c->interface_name);
    if(action==NC_ONLINE && !strcmp(c->interface_name,"AUTO"))return 0;
    return rd_command(&rd,action==NC_ONLINE ? RD_CMD_CONNECT:action==NC_OFFLINE ? RD_CMD_SHUTDOWN:RD_CMD_STATUS,out,size);
}

/* Quiesce the selected device before the stack-wide shutdown. Some SANA-II
 * drivers leave Roadshow waiting if shutdown starts with the device online.
 * Do not begin irreversible shutdown when the preparatory step failed. */
int nc_quiesce_command(const struct NCConfig *c,char *out,unsigned int size)
{
    char command[128];
    if(!c || !out || !rd_valid_interface(c->interface_name) ||
       !strcmp(c->interface_name,"AUTO") ||
       (strcmp(c->stack,"AUTO") && strcmp(c->stack,"ROADSHOW")))return 0;
    sprintf(command,"C:ConfigureNetInterface %s OFFLINE QUIET",c->interface_name);
    if(strlen(command)>=size)return 0;
    strcpy(out,command);return 1;
}
int nc_shutdown(const struct NCConfig *c,NCExecute execute,void *context)
{
    char command[256];
    if(!execute || !nc_quiesce_command(c,command,sizeof(command)))return NC_QUIESCE_FAILED;
    if(execute(context,command)!=0)return NC_QUIESCE_FAILED;
    if(!nc_command(c,NC_OFFLINE,command,sizeof(command)))return 20;
    return execute(context,command);
}
