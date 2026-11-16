static int dock_command(const char *command)
{
    struct MsgPort *reply,*host;
    struct RexxMsg *msg;
    STRPTR marker=NULL;
    int replied=0,waited,ok=0;
    if (!RexxSysBase || !(reply=CreateMsgPort())) return 0;
    msg=CreateRexxMsg(reply,(STRPTR)"rexx",(STRPTR)"NOVADOCK");
    if (!msg) {DeleteMsgPort(reply);return 0;}
    if (!msg->rm_Node.mn_Node.ln_Name) {
        marker=CreateArgstring((STRPTR)"REXX",4);
        msg->rm_Node.mn_Node.ln_Name=(char *)marker;
    }
    msg->rm_Args[0]=CreateArgstring((STRPTR)command,(ULONG)strlen(command));
    msg->rm_Action=RXCOMM;
    if (msg->rm_Args[0]) {
        Forbid();
        host=FindPort((STRPTR)"NOVADOCK");
        if (host) PutMsg(host,(struct Message *)msg);
        Permit();
        if (host) {
            for (waited=0;waited<150 && !replied;waited+=2) {
                if (GetMsg(reply)) replied=1;
                else Delay(2);
            }
            if (!replied) {
                Forbid();
                if (GetMsg(reply)) replied=1;
                else {
                    reply->mp_Flags=PA_IGNORE;reply->mp_SigTask=NULL;
                    FreeSignal((LONG)reply->mp_SigBit);
                }
                Permit();
                if (!replied) return 0;
            }
            ok=msg->rm_Result1==0;
        }
        DeleteArgstring(msg->rm_Args[0]);
    }
    DeleteRexxMsg(msg);
    if (marker) DeleteArgstring(marker);
    DeleteMsgPort(reply);
    return ok;
}
/* Confirmation is handled by the resident before calling this function.
 * No action runs from option parsing, startup or periodic updates. */
static const char *run_action(int action)
{
 int closed,opened,resumed;
 if(action<0 || action>=ND_ACTION_COUNT)return "INVALID ACTION";
 switch(action){
 case ND_ACTION_REBOOT:ColdReboot();return "REBOOT FAILED";
 case ND_ACTION_WORKBENCH:
  if(!dock_command("SUSPEND"))return "DOCK NOT READY";
  closed=CloseWorkBench()!=0;opened=OpenWorkBench()!=0;
  resumed=dock_command("RESUME");
  if(!closed)return "Close other Workbench windows";
  if(!opened)return "Workbench could not reopen";
  return resumed ? NULL:"Dock is resuming";
 case ND_ACTION_DOCK:return dock_command("RESTART") ? NULL:"RESTART FAILED";
 case ND_ACTION_FRONT:WBenchToFront();return NULL;
 case ND_ACTION_BACK:WBenchToBack();return NULL;
 case ND_ACTION_SHELL:{
  BPTR input=Open("NIL:",MODE_OLDFILE),output=Open("NIL:",MODE_NEWFILE);LONG result;
  if(!input || !output){if(input)Close(input);if(output)Close(output);return "CANNOT OPEN SHELL";}
  result=SystemTags("NewShell",SYS_Asynch,TRUE,SYS_Input,input,SYS_Output,output,TAG_DONE);
  if(result==-1){Close(input);Close(output);return "CANNOT OPEN SHELL";}
  return NULL;
 }
 }
 return "INVALID ACTION";
}
