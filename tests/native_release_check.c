/* Execute real Installer operations only in an AmigaMac disposable guest. */
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <workbench/workbench.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/icon.h>
#include "nd_control.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
long __stack=65536;
struct Library *IconBase=NULL;
static unsigned int checks;static int user_level;
static char previous_version[32],current_version[32];
static char appdir[128],prefsdir[128],shared[128],parent[128],application[160],preferences[160];
static int versions(void)
{FILE *f=fopen("QATests:versions.txt","rb");int ok;if(!f)return 0;ok=fgets(previous_version,sizeof(previous_version),f)!=NULL&&fgets(current_version,sizeof(current_version),f)!=NULL;fclose(f);previous_version[strcspn(previous_version,"\r\n")]=0;current_version[strcspn(current_version,"\r\n")]=0;return ok&&previous_version[0]&&current_version[0];}
static int exists(const char *p){BPTR l=Lock((STRPTR)p,ACCESS_READ);if(!l)return 0;UnLock(l);return 1;}
static int command(const char *s){LONG rc=SystemTags((STRPTR)s,TAG_DONE);if(rc){printf("FAIL command %ld: %s\n",rc,s);return 0;}return 1;}
static int same(const char *a,const char *b)
{FILE *x=fopen(a,"rb"),*y=fopen(b,"rb");unsigned char p[1024],q[1024];size_t n,m;int ok=0;if(!x||!y)goto end;do{n=fread(p,1,sizeof(p),x);m=fread(q,1,sizeof(q),y);if(n!=m||memcmp(p,q,n))goto end;}while(n);ok=!ferror(x)&&!ferror(y);end:if(x)fclose(x);if(y)fclose(y);return ok;}
static int write_text(const char *path,const char *text){FILE *f=fopen(path,"wb");int ok;if(!f)return 0;ok=fputs(text,f)>=0;return fclose(f)==0&&ok;}
static int mkdir(const char *path){BPTR l;if(exists(path))return 1;l=CreateDir((STRPTR)path);if(!l){printf("FAIL fixture drawer: %s (%ld)\n",path,IoErr());return 0;}UnLock(l);return 1;}
static int layout(char *line)
{char *parts[4],*p=line,*slash;unsigned int n;for(n=0;n<4;++n){parts[n]=p;if(n<3){p=strchr(p,'\t');if(!p)return 0;*p++=0;}}parts[3][strcspn(parts[3],"\r\n")]=0;user_level=atoi(parts[0]);if(user_level<0||user_level>2)return 0;for(n=1;n<4;++n)if(!*parts[n]||strlen(parts[n])>=128||!strchr(parts[n],':')||strchr(parts[n],'"'))return 0;
 strcpy(appdir,parts[1]);strcpy(prefsdir,parts[2]);strcpy(shared,parts[3]);strcpy(parent,shared);slash=strrchr(parent,'/');if(slash)*slash=0;else{slash=strchr(parent,':');slash[1]=0;}sprintf(application,"%s/NovaDock",appdir);sprintf(preferences,"%s/NovaDockPrefs",prefsdir);
 return mkdir(appdir)&&mkdir(prefsdir)&&mkdir(parent);}
