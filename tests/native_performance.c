/* Read-only metrics in a disposable guest, using the production CPU sampler. */
#include "qa_cpu_probe.h"
#include <dos/dostags.h>
#include "nd_control_client.h"
static ULONG ticks(void){struct DateStamp d;DateStamp(&d);return (ULONG)d.ds_Days*4320000UL+(ULONG)d.ds_Minute*3000UL+d.ds_Tick;}
static void mark(FILE *f,const char *phase,unsigned long startup)
{struct NDDiagnosticsSnapshot d;memset(&d,0,sizeof(d));nd_control_diagnostics(&d);fprintf(f,"%s,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n",phase,startup,AvailMem(MEMF_ANY),AvailMem(MEMF_CHIP),AvailMem(MEMF_LARGEST),d.wake_count,d.paint_count,d.update_count,d.watchdog_restarts);fflush(f);}
static int baseline(FILE *f)
{struct MsgPort *port;struct IORequest *io;struct NDIdleTask *task;struct EClockVal now;ULONG start,elapsed,idle;int i;
 port=CreateMsgPort();if(!port)return 0;io=CreateIORequest(port,sizeof(struct timerequest));if(!io){DeleteMsgPort(port);return 0;}if(OpenDevice(TIMERNAME,UNIT_ECLOCK,io,0)){DeleteIORequest(io);DeleteMsgPort(port);return 0;}TimerBase=io->io_Device;
 task=nd_cpu_start_idle_task();if(!task){CloseDevice(io);DeleteIORequest(io);DeleteMsgPort(port);return 0;}
 for(i=0;i<20;++i){nd_cpu_begin_sample(task);ReadEClock(&now);start=now.ev_lo;Delay(5);ReadEClock(&now);elapsed=now.ev_lo-start;idle=nd_cpu_end_sample();fprintf(f,"baseline,%d,%u,%lu,%lu\n",i,nd_cpu_used_percent(idle,elapsed),AvailMem(MEMF_ANY),AvailMem(MEMF_CHIP));fflush(f);Delay(45);}
 nd_cpu_stop_idle_task(task);CloseDevice(io);TimerBase=NULL;DeleteIORequest(io);DeleteMsgPort(port);return 1;}
static int launch(const char *config,ULONG *time)
{BPTR in,out;char cmd[240];ULONG start=ticks();int i;struct NDDiagnosticsSnapshot d;
 sprintf(cmd,"Copy QATests:%s ENV:NovaDock/novadock.cfg",config);if(SystemTags(cmd,TAG_DONE))return 0;
 in=Open("NIL:",MODE_OLDFILE);out=Open("NIL:",MODE_NEWFILE);if(!in||!out)return 0;
 if(SystemTags("QATests:Release04/NovaDock/NovaDock",SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE)<0){Close(in);Close(out);return 0;}
 for(i=0;i<150;++i){if(nd_control_diagnostics(&d)==ND_CONTROL_OK&&d.tile_count){*time=ticks()-start;Delay(250);return 1;}Delay(5);}return 0;}
static int measure(FILE *samples,FILE *phases,const char *phase,ULONG startup)
{int i;struct NDControlPluginHealthMessage h;mark(phases,phase,startup);
 for(i=0;i<20;++i){if(nd_control_plugin_health("CPU",&h)!=ND_CONTROL_OK||!h.online)return 0;fprintf(samples,"%s,%d,%d,%lu,%lu\n",phase,i,atoi(h.value),AvailMem(MEMF_ANY),AvailMem(MEMF_CHIP));fflush(samples);Delay(50);}mark(phases,"phase-end",0);return 1;}
int main(void)
{FILE *samples,*phases;ULONG startup;int result=10,i;BPTR marker;
 marker=Lock("QATests:ISOLATED-QA-ONLY",ACCESS_READ);if(!marker)return 10;UnLock(marker);
 Delay(250);SystemTags("QATests:Release04/NovaDock/NovaDock QUIT",TAG_DONE);Delay(100);
 SystemTags("MakeDir RAM:PerfEnv RAM:PerfEnv/NovaDock SYS:PerfEnvArchive",TAG_DONE);
 SystemTags("Assign ENV: RAM:PerfEnv",TAG_DONE);SystemTags("Assign ENVARC: SYS:PerfEnvArchive",TAG_DONE);
 SystemTags("Assign NovaDock: QATests:Release04/NovaDock",TAG_DONE);
 samples=fopen("QATests:performance.csv","w");phases=fopen("QATests:phases.csv","w");if(!samples||!phases)return 10;
 fputs("phase,sample,cpu_percent,free_bytes,chip_bytes\n",samples);fputs("phase,startup_ticks,free_bytes,chip_bytes,largest,wakes,paints,updates,restarts\n",phases);
 if(!baseline(samples))goto done;mark(phases,"baseline-end",0);
 if(!launch("stock.cfg",&startup)||!measure(samples,phases,"stock",startup))goto done;
 if(nd_control_send(ND_CONTROL_QUIT)!=ND_CONTROL_OK)goto done;Delay(150);mark(phases,"stock-stopped",0);
 if(!launch("populated.cfg",&startup)||!measure(samples,phases,"populated",startup))goto done;
 if(nd_control_send(ND_CONTROL_HIDE)!=ND_CONTROL_OK)goto done;Delay(100);
 if(!measure(samples,phases,"hidden",0))goto done;
 if(nd_control_send(ND_CONTROL_SHOW)!=ND_CONTROL_OK)goto done;
 for(i=0;i<10;++i){if(nd_control_reload()!=ND_CONTROL_OK)goto done;Delay(25);}mark(phases,"ten-identical-reloads",0);
 if(nd_control_send(ND_CONTROL_QUIT)!=ND_CONTROL_OK)goto done;Delay(250);mark(phases,"all-stopped",0);result=0;
 puts("PASS: baseline/stock/populated/hidden CPU and memory, startup timing and ten reloads recorded.");
done:if(result)puts("FAIL: performance measurement did not complete.");fclose(samples);fclose(phases);return result;}
