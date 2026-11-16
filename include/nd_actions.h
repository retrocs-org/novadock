#ifndef ND_ACTIONS_H
#define ND_ACTIONS_H
#include <string.h>
#define ND_ACTION_COUNT 6
#define ND_ACTION_REBOOT 0
#define ND_ACTION_WORKBENCH 1
#define ND_ACTION_DOCK 2
#define ND_ACTION_SHELL 3
#define ND_ACTION_FRONT 4
#define ND_ACTION_BACK 5
struct NDAction {const char *name,*notice;};
static const struct NDAction nd_actions[ND_ACTION_COUNT]={
 {"Restart Amiga","Unsaved work will be lost. Restart the entire Amiga?"},
 {"Restart Workbench","Reopen Workbench; other programs keep running."},
 {"Restart NovaDock","Restart NovaDock and its plugin windows."},
 {"Open Shell","Open a new AmigaDOS Shell window."},
 {"Workbench front","Bring the Workbench screen to the front."},
 {"Workbench back","Send the Workbench screen behind other screens."}};
static int nd_action_index(const char *name)
{int i;if(name)for(i=0;i<ND_ACTION_COUNT;++i)if(!strcmp(name,nd_actions[i].name))return i;return -1;}
#endif
