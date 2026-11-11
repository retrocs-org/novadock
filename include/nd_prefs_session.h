#ifndef ND_PREFS_SESSION_H
#define ND_PREFS_SESSION_H

/* The unified executable owns one working configuration for its lifetime.
 * Standalone compatibility editors continue to load their own copy. */
#ifdef ND_PREFS_EMBEDDED
extern struct NDConfig *nd_prefs_shared_config;
/* NULL when the starting settings are kept as text in T: instead. */
extern struct NDConfig *nd_prefs_shared_original;
extern int nd_prefs_shared_dirty;
extern int nd_prefs_preview_active;
#define ND_PREFS_SHARED_CONFIG nd_prefs_shared_config
#define ND_PREFS_SHARED_ORIGINAL nd_prefs_shared_original
#define ND_PREFS_MARK_DIRTY() (nd_prefs_shared_dirty=1)
#define ND_PREFS_MARK_CLEAN() (nd_prefs_shared_dirty=0)
#define ND_PREFS_IS_DIRTY() (nd_prefs_shared_dirty!=0)
#define ND_PREFS_MARK_PREVIEW() (nd_prefs_preview_active=1)
#define ND_PREFS_CLEAR_PREVIEW() (nd_prefs_preview_active=0)
#else
#define ND_PREFS_SHARED_CONFIG NULL
#define ND_PREFS_SHARED_ORIGINAL NULL
#define ND_PREFS_MARK_DIRTY() ((void)0)
#define ND_PREFS_MARK_CLEAN() ((void)0)
#define ND_PREFS_IS_DIRTY() 0
#define ND_PREFS_MARK_PREVIEW() ((void)0)
#define ND_PREFS_CLEAR_PREVIEW() ((void)0)
#endif

#endif
