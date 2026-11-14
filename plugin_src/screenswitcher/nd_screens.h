#ifndef ND_SCREENS_H
#define ND_SCREENS_H
#define ND_SCREEN_MAX 32
#define ND_SCREEN_NAME 128
struct NDScreenEntry {char name[ND_SCREEN_NAME];unsigned int width,height;};
/* Copy names while the public list is locked. No screen pointer survives it. */
static int nd_screen_collect(struct NDScreenEntry *entries)
{
    struct List *list=LockPubScreenList();
    struct PubScreenNode *node;
    int count=0,i,j;
    if(!list)return 0;
    for(node=(struct PubScreenNode *)list->lh_Head;node->psn_Node.ln_Succ;
        node=(struct PubScreenNode *)node->psn_Node.ln_Succ){
        const char *name=node->psn_Node.ln_Name;
        if((node->psn_Flags&PSNF_PRIVATE) || !node->psn_Screen || !name ||
           !*name || strlen(name)>=ND_SCREEN_NAME || count>=ND_SCREEN_MAX)continue;
        strcpy(entries[count].name,name);
        entries[count].width=(unsigned int)node->psn_Screen->Width;
        entries[count].height=(unsigned int)node->psn_Screen->Height;++count;
    }
    UnlockPubScreenList();
    for(i=1;i<count;++i)for(j=i;j>0 && strcmp(entries[j-1].name,entries[j].name)>0;--j){
        struct NDScreenEntry swap=entries[j];entries[j]=entries[j-1];entries[j-1]=swap;
    }
    return count;
}
/* The owner may close a screen while the picker is open: lock its name anew. */
static int nd_screen_raise(const char *name)
{
    struct Screen *screen=LockPubScreen((STRPTR)name);
    if(!screen)return 0;
    ScreenToFront(screen);UnlockPubScreen(NULL,screen);return 1;
}
#endif
