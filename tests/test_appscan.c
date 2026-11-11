/* Run the production scanner against controlled Intuition window lists. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef unsigned long ULONG;
typedef char *STRPTR;
#define APP_MAX 48
#define APP_LABEL 48
#define WFLG_BACKDROP 1
struct Node {char *ln_Name;};
struct Task {struct Node tc_Node;};
struct MsgPort {struct Task *mp_SigTask;};
struct Window {struct Window *NextWindow; char *Title; struct MsgPort *UserPort; ULONG Flags;};
struct Screen {struct Screen *NextScreen; struct Window *FirstWindow;};
struct TestBase {struct Screen *FirstScreen;};
static struct TestBase base,*IntuitionBase=&base;
static struct Task self={{"RunningApps"}},wb={{"Workbench"}},app={{"Editor"}};
static struct Window *raised,*activated;
static struct Screen *front;
static void Forbid(void){}
static void Permit(void){}
static struct Task *FindTask(STRPTR name){return name ? &wb : &self;}
static ULONG LockIBase(int n){(void)n;return 0;}
static void UnlockIBase(ULONG lock){(void)lock;}
static void ScreenToFront(struct Screen *s){front=s;}
static void WindowToFront(struct Window *w){raised=w;}
static void ActivateWindow(struct Window *w){activated=w;}
#include "../plugin_src/runningapps/nd_appscan.h"
int main(void)
{
    struct MsgPort editor={&app},workbench={&wb},own={&self};
    struct Window w[8];struct Screen screen,other;
    struct AppEntry entries[APP_MAX],closed;
    int n,i;
    memset(w,0,sizeof(w));
    for(i=0;i<7;++i)w[i].NextWindow=&w[i+1];
    w[0].Title="Editor one";w[0].UserPort=&editor;
    w[1].Title="Editor two";w[1].UserPort=&editor;
    w[2].Title="AmigaShell";w[3].Title="AmigaShell";
    w[4].Title="Workbench";w[4].UserPort=&workbench;
    w[5].Title="NovaDock Apps"; /* another helper must not list itself */
    w[6].Title="Background";w[6].Flags=WFLG_BACKDROP;
    w[7].Title="Self";w[7].UserPort=&own;
    screen.FirstWindow=w;screen.NextScreen=NULL;base.FirstScreen=&screen;
    n=collect_apps(entries);assert(n==3);assert(count_apps()==3);
    assert(!strcmp(entries[0].label,"AmigaShell"));
    assert(!strcmp(entries[1].label,"AmigaShell"));
    assert(entries[0].window!=entries[1].window);
    assert(raise_app(&entries[0]));assert(raised==entries[0].window && activated==raised);
    assert(raise_app(&entries[1]));assert(raised==entries[1].window);
    /* Selection is revalidated: closing a console must not raise another. */
    closed=entries[0];
    for(i=0;i<7;++i)if(w[i].NextWindow==closed.window)w[i].NextWindow=closed.window->NextWindow;
    assert(!raise_app(&closed));
    other.FirstWindow=NULL;other.NextScreen=&screen;base.FirstScreen=&other;
    assert(raise_app(&entries[2]));assert(front==&screen);
    puts("Apps scanner: console windows, grouping, exclusions and stale selection passed.");
    return 0;
}
