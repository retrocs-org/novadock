/* Workbench entry point for Install-NovaDock. Run the system Installer as
 * a synchronous CLI tool with an explicit stack and package directory. */
#ifndef ND_INSTALLER_TEST
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <workbench/startup.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#endif
#include <string.h>
#include <stdio.h>
#ifndef ND_INSTALLER_TEST
#include "nd_version.h"
const char nd_version[]=ND_VER_STRING("StartInstaller");
long __stack=32768;
struct IntuitionBase *IntuitionBase=NULL;
static void problem(const char *text)
{
    struct EasyStruct request;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",37);
    if (!IntuitionBase) return;
    memset(&request,0,sizeof(request));
    request.es_StructSize=sizeof(request);
    request.es_Title=(STRPTR)"NovaDock Installer";
    request.es_TextFormat=(STRPTR)text;
    request.es_GadgetFormat=(STRPTR)"OK";
    EasyRequestArgs(NULL,&request,NULL,NULL);
    CloseLibrary((struct Library *)IntuitionBase);
    IntuitionBase=NULL;
}
 #endif
/* AmigaDOS quotes use '*' as the escape character, including inside quotes. */
static int quoted(char *out,int size,const char *in)
{
    int at=0;
    if (size<3) return 0;
    out[at++]='"';
    while (*in) {
        unsigned char c=(unsigned char)*in++;
        if (c<32 || at+4>=size) return 0;
        if (c=='*' || c=='"') out[at++]='*';
        out[at++]=(char)c;
    }
    out[at++]='"';out[at]=0;
    return 1;
}
/* Workbench may pass a drawer argument (or an empty project name).
 * Always use the fixed script beside this executable, never that argument. */
static int script_in_drawer(char *out,int size,const char *drawer)
{
    int n=(int)strlen(drawer);
    const char *name="Install-NovaDock";
    int separator=n && drawer[n-1]!=':' && drawer[n-1]!='/';
    if (!n || n+separator+(int)strlen(name)+1>size) return 0;
    strcpy(out,drawer);
    if (separator) strcat(out,"/");
    strcat(out,name);
    return 1;
}
#ifndef ND_INSTALLER_TEST
static int exists(const char *path)
{
    BPTR lock=Lock((STRPTR)path,ACCESS_READ);
    if (!lock) return 0;
    UnLock(lock);return 1;
}
int main(int argc,char **argv)
{
    char script[512],directory[512],qscript[1030],command[1300],error[650];
    const char *installer;
    BPTR lock,input,output,childdir;
    struct FileInfoBlock info;
    LONG result;
    int ok=0;
    (void)argc; (void)argv;
    script[0]=directory[0]=0;
    /* GetProgramDir remains valid after uninstall removes NovaDock assigns.
     * It also avoids relying on a cached Workbench project/drawer argument. */
    lock=GetProgramDir();
    if (lock) ok=NameFromLock(lock,directory,sizeof(directory));
    if (!ok || !script_in_drawer(script,sizeof(script),directory)) {
        problem("Cannot locate the installer package directory.");return 20;
    }
    lock=Lock((STRPTR)script,ACCESS_READ);
    ok=lock && Examine(lock,&info) && info.fib_DirEntryType<0 && info.fib_Size>0;
    if (lock) UnLock(lock);
    if (!ok) {
        sprintf(error,"Cannot open the installer script:\n%s",script);
        problem(error);return 20;
    }
    /* AmigaOS 3.2 keeps its supported Installer in SYS:System. C: may
     * contain an older third-party copy installed years earlier. */
    if (exists("SYS:System/Installer")) installer="SYS:System/Installer";
    else if (exists("C:Installer")) installer="C:Installer";
    else {problem("Installer was not found in SYS:System or C:.");return 20;}
    if (!quoted(qscript,sizeof(qscript),script)) return 20;
    sprintf(command,"\"%s\" %s APPNAME NovaDock MINUSER NOVICE DEFUSER NOVICE",
            installer,qscript);
    input=Open("NIL:",MODE_OLDFILE);output=Open("NIL:",MODE_NEWFILE);
    if (!input || !output) {
        if (input) Close(input);
        if (output) Close(output);
        problem("Cannot open the installer command streams.");return 20;
    }
    childdir=DupLock(GetProgramDir());
    if (!childdir) {Close(input);Close(output);problem("Cannot open the installer package directory.");return 20;}
    /* SystemTags receives one command, with the package as its working drawer.
     * Do not feed Stack/CD commands through the Installer's script argument. */
    result=SystemTags((STRPTR)command,SYS_Input,input,SYS_Output,output,
                      SYS_Asynch,FALSE,NP_StackSize,65536UL,NP_CurrentDir,childdir,TAG_DONE);
    Close(input);Close(output);
    if (result<0) {UnLock(childdir);problem("The system Installer could not be started.");return 20;}
    return (int)result;
}

#endif