static int same_location(const char *a,const char *b)
{BPTR x=Lock((STRPTR)a,ACCESS_READ),y=Lock((STRPTR)b,ACCESS_READ);int ok=x&&y&&SameLock(x,y)==LOCK_SAME;if(x)UnLock(x);if(y)UnLock(y);if(!ok)printf("FAIL location: %s -> %s\n",a,b);return ok;}
static int startup_assign(const char *text,const char *name,const char *target)
{char literal[300],resolved[300],canonical[180];BPTR lock;int ok;sprintf(literal,"Assign >NIL: %s: \"%s\"",name,target);lock=Lock((STRPTR)target,ACCESS_READ);if(!lock)return 0;ok=NameFromLock(lock,canonical,sizeof(canonical));UnLock(lock);if(!ok)return 0;sprintf(resolved,"Assign >NIL: %s: \"%s\"",name,canonical);if(strstr(text,literal)||strstr(text,resolved))return 1;printf("FAIL persistent assignment: %s\n",name);return 0;}
static int startup_assigns(void)
{static char text[32768];FILE *f=fopen("S:User-Startup","rb");size_t n;int ok;if(!f)return 0;n=fread(text,1,sizeof(text)-1,f);text[n]=0;ok=!ferror(f)&&fgetc(f)==EOF;fclose(f);return ok&&startup_assign(text,"NovaDockApp",appdir)&&startup_assign(text,"NovaDockPrefs",prefsdir)&&startup_assign(text,"NovaDock",shared);}
static int locations(void)
{struct DiskObject *icon;int ok;if(!startup_assigns()||!same_location("NovaDockApp:",appdir)||!same_location("NovaDockPrefs:",prefsdir)||!same_location("NovaDock:",shared))return 0;icon=GetDiskObject("SYS:WBStartup/NovaDock");if(!icon)return 0;ok=icon->do_Type==WBPROJECT&&icon->do_DefaultTool&&same_location(icon->do_DefaultTool,application)&&FindToolType(icon->do_ToolTypes,"DONOTWAIT")!=NULL;FreeDiskObject(icon);if(ok)++checks;return ok;}
static int verify(const char *release)
{FILE *f;char list[100],line[700],*tab;unsigned int n=0;sprintf(list,"QATests:%s-%d-files.tsv",release,user_level);f=fopen(list,"rb");if(!f)return 0;while(fgets(line,sizeof(line),f)){tab=strchr(line,'\t');if(!tab){fclose(f);return 0;}*tab++=0;tab[strcspn(tab,"\r\n")]=0;if(!same(line,tab)){printf("FAIL installed bytes: %s\n",tab);fclose(f);return 0;}++n;}fclose(f);if(!locations())return 0;checks+=n;printf("PASS: %s installed bytes, assigns and WBStartup target (%u files).\n",release,n);return 1;}
static int installer(const char *release,const char *mode)
{char cmd[300],script[96];BPTR input,output;LONG rc;sprintf(script,"QATests:%s-%s-%d",release,mode,user_level);sprintf(cmd,"QATests:InstallerWelcome %s",script);input=Open("NIL:",MODE_OLDFILE);output=Open("NIL:",MODE_NEWFILE);if(!input||!output){if(input)Close(input);if(output)Close(output);return 0;}rc=SystemTags(cmd,SYS_Input,input,SYS_Output,output,SYS_Asynch,TRUE,NP_StackSize,32768UL,TAG_DONE);if(rc<0){Close(input);Close(output);return 0;}sprintf(cmd,"SYS:System/Installer %s APPNAME NovaDock MINUSER 0 DEFUSER 0 NOPRETEND NOLOG NOPRINT",script);return command(cmd);}
static int stop(void)
{int i;if(!command("QATests:Current/NovaDock/NovaDock QUIT"))return 0;for(i=0;i<50;++i){int live;Forbid();live=FindPort(ND_CONTROL_PORT)!=NULL;Permit();if(!live){Delay(50);return 1;}Delay(5);}return 0;}
static int unrelated(int create)
{const char *dirs[3];char path[180];unsigned int i;dirs[0]=appdir;dirs[1]=prefsdir;dirs[2]=parent;for(i=0;i<3;++i){sprintf(path,"%s/QAUnrelated",dirs[i]);if(create){if(!write_text(path,"Unrelated file: preserve on update/removal\n"))return 0;}else if(!exists(path))return 0;}return 1;}
static int fixtures(void)
{BPTR config;LONG at;const char *settings="\nTOOLTIPS=NO\nGAP=7\n";char p[240],cmd[500];unsigned int i;const char *dirs[]={"ENVARC:NovaDock/Notes","ENVARC:NovaDock/Notes/Deleted","SYS:QANotes"};
 config=Open("ENVARC:NovaDock/novadock.cfg",MODE_READWRITE);if(!config)return 0;at=Seek(config,0,OFFSET_END);if(at<0||Write(config,(APTR)settings,strlen(settings))!=(LONG)strlen(settings)){Close(config);return 0;}if(!Close(config))return 0;
 for(i=0;i<3;++i)if(!mkdir(dirs[i]))return 0;
 if(!command("Copy ENVARC:NovaDock/novadock.cfg QATests:preserved.cfg")||!write_text("ENVARC:NovaDock/Notes/qa.md","# Permanent QA note\nRetain after update/reinstall.\n")||!write_text("ENVARC:NovaDock/Notes/Deleted/qa.md","Deleted-note backup\n")||!write_text("SYS:QANotes/custom.md","Custom note: must survive all removals\n")||!unrelated(1))return 0;
 sprintf(p,"%s/Plugins/QAImported.assets",shared);if(!mkdir(p))return 0;strcat(p,"/fixture");if(!write_text(p,"Imported artwork\n"))return 0;
 sprintf(cmd,"Copy QATests:Previous/NovaDock/Plugins/Command \"%s/Plugins/QAImported\"",shared);if(!command(cmd))return 0;
 sprintf(p,"%s/Plugins/QAImported.ndplugin",shared);if(!write_text(p,"NOVADOCK_ID=QAIMPORTED\nNOVADOCK_PROGRAM=QAImported\nNOVADOCK_NAME=QA Imported\nNOVADOCK_VERSION=0.1\nNOVADOCK_ABI=1\nNOVADOCK_FEATURES=meter\n"))return 0;
 sprintf(p,"%s/Themes/QAImported",shared);if(!mkdir(p))return 0;strcat(p,"/theme.cfg");if(!write_text(p,"# Imported theme\n"))return 0;sprintf(p,"%s/Rexx/QAImported.rexx",shared);return write_text(p,"/* Imported QA script */\n");}
