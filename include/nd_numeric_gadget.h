#ifndef ND_NUMERIC_GADGET_H
#define ND_NUMERIC_GADGET_H

/* INTEGER_KIND's edit hook is unreliable with some input/keymap combinations.
 * Use an ordinary STRING_KIND and validate the complete decimal value when
 * the user applies it. This also makes Backspace, replacement and Tab behave
 * like the other text fields in NovaDock Preferences. */
static int nd_numeric_gadget_read(struct Gadget *g, struct Window *window,
                                  LONG *value)
{
    struct StringInfo *info;
    STRPTR text=NULL;
    LONG parsed=0;
    char tail=0;
    if (!g || !window || !value) return 0;
    /* The native edit buffer is authoritative. Some classic GadTools
     * versions return zero for a GTST_String query even while this buffer is
     * valid, which previously made every apply path silently stop here. */
    info=(struct StringInfo *)g->SpecialInfo;
    if (info && info->Buffer) text=info->Buffer;
    if (!text &&
        !GT_GetGadgetAttrs(g,window,NULL,
                           GTST_String,(ULONG)&text,TAG_DONE))
        text=NULL;
    if (!text || !*text || sscanf(text,"%ld%c",&parsed,&tail)!=1)
        return 0;
    *value=parsed;
    return 1;
}

#endif
