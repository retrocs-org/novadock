#ifndef ND_LOCALE_H
#define ND_LOCALE_H
/* Optional locale.library catalogs for NovaDock's own messages.
 * English is built in; Catalogs/<language>/novadock.catalog overrides it.
 * Strings are looked up by their English text, which tools/
 * generate_catalog.py maps to the numeric IDs in catalogs/novadock.cd. */
void nd_locale_open(void);
void nd_locale_close(void);
/* Translation of ENGLISH (or ENGLISH itself). */
const char *nd_tr(const char *english);
/* For printf-style formats: the translation is used only if it is short
 * enough for NovaDock's fixed message buffers (ND_TR_FORMAT_MAX). */
#define ND_TR_FORMAT_MAX 80
const char *nd_trf(const char *english_format);
#endif
