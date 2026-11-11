/* NovaDock catalog support.  Without locale.library (or without a catalog
 * for the user's language) every lookup returns the built-in English. */
#include <string.h>
#include "nd_locale.h"
#include "nd_strings.h"

#if defined(__amigaos__) || defined(AMIGA) || defined(__VBCC__)
#include <exec/types.h>
#include <libraries/locale.h>
#include <proto/exec.h>
#include <proto/locale.h>
struct LocaleBase *LocaleBase=NULL;
static struct Catalog *catalog=NULL;
void nd_locale_open(void)
{
    LocaleBase=(struct LocaleBase *)OpenLibrary("locale.library",38);
    if (LocaleBase)
        catalog=OpenCatalog(NULL,(STRPTR)"novadock.catalog",
                            OC_BuiltInLanguage,(ULONG)"english",TAG_DONE);
}
void nd_locale_close(void)
{
    if (LocaleBase) {
        if (catalog) CloseCatalog(catalog);
        catalog=NULL;
        CloseLibrary((struct Library *)LocaleBase);
        LocaleBase=NULL;
    }
}
static const char *lookup(long id,const char *english)
{
    if (!catalog || !LocaleBase) return english;
    return (const char *)GetCatalogStr(catalog,id,(STRPTR)english);
}
#else
void nd_locale_open(void) {}
void nd_locale_close(void) {}
static const char *lookup(long id,const char *english)
{
    (void)id;
    return english;
}
#endif

static long string_id(const char *english)
{
    unsigned int i;
    if (!english) return -1;
    for (i=0;i<sizeof(nd_strings)/sizeof(nd_strings[0]);++i)
        if (!strcmp(nd_strings[i].text,english)) return nd_strings[i].id;
    return -1;
}
const char *nd_tr(const char *english)
{
    long id=string_id(english);
    return id<0 ? english : lookup(id,english);
}
const char *nd_trf(const char *english_format)
{
    const char *translated=nd_tr(english_format);
    /* Formats are written into fixed buffers: keep a long translation from
     * overflowing them, and require the same conversions in the same order. */
    const char *a=english_format,*b=translated;
    if (!translated || strlen(translated)>ND_TR_FORMAT_MAX) return english_format;
    for (;;) {
        a=strchr(a,'%');b=strchr(b,'%');
        if (!a || !b) break;
        while (*a && !strchr("sdu",*a)) ++a;
        while (*b && !strchr("sdu",*b)) ++b;
        if (*a!=*b) return english_format;
        if (*a) ++a;
        if (*b) ++b;
    }
    return (!a && !b) ? translated : english_format;
}
