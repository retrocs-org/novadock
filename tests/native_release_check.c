/* Execute real Installer actions in an AmigaMac disposable guest only. */
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include "nd_control.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
long __stack=65536;
static unsigned int checks;
static int exists(const char *p){BPTR l=Lock((STRPTR)p,ACCESS_READ);if(!l)return 0;UnLock(l);return 1;}
static int command(const char *s){LONG rc=SystemTags((STRPTR)s,TAG_DONE);if(rc){printf("FAIL command %ld: %s\n",rc,s);return 0;}return 1;}
static int same(const char *a,const char *b)
{FILE *x=fopen(a,"rb"),*y=fopen(b,"rb");unsigned char p[1024],q[1024];size_t n,m;int ok=0;if(!x||!y)goto end;do{n=fread(p,1,sizeof(p),x);m=fread(q,1,sizeof(q),y);if(n!=m||memcmp(p,q,n))goto end;}while(n);ok=!ferror(x)&&!ferror(y);end:if(x)fclose(x);if(y)fclose(y);return ok;}
static int write_text(const char *path,const char *text){FILE *f=fopen(path,"wb");int ok;if(!f)return 0;ok=fputs(text,f)>=0;return fclose(f)==0&&ok;}
static int verify(const char *release)
{FILE *f;char list[100],line[700],*tab;unsigned int n=0;sprintf(list,"QATests:%s-files.tsv",release);f=fopen(list,"rb");if(!f)return 0;while(fgets(line,sizeof(line),f)){tab=strchr(line,'\t');if(!tab){fclose(f);return 0;}*tab++=0;tab[strcspn(tab,"\r\n")]=0;if(!same(line,tab)){printf("FAIL installed bytes: %s\n",tab);fclose(f);return 0;}++n;}fclose(f);checks+=n;printf("PASS: %s installed files match package bytes (%u checks).\n",release,n);return 1;}
static int installer(const char *release,const char *mode)
{char cmd[300];sprintf(cmd,"SYS:System/Installer QATests:%s-%s APPNAME NovaDock MINUSER 0 DEFUSER 0 NOPRETEND NOLOG NOPRINT",release,mode);return command(cmd);}
static int stop(void){int i;if(!command("QATests:Release04/NovaDock/NovaDock QUIT"))return 0;for(i=0;i<50;++i){int live;Forbid();live=FindPort(ND_CONTROL_PORT)!=NULL;Permit();if(!live){Delay(50);return 1;}Delay(5);}return 0;}
static int fixtures(void)
{
 const char *dirs[]={"ENVARC:NovaDock/Notes","ENVARC:NovaDock/Notes/Deleted","SYS:QANotes","SYS:NovaDock/Plugins/QAImported.assets","SYS:NovaDock/Themes/QAImported","SYS:NovaDock/Rexx"};unsigned int i;BPTR l;
 for(i=0;i<sizeof(dirs)/sizeof(dirs[0]);++i)if(!exists(dirs[i])){l=CreateDir((STRPTR)dirs[i]);if(!l)return 0;UnLock(l);}
 return command("Copy ENVARC:NovaDock/novadock.cfg QATests:preserved.cfg")&&
 write_text("ENVARC:NovaDock/Notes/qa.md","# Permanent QA note\nRetain after update/reinstall.\n")&&write_text("ENVARC:NovaDock/Notes/Deleted/qa.md","Deleted-note backup\n")&&
 write_text("SYS:QANotes/custom.md","Custom note: must survive all removals\n")&&write_text("SYS:NovaDock/Plugins/QAImported.assets/fixture","Imported artwork\n")&&
 command("Copy QATests:Release03/NovaDock/Plugins/Command SYS:NovaDock/Plugins/QAImported")&&
 write_text("SYS:NovaDock/Plugins/QAImported.ndplugin","NOVADOCK_ID=QAIMPORTED\nNOVADOCK_PROGRAM=QAImported\nNOVADOCK_NAME=QA Imported\nNOVADOCK_VERSION=0.1\nNOVADOCK_ABI=1\nNOVADOCK_FEATURES=meter\n")&&
 write_text("SYS:NovaDock/Themes/QAImported/theme.cfg","# Imported theme preservation fixture\n")&&write_text("SYS:NovaDock/Rexx/QAImported.rexx","/* Imported QA script */\n")&&write_text("SYS:Utilities/QAUnrelated","Unrelated application file\n")&&write_text("SYS:Prefs/QAUnrelated","Unrelated preference file\n");
}
static int preserved(int components)
{
 if(!same("QATests:preserved.cfg","ENVARC:NovaDock/novadock.cfg")||!exists("ENVARC:NovaDock/Notes/qa.md")||!exists("ENVARC:NovaDock/Notes/Deleted/qa.md")||!exists("SYS:QANotes/custom.md")||!exists("SYS:Utilities/QAUnrelated")||!exists("SYS:Prefs/QAUnrelated"))return 0;
 if(components&&(!same("QATests:Release03/NovaDock/Plugins/Command","SYS:NovaDock/Plugins/QAImported")||!exists("SYS:NovaDock/Plugins/QAImported.ndplugin")||!exists("SYS:NovaDock/Plugins/QAImported.assets/fixture")||!exists("SYS:NovaDock/Themes/QAImported/theme.cfg")||!exists("SYS:NovaDock/Rexx/QAImported.rexx")))return 0;
 ++checks;return 1;
}
static int removed(int keep)
{const char *paths[]={"SYS:Utilities/NovaDock","SYS:Prefs/NovaDockPrefs","SYS:NovaDock","SYS:WBStartup/NovaDock","SYS:WBStartup/NovaDock.info","HELP:NovaDock.guide"};unsigned int i;for(i=0;i<sizeof(paths)/sizeof(paths[0]);++i)if(exists(paths[i])){printf("FAIL removal: %s\n",paths[i]);return 0;}if(keep)return preserved(0);return !exists("ENVARC:NovaDock")&&exists("SYS:QANotes/custom.md")&&exists("SYS:Utilities/QAUnrelated")&&exists("SYS:Prefs/QAUnrelated");}
int main(void)
{
 const char *old[]={"SYS:Utilities/NovaDock","SYS:Utilities/NovaDock.info","SYS:NovaDock","SYS:Prefs/NovaDockPrefs","SYS:Prefs/NovaDockPrefs.info","SYS:WBStartup/NovaDock","SYS:WBStartup/NovaDock.info","HELP:NovaDock.guide","HELP:NovaDock.guide.info"};unsigned int i;char target[80];
 if(!exists("QATests:ISOLATED-QA-ONLY")||!exists("AutoRun:Startup")){puts("FAIL: isolated-test markers required.");return 10;}
 if(!stop())return 10;
 /* The bundled OS may have an older dock. Preserve it inside this disposable disk. */
 command("Assign NovaDock:");command("Assign NovaDockApp:");command("Assign NovaDockPrefs:");command("Assign NovaDockHelp:");
 for(i=0;i<sizeof(old)/sizeof(old[0]);++i)if(exists(old[i])){sprintf(target,"SYS:QAOriginal-%u",i);if(!Rename((STRPTR)old[i],target)){printf("FAIL archive: %s\n",old[i]);return 10;}}
 if(!command("MakeDir RAM:QAEnv SYS:QAEnvArchive")||!command("Assign ENV: RAM:QAEnv")||!command("Assign ENVARC: SYS:QAEnvArchive"))return 10;
 if(!installer("Release04","Install")||!stop()||!verify("Release04"))goto fail;
 puts("PASS: fresh 0.4 installation.");
 if(!fixtures()||!installer("Release04","Keep")||!removed(1))goto fail;
 puts("PASS: uninstall keeps settings, Notes and unrelated files.");
 if(!installer("Release04","Install")||!stop()||!verify("Release04")||!preserved(0))goto fail;
 puts("PASS: reinstallation restores retained settings and Notes.");
 if(!installer("Release04","Remove")||!removed(0))goto fail;
 puts("PASS: full removal deletes default settings/Notes and keeps custom Notes.");
 if(!installer("Release03","Install")||!stop()||!verify("Release03")||!fixtures())goto fail;
 puts("PASS: published 0.3 installed with preservation fixtures.");
 if(!installer("Release04","Install")||!stop()||!verify("Release04")||!preserved(1))goto fail;
 puts("PASS: 0.3 -> 0.4 preserves settings, Notes, imported plugin/artwork/theme/script.");
 if(!installer("Release04","Install")||!stop()||!verify("Release04")||!preserved(1))goto fail;
 puts("PASS: repeated 0.4 update remains safe.");
 if(!installer("Release04","Remove")||!removed(0))goto fail;
 printf("PASS: complete native Installer lifecycle (%u file/preservation checks).\n",checks);return 0;
fail:puts("FAIL: Installer lifecycle stopped; inspect retained guest and transcript.");return 10;
}
