/* Thirty real minutes of repeated native activity in a disposable guest. */
#include "native_qa_ui.h"
#include "nd_build_id.h"
long __stack=65536;
static void counts(ULONG *ports,ULONG *tasks){struct Node *n;*ports=*tasks=0;Forbid();for(n=SysBase->PortList.lh_Head;n->ln_Succ;n=n->ln_Succ)++*ports;for(n=SysBase->TaskReady.lh_Head;n->ln_Succ;n=n->ln_Succ)++*tasks;for(n=SysBase->TaskWait.lh_Head;n->ln_Succ;n=n->ln_Succ)++*tasks;Permit();}
static int config(unsigned int cycle){FILE *f=fopen("ENV:NovaDock/novadock.cfg","w");static const char *sides[]={"BOTTOM","LEFT","TOP","RIGHT"};if(!f)return 0;fprintf(f,"SIDE=%s\nDRAG_BAR=YES\nALWAYS_ON_TOP=NO\nAUTO_HIDE=NO\nREMOVABLE_MEDIA=NO\nRUNNING_APPS=NO\nRUNNING_SECTION=NO\nTHEME=Nord\nDOCK_BACKGROUND=ARTWORK\nHOVER_MAGNIFICATION=100\nHOVER_ANIMATION=YES\nTOOLTIPS=YES\n",sides[cycle%4]);fputs("DRAWER=System|SYS:|SYS:Disk|ON|AUTO\nLAUNCH=Shell|NewShell|SYS:System/Shell|ON|AUTO\nPLUGIN=CPU|Plugins/Activity|CPU|METER|ON\nPLUGIN=RAM|Plugins/Memory|RAM|METER|ON\nPLUGIN=Clock|Plugins/Clock|CLOCK|METER|ON\nPLUGIN=Eyes|Plugins/Eyes|EYES|METER|ON\nPLUGIN=Games|Plugins/Games|WHDLOAD|METER|ON\nPLUGIN=Music|Plugins/MusicLibrary|MUSICLIB|METER|ON\nPLUGIN=Watch|Plugins/Watch|WATCH|METER|ON\nPLUGIN=Logs|Plugins/Logs|LOGS|METER|ON\nOPTION.WHDLOAD.DRAWER=QATests:Empty\nOPTION.MUSICLIB.DRAWER=QATests:Empty\nOPTION.WATCH.DRAWER=QATests:Empty\nOPTION.LOGS.FILE=QATests:soak-sample.log\n",f);fputs("GROUP=Popup|QATests:Empty|QATests:Dock/Icons/Folder.iff|ON|AUTO\n",f);if(cycle&1)fputs("PLUGIN=Pet|Plugins/Pet|PET|METER|ON\n",f);return fclose(f)==0;}
static int collection(const char *command,const char *title){struct Window *w;int ok=qa_rexx(command);printf("SOAK_CLICK %s rc=%d\n",command,ok);if(!ok||(w=qa_wait_window(title))==NULL){struct Screen *s;for(s=IntuitionBase->FirstScreen;s;s=s->NextScreen)for(w=s->FirstWindow;w;w=w->NextWindow)printf("SOAK_WINDOW %s\n",w->Title?w->Title:"untitled");return 0;}if(!qa_bounds(w))return 0;if(!qa_post(w,IDCMP_GADGETUP,0,3))return 0;Delay(10);return qa_close(title);}
int main(int argc,char **argv){struct NDDiagnosticsSnapshot d;struct NDControlPluginHealthMessage h;struct Screen *s;struct Window *w;FILE *f,*csv;BPTR dir;ULONG start,cycle_start,ports,tasks,base_ports=0,base_tasks=0,free_now,base_free=0,largest,base_largest=0,seconds=1800;unsigned int cycle=0,i;int result=10,cpu=-1;
 if(argc==2)seconds=(ULONG)atoi(argv[1]);if(seconds<20||seconds>3600||!qa_init())return 20;
 {BPTR prior=Lock("QATests:soak-started",ACCESS_READ);if(prior){UnLock(prior);printf("FAIL SOAK guest rebooted; previous alert %08lx task %08lx\n",SysBase->LastAlert[0],SysBase->LastAlert[1]);qa_finish();return 20;}f=fopen("QATests:soak-started","w");if(!f){qa_finish();return 20;}fputs("started",f);fclose(f);}

 SystemTags("MakeDir RAM:SoakEnv RAM:SoakEnv/NovaDock RAM:SoakArc",TAG_DONE);SystemTags("Assign ENV: RAM:SoakEnv",TAG_DONE);SystemTags("Assign ENVARC: RAM:SoakArc",TAG_DONE);SystemTags("Assign NovaDock: QATests:Dock",TAG_DONE);dir=CreateDir("QATests:Empty");if(dir)UnLock(dir);f=fopen("QATests:Empty/Sample.slave","w");if(!f)goto done;fputs("QA entry: never executed",f);fclose(f);SystemTags("Copy QATests:Dock/Music/#?.mod QATests:Empty/ QUIET",TAG_DONE);SystemTags("Copy QATests:Dock/NovaDock.info QATests:Empty/Sample.info QUIET",TAG_DONE);f=fopen("QATests:Empty/Sample.mod","w");if(!f)goto done;fputs("QA audio entry: never played",f);fclose(f);f=fopen("QATests:soak-sample.log","w");if(!f)goto done;fputs("Sample log for isolated soak\n",f);fclose(f);s=LockPubScreen("Workbench");if(!s)goto done;csv=fopen("QATests:soak.csv","w");if(!csv){UnlockPubScreen(NULL,s);goto done;}fputs("elapsed_seconds,cycle,free,largest,chip,ports,tasks,cpu,wakes,paints,updates\n",csv);
 printf("SOAK_BUILD %s requested_seconds=%lu\n",ND_BUILD_ID,seconds);start=qa_ticks();
 while(qa_ticks()-start<seconds*50UL || cycle<4){cycle_start=qa_ticks();
  if(!config(cycle)||!qa_start("QATests:Dock/NovaDock"))goto failed;
  for(i=0;i<500;++i){if(nd_control_diagnostics(&d)==ND_CONTROL_OK&&d.online_count==(cycle&1?9:8))break;Delay(2);}if(i==500)goto failed;
  printf("SOAK_STAGE online=%u tiles=%lu\n",d.online_count,d.tile_count);qa_rexx("LIST");if(nd_control_plugin_health("WHDLOAD",&h)==ND_CONTROL_OK)printf("SOAK_GAMES value=%s\n",h.value);f=fopen("ENV:NovaDock/novadock.source","r");if(f){char source[128];if(fgets(source,sizeof(source),f))printf("SOAK_SOURCE %s\n",source);fclose(f);}
  if(!collection("CLICK 7","WHDLoad")||!collection("CLICK 8","MUSIC"))goto failed;
  printf("SOAK_STAGE collections done\n");
  if(!qa_rexx("OPEN Popup"))goto failed;Delay(30);if(!qa_key(0x45,0))goto failed;Delay(10);
  if(!qa_start("QATests:Dock/NovaDockPrefs PAGE=APPEARANCE")||(w=qa_wait_window("NovaDock Preferences - Appearance"))==NULL||!qa_bounds(w)||!qa_close("NovaDock Preferences - Appearance"))goto failed;
  /* Change the live plugin set as well as restarting the dock each cycle. */
  if(!config(cycle+1)||nd_control_reload()!=ND_CONTROL_OK)goto failed;Delay(5);
  if(nd_control_diagnostics(&d)!=ND_CONTROL_OK)goto failed;
  if(!qa_pointer(s,d.viewport_left+d.viewport_width/3,d.viewport_top+d.viewport_height/2))goto failed;Delay(10);qa_pointer(s,s->Width/2,s->Height/2);Delay(10);
  cpu=-1;if(nd_control_plugin_health("CPU",&h)==ND_CONTROL_OK)cpu=atoi(h.value);
  if(!qa_stop())goto failed;Delay(40);
  for(i=0;i<8;++i){static const char *ids[]={"CPU","RAM","CLOCK","EYES","WHDLOAD","MUSICLIB","WATCH","LOGS"};char port[64];strcpy(port,ND_WAKE_PORT_PREFIX);strcat(port,ids[i]);if(qa_port(port))goto failed;}if(qa_port(ND_WAKE_PORT_PREFIX "PET"))goto failed;
  for(i=0;i<2;++i){char guard[64];sprintf(guard,"NovaDock.Folder.%u",i);if(qa_port(guard))goto failed;sprintf(guard,"NovaDock.FolderIcon.%u",i);if(qa_port(guard))goto failed;sprintf(guard,"NovaDock.Icon.%u",i);if(qa_port(guard))goto failed;}
  free_now=AvailMem(MEMF_ANY);largest=AvailMem(MEMF_LARGEST);counts(&ports,&tasks);
  if(cycle==3){base_free=free_now;base_largest=largest;base_ports=ports;base_tasks=tasks;printf("SOAK_BASE free=%lu largest=%lu ports=%lu tasks=%lu\n",base_free,base_largest,ports,tasks);}
  if(cycle>=4&&(free_now+65536UL<base_free||largest+131072UL<base_largest||ports>base_ports+2||tasks>base_tasks+2))goto failed;
  fprintf(csv,"%lu,%u,%lu,%lu,%lu,%lu,%lu,%d,%lu,%lu,%lu\n",(qa_ticks()-start)/50UL,cycle,free_now,largest,AvailMem(MEMF_CHIP),ports,tasks,cpu,d.wake_count,d.paint_count,d.update_count);fflush(csv);
  printf("SOAK_PROGRESS seconds=%lu cycle=%u free=%lu largest=%lu ports=%lu tasks=%lu cpu=%d\n",(qa_ticks()-start)/50UL,cycle,free_now,largest,ports,tasks,cpu);++cycle;
  while(qa_ticks()-cycle_start<500UL && qa_ticks()-start<seconds*50UL)Delay(10);
 }
 printf("PASS SOAK seconds=%lu cycles=%u free=%lu largest=%lu\n",(qa_ticks()-start)/50UL,cycle,AvailMem(MEMF_ANY),AvailMem(MEMF_LARGEST));result=0;goto cleanup;
 failed:printf("FAIL SOAK cycle=%u seconds=%lu DOS=%ld\n",cycle,(qa_ticks()-start)/50UL,IoErr());qa_stop();
 cleanup:fclose(csv);UnlockPubScreen(NULL,s);
 done:qa_finish();return result;}
