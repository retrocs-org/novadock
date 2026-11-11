#ifndef ND_PREFS_HELP_H
#define ND_PREFS_HELP_H
/* The Help key (raw key code) opens the NovaDock manual from any
 * Preferences window. */
#define ND_HELP_KEY 0x5F
/* Start MultiView on NovaDock.guide as a separate program, so Preferences
 * stays usable.  0 if the guide or MultiView cannot be found. */
int nd_prefs_help_launch(void);
/* As above, beeping on SCREEN on failure.  A macro because each
 * Preferences page names its own IntuitionBase. */
#define nd_prefs_help(screen) \
    ((void)(nd_prefs_help_launch() || (DisplayBeep(screen),0)))
#endif