static int preserved(int components)
{char p[240];unsigned int i;const char *files[]={"Plugins/QAImported.ndplugin","Plugins/QAImported.assets/fixture","Themes/QAImported/theme.cfg","Rexx/QAImported.rexx"};
 if(!same("QATests:preserved.cfg","ENVARC:NovaDock/novadock.cfg")||!exists("ENVARC:NovaDock/Notes/qa.md")||!exists("ENVARC:NovaDock/Notes/Deleted/qa.md")||!exists("SYS:QANotes/custom.md")||!unrelated(0))return 0;
 if(components){sprintf(p,"%s/Plugins/QAImported",shared);if(!same("QATests:Previous/NovaDock/Plugins/Command",p))return 0;for(i=0;i<4;++i){sprintf(p,"%s/%s",shared,files[i]);if(!exists(p))return 0;}}++checks;return 1;}
static int removed(int keep)
{const char *paths[6];unsigned int i;paths[0]=application;paths[1]=preferences;paths[2]=shared;paths[3]="SYS:WBStartup/NovaDock";paths[4]="SYS:WBStartup/NovaDock.info";paths[5]="HELP:NovaDock.guide";for(i=0;i<6;++i)if(exists(paths[i])){printf("FAIL removal: %s\n",paths[i]);return 0;}if(keep)return preserved(0);return !exists("ENVARC:NovaDock")&&exists("SYS:QANotes/custom.md")&&unrelated(0);}
int main(void)
{const char *old[]={"SYS:Utilities/NovaDock","SYS:Utilities/NovaDock.info","SYS:NovaDock","SYS:Prefs/NovaDockPrefs","SYS:Prefs/NovaDockPrefs.info","SYS:WBStartup/NovaDock","SYS:WBStartup/NovaDock.info","HELP:NovaDock.guide","HELP:NovaDock.guide.info"};unsigned int i;char target[80],line[500];FILE *layouts=NULL;int result=10;
 if(!exists("QATests:ISOLATED-QA-ONLY")||!exists("AutoRun:Startup"))return 20;setbuf(stdout,NULL);if(!versions()||!stop())return 10;IconBase=OpenLibrary("icon.library",36);if(!IconBase)return 10;
 command("Assign NovaDock:");command("Assign NovaDockApp:");command("Assign NovaDockPrefs:");command("Assign NovaDockHelp:");
 for(i=0;i<sizeof(old)/sizeof(old[0]);++i)if(exists(old[i])){sprintf(target,"SYS:QAOriginal-%u",i);if(!Rename((STRPTR)old[i],target))goto done;}
 if(!command("MakeDir RAM:QAEnv SYS:QAEnvArchive")||!command("Assign ENV: RAM:QAEnv")||!command("Assign ENVARC: SYS:QAEnvArchive"))goto done;
 layouts=fopen("QATests:layouts.tsv","rb");if(!layouts)goto done;
 while(fgets(line,sizeof(line),layouts)){
 if(!layout(line))goto done;printf("MODE %d: app %s; prefs %s; shared %s\n",user_level,appdir,prefsdir,shared);
 if(!installer("Current","Install")||!stop()||!verify("Current"))goto done;puts("PASS: fresh installation and startup locations.");
 if(!fixtures()||!installer("Current","Keep")||!removed(1))goto done;puts("PASS: removal keeps settings, Notes and unrelated files.");
 if(!installer("Current","Install")||!stop()||!verify("Current")||!preserved(0))goto done;puts("PASS: reinstall restores settings and Notes.");
 if(!installer("Current","Remove")||!removed(0))goto done;puts("PASS: complete removal keeps external Notes and unrelated files.");
 if(!installer("Previous","Install")||!stop()||!verify("Previous")||!fixtures())goto done;printf("PASS: previous %s installed with preservation fixtures.\n",previous_version);
 if(!installer("Current","Install")||!stop()||!verify("Current")||!preserved(1))goto done;printf("PASS: %s -> %s preserves settings, Notes and imported assets at selected paths.\n",previous_version,current_version);
 if(!installer("Current","Install")||!stop()||!verify("Current")||!preserved(1))goto done;puts("PASS: repeated update preserves all fixtures.");
 if(!installer("Current","Remove")||!removed(0))goto done;
 }
 if(ferror(layouts)||!checks)goto done;printf("PASS: complete native Installer lifecycle (%u file/preservation checks).\n",checks);result=0;
 done:if(layouts)fclose(layouts);CloseLibrary(IconBase);if(result)puts("FAIL: Installer lifecycle stopped; inspect retained guest and transcript.");return result;}
