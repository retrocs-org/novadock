/* NovaDock Command-output tile: shows a line of a shell command's output.
 * The command runs in an asynchronous shell with its output in a T: file;
 * this process only polls for completion, so a slow or hanging command can
 * never block the plugin's heartbeat, the dock or the other plugins. */
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "nd_version.h"
#include "nd_client.h"
#include "nd_option_runtime.h"
#include "nd_command.h"

const char nd_version[]=ND_VER_STRING("Command");
/* Honoured by the vbcc and libnix startup code; harmless elsewhere. */
long __stack=16384;

#define ND_CMD_TICKS 50UL
#define POLL_TICKS 10U           /* while a command is running */
#define OUTPUT_LIMIT 2048

enum { CMD_IDLE, CMD_RUNNING, CMD_COLLECTING };

static char output[OUTPUT_LIMIT+1];

static void read_settings(const char *id,struct NDCommandConfig *cfg)
{
    nd_cmd_defaults(cfg);
    if (nd_plugin_options_effective(id,nd_cmd_apply_option,cfg)!=1) {
        /* A rejected value leaves the defaults for the rest; keep going. */
    }
}
static int file_exists(const char *path)
{
    BPTR lock=Lock((STRPTR)path,ACCESS_READ);
    if (!lock) return 0;
    UnLock(lock);
    return 1;
}
static int start_command(const char *command,const char *out_file,
                         const char *done_file)
{
    char script[ND_CMD_COMMAND_LEN+96];
    BPTR input,result_file;
    LONG rc;
    DeleteFile((STRPTR)done_file);
    if (!nd_cmd_build_script(command,done_file,script,sizeof(script))) return 0;
    input=Open((STRPTR)"NIL:",MODE_OLDFILE);
    result_file=Open((STRPTR)out_file,MODE_NEWFILE);
    if (!input || !result_file) {
        if (input) Close(input);
        if (result_file) Close(result_file);
        return 0;
    }
    rc=SystemTags((STRPTR)script,SYS_Input,input,SYS_Output,result_file,
                  SYS_Asynch,TRUE,NP_StackSize,16384,TAG_DONE);
    if (rc==-1) {Close(input);Close(result_file);return 0;}
    return 1;
}
static void run_click_command(const char *command)
{
    char script[ND_CMD_COMMAND_LEN+32];
    BPTR input,out;
    if (!command[0] || strlen(command)+32>sizeof(script)) return;
    sprintf(script,"Stack 16384\n%s",command);
    input=Open((STRPTR)"NIL:",MODE_OLDFILE);
    out=Open((STRPTR)"NIL:",MODE_NEWFILE);
    if (!input || !out) {
        if (input) Close(input);
        if (out) Close(out);
        return;
    }
    if (SystemTags((STRPTR)script,SYS_Input,input,SYS_Output,out,
                   SYS_Asynch,TRUE,NP_StackSize,16384,TAG_DONE)==-1) {
        Close(input);Close(out);
    }
}
/* Read a small file; returns 0 while the shell still holds it open. */
static int read_file(const char *path,char *buffer,unsigned int size)
{
    BPTR file=Open((STRPTR)path,MODE_OLDFILE);
    LONG got;
    if (!file) return 0;
    got=Read(file,buffer,(LONG)size-1);
    Close(file);
    buffer[got>0 ? got : 0]='\0';
    return 1;
}
int main(int argc,char **argv)
{
    struct NDClient client;
    struct NDCommandConfig cfg;
    char out_file[64],done_file[64],value[ND_CMD_TEXT_LEN],rc_text[16];
    const char *caption;
    unsigned int event,meter=ND_NO_METER,elapsed=0,wait;
    unsigned long until_next=0,running_ticks=0;
    int state=CMD_IDLE;
    long rc;
    /* Badge: "!" while the last run failed (RC 10 or more, could not start
     * or timed out); cleared by the next successful run. */
    const char *badge="";
    if (argc<2 || !nd_cmd_file_names(argv[1],out_file,done_file,sizeof(out_file)) ||
        !nd_client_init(&client,argv[1])) return 20;
    read_settings(argv[1],&cfg);
    DeleteFile((STRPTR)out_file);DeleteFile((STRPTR)done_file);
    strcpy(value,"STARTING");
    for (;;) {
        if (state==CMD_IDLE && until_next==0) {
            if (start_command(cfg.command,out_file,done_file)) {
                state=CMD_RUNNING;running_ticks=0;
            } else {
                strcpy(value,"START FAILED");
                badge="!";
                until_next=(unsigned long)cfg.interval*ND_CMD_TICKS;
            }
        }
        if (state==CMD_RUNNING && file_exists(done_file)) state=CMD_COLLECTING;
        if (state==CMD_COLLECTING &&
            read_file(done_file,rc_text,sizeof(rc_text)) &&
            read_file(out_file,output,sizeof(output))) {
            rc=nd_cmd_parse_rc(rc_text);
            badge=rc>=10 ? "!" : "";
            if (!nd_cmd_extract(output,cfg.line,cfg.meter,value,sizeof(value),
                                &meter)) {
                meter=ND_NO_METER;
                if (rc>0) sprintf(value,"RC %ld",rc);
                else strcpy(value,"NO OUTPUT");
            }
            DeleteFile((STRPTR)out_file);DeleteFile((STRPTR)done_file);
            state=CMD_IDLE;
            until_next=(unsigned long)cfg.interval*ND_CMD_TICKS;
        }
        if (state!=CMD_IDLE &&
            running_ticks>(unsigned long)cfg.timeout*ND_CMD_TICKS) {
            /* AmigaDOS offers no safe way to kill the shell; report it and
             * start the next run only once this one has finished. */
            strcpy(value,"TIMEOUT");
            meter=ND_NO_METER;
            badge="!";
        }
        caption=cfg.caption[0] ? cfg.caption : "";
        event=nd_client_update_quiet(&client,caption,value,meter,elapsed,500U);
        if (!(event&ND_EVENT_QUIT)) event|=nd_client_set_badge(&client,badge);
        if (event&ND_EVENT_QUIT) break;
        if (event&ND_EVENT_SETTINGS) {
            read_settings(argv[1],&cfg);
            if (state==CMD_IDLE) until_next=0;
        }
        if (event&ND_EVENT_CLICK) {
            if (cfg.click==ND_CMD_CLICK_RERUN && state==CMD_IDLE) until_next=0;
            else if (cfg.click==ND_CMD_CLICK_COMMAND)
                run_click_command(cfg.click_command);
        }
        if (state==CMD_IDLE && until_next==0) {elapsed=0;continue;}
        if (state!=CMD_IDLE) wait=POLL_TICKS;
        else {
            wait=until_next>250UL ? 250U : (unsigned int)until_next;
            wait=nd_client_delay_ticks(&client,wait);
            if ((unsigned long)wait>until_next && until_next>0) wait=(unsigned int)until_next;
        }
        elapsed=nd_client_wait(&client,wait);
        if (state!=CMD_IDLE) running_ticks+=elapsed;
        else until_next=until_next>elapsed ? until_next-elapsed : 0;
    }
    nd_client_close(&client);
    return 0;
}
