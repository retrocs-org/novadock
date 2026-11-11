#ifndef ND_OPTION_RUNTIME_H
#define ND_OPTION_RUNTIME_H
/* Standalone plugin processes read the ONE NovaDock config, never their own.
 * Consumer callback returns 0 on invalid value, 1 on accepted setting. */
/* One line naming the configuration file the running dock loaded. */
#define ND_CONFIG_SOURCE_FILE "ENV:NovaDock/novadock.source"
typedef int (*NDOptionApply)(void *ctx, const char *key, const char *value);
int nd_plugin_options_read(const char *path, const char *id,
                           NDOptionApply apply, void *ctx);
int nd_plugin_options_effective(const char *id, NDOptionApply apply, void *ctx);
int nd_plugin_option_string(const char *id, const char *key,
                            const char *fallback, char *out, unsigned int limit);
/* Fill all requested strings with one scan. Invalid input restores defaults. */
struct NDOptionString {const char *key,*fallback;char *out;unsigned int limit;};
int nd_plugin_option_strings(const char *id,struct NDOptionString *options,unsigned int count);
#endif
