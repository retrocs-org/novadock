/* Five-day weather with cached data and isolated HTTPS downloader workers. */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_plugin_versions.h"
#include "nd_client.h"
#include "nd_option_runtime.h"
#include "nd_interaction.h"
#include "nd_panel.h"
#include "nd_weather.h"
#include "nd_weather_codes.h"
#include "nd_fetch_worker.h"
const char nd_version[]=ND_PLUGIN_VER_STRING(WEATHER,"Weather");
long __stack=32768;
struct WeatherSettings {char location[96],program[96],engine[8],units[8];unsigned int minutes;};
struct WeatherJob {struct MsgPort *port;char name[80],directory[160],command[1800];ULONG started;int transferred,phase;};
static struct NDWeather forecast;
static struct WeatherJob job;
static char response[ND_WEATHER_RESPONSE+2],latitude[20],longitude[20],status[80];
static int have,stale;
static ULONG received;
static void settings_read(const char *id,struct WeatherSettings *s)
{
    struct NDOptionString fields[5];char minutes[12];
    fields[0].key="LOCATION";fields[0].fallback="";fields[0].out=s->location;fields[0].limit=sizeof(s->location);
    fields[1].key="PROGRAM";fields[1].fallback="C:curl";fields[1].out=s->program;fields[1].limit=sizeof(s->program);
    fields[2].key="ENGINE";fields[2].fallback="CURL";fields[2].out=s->engine;fields[2].limit=sizeof(s->engine);
    fields[3].key="UNITS";fields[3].fallback="C";fields[3].out=s->units;fields[3].limit=sizeof(s->units);
    fields[4].key="REFRESH_MINUTES";fields[4].fallback="30";fields[4].out=minutes;fields[4].limit=sizeof(minutes);
    nd_plugin_option_strings(id,fields,5);s->minutes=(unsigned int)atoi(minutes);
    if(s->minutes<15 || s->minutes>360)s->minutes=30;
}
static int read_response(const char *path)
{
    FILE *f=fopen(path,"rb");size_t size;if(!f)return 0;
    size=fread(response,1,ND_WEATHER_RESPONSE+1,f);
    if(ferror(f) || size>ND_WEATHER_RESPONSE || memchr(response,0,size)){fclose(f);return 0;}
    fclose(f);response[size]=0;return size>0;
}
static void cache_load(unsigned long key)
{
    FILE *f=fopen("ENVARC:NovaDock/Weather.cache","rb");char header[96];unsigned long stored,stamp;size_t size;
    if(!f)return;
    if(!fgets(header,sizeof(header),f) || sscanf(header,"NDWEATHER1 %lx %lu",&stored,&stamp)!=2 || stored!=key){fclose(f);return;}
    size=fread(response,1,ND_WEATHER_RESPONSE+1,f);fclose(f);
    if(size>ND_WEATHER_RESPONSE || memchr(response,0,size))return;response[size]=0;
    if(nd_weather_parse(response,&forecast)){have=1;received=stamp;}
}
static void cache_save(unsigned long key)
{
    BPTR lock;char dir[160],path[180],old[180];FILE *f;int serial,ok,backed=0;
    lock=Lock("ENVARC:NovaDock",ACCESS_READ);if(lock)UnLock(lock);
    else{lock=CreateDir("ENVARC:NovaDock");if(!lock)return;UnLock(lock);}
    for(serial=0;serial<20;++serial){sprintf(dir,"ENVARC:NovaDock/WeatherSave-%08lx-%d",(ULONG)FindTask(NULL),serial);lock=CreateDir(dir);if(lock){UnLock(lock);break;}}
    if(serial==20)return;sprintf(path,"%s/new",dir);sprintf(old,"%s/old",dir);f=fopen(path,"wb");
    if(!f){DeleteFile(dir);return;}
    ok=fprintf(f,"NDWEATHER1 %08lx %lu\n",key,(unsigned long)received)>0 && fputs(response,f)>=0;
    if(fclose(f))ok=0;
    if(ok){lock=Lock("ENVARC:NovaDock/Weather.cache",ACCESS_READ);if(lock){UnLock(lock);backed=Rename("ENVARC:NovaDock/Weather.cache",old)!=0;if(!backed)ok=0;}}
    if(ok && !Rename(path,"ENVARC:NovaDock/Weather.cache"))ok=0;
    if(ok){if(backed)DeleteFile(old);}else if(backed && !Rename(old,"ENVARC:NovaDock/Weather.cache")){DeleteFile(path);return;}
    DeleteFile(path);DeleteFile(dir);
}
static void job_close(void)
{
    if(job.port)fetch_close_port(job.port);job.port=NULL;
    if(!job.transferred && job.directory[0])fetch_clean(job.directory);
    memset(&job,0,sizeof(job));
}
static int job_start(const char *program,const struct WeatherSettings *s,const char *url,int phase)
{
    BPTR lock,input,output;char executable[180],tool[370],quoted[1800],self[370],command[600],path[200];
    int serial;size_t n;LONG rc;
    if(job.port)return 0;
    lock=Lock((STRPTR)s->program,ACCESS_READ);if(!lock){strcpy(status,"Downloader not found; check Weather settings");return 0;}
    rc=NameFromLock(lock,executable,sizeof(executable));UnLock(lock);if(!rc)return 0;
    memset(&job,0,sizeof(job));job.phase=phase;job.started=weather_ticks();
    for(serial=0;serial<20;++serial){sprintf(job.directory,"T:NovaDockWeather-%08lx-%08lx-%d",(ULONG)FindTask(NULL),job.started,serial);lock=CreateDir(job.directory);if(lock){UnLock(lock);break;}}
    if(serial==20){job.directory[0]=0;return 0;}
    if(!nd_quote_path(tool,sizeof(tool),executable))goto fail;
    sprintf(path,"%s/response",job.directory);if(!nd_quote_path(quoted,sizeof(quoted),path))goto fail;
    sprintf(job.command,"%s %s %s",tool,!strcmp(s->engine,"CURL") ? "--disable --fail --silent --show-error --connect-timeout 90 --max-time 120 --output":"--no-verbose --timeout=30 --tries=1 --output-document",quoted);
    if(strcmp(s->engine,"CURL")){
        sprintf(path,"%s/fetch.log",job.directory);
        if(!nd_quote_path(quoted,sizeof(quoted),path))goto fail;
        strcat(job.command," --output-file ");strcat(job.command,quoted);
    }
    n=strlen(job.command);if(!nd_quote_path(quoted,sizeof(quoted),url) || n+strlen(quoted)+2>=sizeof(job.command))goto fail;
    strcat(job.command," ");strcat(job.command,quoted);
    job.port=CreateMsgPort();if(!job.port)goto fail;
    sprintf(job.name,"NovaDock.Weather.%08lx.%08lx.%d",(ULONG)FindTask(NULL),job.started,serial);job.port->mp_Node.ln_Name=job.name;
    Forbid();AddPort(job.port);Permit();
    if(!nd_quote_path(self,sizeof(self),program))goto fail;sprintf(command,"%s WORK %s",self,job.name);
    input=Open("NIL:",MODE_OLDFILE);output=Open("NIL:",MODE_NEWFILE);
    if(!input || !output){if(input)Close(input);if(output)Close(output);goto fail;}
    rc=SystemTags(command,SYS_Input,input,SYS_Output,output,SYS_Asynch,TRUE,NP_StackSize,32768UL,TAG_DONE);
    if(rc==-1){Close(input);Close(output);goto fail;}strcpy(status,"Updating forecast");return 1;
fail:job_close();return 0;
}
/* 0 waiting, 1 valid response ready, -1 failure. */
static int job_events(void)
{
    struct FetchMessage *m;int result=0;char path[200];
    if(!job.port)return 0;
    while((m=(struct FetchMessage *)GetMsg(job.port))!=NULL){
        if(m->magic!=FETCH_MAGIC || m->msg.mn_Length!=sizeof(*m)){
            if(m->msg.mn_ReplyPort)ReplyMsg((struct Message *)m);continue;
        }
        if(m->kind==1){
            if(job.transferred)m->kind=-1;
            else{strcpy(m->command,job.command);strcpy(m->directory,job.directory);job.transferred=1;}
            ReplyMsg((struct Message *)m);
        }else if(m->kind==2){
            sprintf(path,"%s/response",m->directory);
            result=m->result==0 && read_response(path) ? 1:-1;
            if(result<0){
                FILE *log;char detail[4097];size_t size=0;const char *problem;
                sprintf(path,"%s/fetch.log",m->directory);log=fopen(path,"rb");
                if(log){size=fread(detail,1,sizeof(detail)-1,log);fclose(log);}detail[size]=0;
                problem=nd_weather_download_problem(detail);
                if(problem){strncpy(status,problem,sizeof(status)-1);status[sizeof(status)-1]=0;}
                else if(m->result)sprintf(status,"HTTPS downloader failed (code %ld)",m->result);
                else strcpy(status,"Weather service returned an empty or oversized response");
            }
            fetch_clean(m->directory);FreeVec(m);break;
        }
    }
    if(!result && (ULONG)(weather_ticks()-job.started)>=7500UL){strcpy(status,"Weather download timed out");result=-1;}
    return result;
}
static void panel_text(const struct WeatherSettings *s,char text[8][160],char labels[8][20])
{
    unsigned int i;char low[24],high[24],now[24];int fahrenheit=!strcmp(s->units,"F");
    strcpy(labels[0],"Now");strcpy(labels[6],"Status");strcpy(labels[7],"Source");
    if(have){nd_weather_temperature(forecast.temperature,fahrenheit,now);sprintf(text[0],"%s  %s",now,nd_weather_condition(forecast.code));}
    else strcpy(text[0],s->location[0] ? "Forecast not available":"Set a location in Weather settings");
    for(i=0;i<ND_WEATHER_DAYS;++i){
        if(have && i<forecast.count){strcpy(labels[i+1],nd_weather_weekday(forecast.days[i].date));
            nd_weather_temperature(forecast.days[i].low,fahrenheit,low);nd_weather_temperature(forecast.days[i].high,fahrenheit,high);
            sprintf(text[i+1],"%s / %s  %s  Rain %d%%",low,high,nd_weather_condition(forecast.days[i].code),forecast.days[i].rain);
        }else{strcpy(labels[i+1],"--");strcpy(text[i+1],"--");}
    }
    sprintf(text[6],"%.19s%s%.75s",have && !stale ? forecast.updated:"",have && !stale ? "  ":"",status);
    strcpy(text[7],"Open-Meteo.com / CC BY 4.0");
}
static struct NDPanel *panel_open(const struct WeatherSettings *s)
{
    struct NDPanelField fields[8];char text[8][160],labels[8][20],title[128];unsigned int i;
    static const char *buttons[]={"Refresh","Close"};
    panel_text(s,text,labels);
    for(i=0;i<8;++i){fields[i].label=labels[i];fields[i].value=text[i];fields[i].limit=0;}
    sprintf(title,"%.85s - Weather (low / high)",s->location[0] ? s->location:"Weather");
    return nd_panel_open(title,fields,8,buttons,2);
}
static void panel_update(struct NDPanel *panel,const struct WeatherSettings *s)
{
    char text[8][160],labels[8][20];unsigned int i;panel_text(s,text,labels);
    for(i=0;i<8;++i){nd_panel_set_label(panel,i,labels[i]);nd_panel_set(panel,i,text[i]);}
}
int main(int argc,char **argv)
{
    struct NDClient client;struct NDPanel *panel=NULL;struct WeatherSettings settings,next;
    unsigned int event,elapsed=0;unsigned long key=0,until=0,last_attempt=0,interval,extra;
    char url[1000],caption[32],value[32],badge[2];int reread=1,pending=0,result,phase,action,changed;
    if(argc==3 && !strcmp(argv[1],"WORK"))return fetch_worker(argv[2]);
    if(argc<2)return 20;((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
    if(!nd_client_init(&client,argv[1]))return 20;memset(&settings,0,sizeof(settings));strcpy(status,"Set location in Weather settings");
    for(;;){
        if(reread){settings_read(argv[1],&next);changed=strcmp(next.location,settings.location) || strcmp(next.units,settings.units);
            settings=next;key=nd_weather_key(settings.location,!strcmp(settings.units,"F"));reread=0;
            if(changed){job_close();have=0;stale=0;latitude[0]=longitude[0]=0;cache_load(key);pending=settings.location[0]!=0;until=0;}
            interval=(unsigned long)settings.minutes*3000UL;
            if(have && (ULONG)(weather_ticks()-received)>=interval)stale=1;
            if(have && !stale && (ULONG)(weather_ticks()-received)<interval){until=interval-(ULONG)(weather_ticks()-received);pending=0;strcpy(status,"Cached forecast");}
            if(panel){nd_panel_close(panel);panel=panel_open(&settings);}
        }
        until=until>elapsed ? until-elapsed:0;
        if(settings.location[0] && !until && !job.port)pending=1;
        if(pending && !job.port){pending=0;last_attempt=weather_ticks();
            result=latitude[0] ? nd_weather_forecast_url(latitude,longitude,!strcmp(settings.units,"F"),url,sizeof(url)):nd_weather_search_url(settings.location,url,sizeof(url));
            if(!result || !job_start(argv[0],&settings,url,latitude[0] ? 2:1)){until=15000UL;stale=have;if(!status[0])strcpy(status,"Could not start update");}
            panel_update(panel,&settings);
        }
        phase=job.phase;result=job_events();
        if(result){job_close();
            if(result==1 && phase==1){
                if(nd_weather_geocode(response,latitude,longitude,sizeof(latitude))){pending=1;until=0;}
                else{strcpy(status,"Location not found");until=15000UL;stale=have;}
            }else if(result==1 && phase==2 && nd_weather_parse(response,&forecast)){
                have=1;stale=0;received=weather_ticks();until=(ULONG)settings.minutes*3000UL;
                strcpy(status,"Updated");cache_save(key);
            }else{if(result>0)strcpy(status,"Weather service returned an invalid forecast");stale=have;until=15000UL;}
            panel_update(panel,&settings);
        }
        if(have){sprintf(caption,"WX:%d %.24s",forecast.code,settings.location);nd_weather_temperature(forecast.temperature,!strcmp(settings.units,"F"),value);}
        else{sprintf(caption,"WX:-1 %.24s",settings.location);strcpy(value,settings.location[0] ? "NO DATA":"SET LOCATION");}
        event=nd_client_update_quiet(&client,caption,value,ND_NO_METER,elapsed,500U);
        badge[0]=stale ? '*':0;badge[1]=0;if(!(event&ND_EVENT_QUIT))event|=nd_client_set_badge(&client,badge);
        if(event&ND_EVENT_QUIT)break;if(event&ND_EVENT_SETTINGS)reread=1;
        if(event&ND_EVENT_CLICK){if(panel)nd_panel_raise(panel);else panel=panel_open(&settings);}
        action=nd_panel_event(panel);
        if(action==-1 || action==101){nd_panel_close(panel);panel=NULL;}
        else if(action==100 && !job.port && (ULONG)(weather_ticks()-last_attempt)>=3000UL){until=0;pending=settings.location[0]!=0;}
        extra=nd_panel_signals(panel)|(job.port ? 1UL<<job.port->mp_SigBit:0);
        elapsed=nd_client_wait_signals(&client,500U,extra);
    }
    job_close();nd_panel_close(panel);nd_client_close(&client);return 0;
}
