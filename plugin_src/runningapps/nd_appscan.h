/* Shared window enumeration and activation policy, tested with fake Intuition lists. */
#ifndef ND_APPSCAN_H
#define ND_APPSCAN_H
struct AppEntry {
    struct Task *task;
    struct Window *window; /* consoles may have no application UserPort */
    char label[APP_LABEL];
};

static int ignored_task(struct Task *task,struct Task *self,
                        struct Task *workbench)
{
    const char *name;
    if (!task || task==self || task==workbench) return 1;
    name=(const char *)task->tc_Node.ln_Name;
    if (!name) return 0;
    return !strcmp(name,"NovaDock") || !strcmp(name,"NovaDockPrefs") ||
           !strcmp(name,"RunningApps");
}

static int collect_apps(struct AppEntry *apps);

/* Use the same window policy for the badge and the popup. */
static int count_apps(void)
{
    struct AppEntry apps[APP_MAX];
    return collect_apps(apps);
}

static int collect_apps(struct AppEntry *apps)
{
    struct Task *self=FindTask(NULL),*workbench;
    struct Screen *screen;
    struct Window *window;
    ULONG lock;
    int count=0,i,j;
    Forbid();workbench=FindTask((STRPTR)"Workbench");Permit();
    lock=LockIBase(0);
    for (screen=IntuitionBase->FirstScreen;screen;screen=screen->NextScreen)
        for (window=screen->FirstWindow;window;window=window->NextWindow) {
            struct Task *task;
            const char *title=(const char *)window->Title;
            if (!title || !title[0] || (window->Flags&WFLG_BACKDROP) ||
                !strcmp(title,"NovaDock Apps")) continue;
            task=window->UserPort ? window->UserPort->mp_SigTask : NULL;
            if (task && ignored_task(task,self,workbench)) continue;
            /* Ordinary apps group by task. Console windows have no IDCMP
             * owner, so retain each window as its own selectable entry. */
            for (i=0;i<count;++i)
                if (task ? apps[i].task==task : apps[i].window==window) break;
            if (i<count || count>=APP_MAX) continue;
            apps[count].task=task;apps[count].window=window;
            strncpy(apps[count].label,title,APP_LABEL-1);
            apps[count].label[APP_LABEL-1]='\0';
            ++count;
        }
    UnlockIBase(lock);
    for (i=0;i<count-1;++i)
        for (j=i+1;j<count;++j)
            if (strcmp(apps[i].label,apps[j].label)>0) {
                struct AppEntry swap=apps[i];apps[i]=apps[j];apps[j]=swap;
            }
    return count;
}

static int raise_app(const struct AppEntry *app)
{
    struct Screen *screen,*owner=NULL;
    struct Window *window,*found=NULL;
    ULONG lock;
    int front=0;
    if (!app || !app->window) return 0;
    lock=LockIBase(0);
    for (screen=IntuitionBase->FirstScreen;screen && !found;
         screen=screen->NextScreen)
        for (window=screen->FirstWindow;window;window=window->NextWindow)
            if (window==app->window &&
                (window->UserPort ? window->UserPort->mp_SigTask : NULL)==app->task) {
                found=window;owner=screen;break;
            }
    front=owner && owner==IntuitionBase->FirstScreen;
    UnlockIBase(lock);
    if (!found) return 0;
    if (!front) ScreenToFront(owner);
    WindowToFront(found);ActivateWindow(found);
    return 1;
}

#endif
