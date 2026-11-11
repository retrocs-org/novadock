#ifndef ND_MUSICPLAYER_H
#define ND_MUSICPLAYER_H
/* NovaDock 0.1: portable multi-player configuration and text-display helpers.
 * Metadata and volume commands are optional because the players/versions may
 * expose different ARexx command names and results.
 */
#define ND_MUSIC_PORT_LEN 31
#define ND_MUSIC_COMMAND_LEN 63
#define ND_MUSIC_CONFIG_LINE 192
#define ND_MUSIC_TITLE_LEN 120
#define ND_MUSIC_TIME_LEN 28
#define ND_MUSIC_AUTO 0
#define ND_MUSIC_EAGLE 1
#define ND_MUSIC_AMP 2
#define ND_MUSIC_HIPPO 3
#define ND_MUSIC_NONE 0

#define ND_MUSIC_PLAY 0
#define ND_MUSIC_PAUSE 1
#define ND_MUSIC_STOP 2
#define ND_MUSIC_PREVIOUS 3
#define ND_MUSIC_NEXT 4
#define ND_MUSIC_ACTIONS 5

#define ND_MUSIC_QUERY_TITLE 0
#define ND_MUSIC_QUERY_TIME 1
#define ND_MUSIC_VOL_UP 2
#define ND_MUSIC_VOL_DOWN 3
#define ND_MUSIC_QUERY_VOLUME 4
#define ND_MUSIC_SET_VOLUME 5
#define ND_MUSIC_QUERY_PLAYING 6
#define ND_MUSIC_EXTRAS 7

struct NDMusicProfile {
    char port[ND_MUSIC_PORT_LEN + 1];
    char alt_port[ND_MUSIC_PORT_LEN + 1];
    char commands[ND_MUSIC_ACTIONS][ND_MUSIC_COMMAND_LEN + 1];
    char extras[ND_MUSIC_EXTRAS][ND_MUSIC_COMMAND_LEN + 1];
    unsigned int volume_max;
};
struct NDMusicConfig {
    struct NDMusicProfile eagle;
    struct NDMusicProfile amp;
    struct NDMusicProfile hippo;
    unsigned int poll_ticks;
    unsigned int query_ticks;
    unsigned int scroll_ticks;
    unsigned int title_width;
    int mode;
    int prefer;
    int auto_query;
    int scroll_title;
};

void nd_music_defaults(struct NDMusicConfig *cfg);
int nd_music_apply_option(void *ctx, const char *key, const char *value);
int nd_music_valid_port(const char *port);
int nd_music_valid_command(const char *cmd);
int nd_music_load(struct NDMusicConfig *cfg, const char *filename);
int nd_music_select(int mode, int prefer, int eagle_present, int amp_present,
                    int hippo_present);
const char *nd_music_name(int player);
const struct NDMusicProfile *nd_music_profile(const struct NDMusicConfig *cfg, int player);
/* HippoPlayer changes playlist selection and starts it as two ARexx actions. */
const char *nd_music_followup(int player, int action);
/* Removes control characters from untrusted result strings without modifying
 * the source buffer. Bytes >= 0x80 (Amiga Latin-1 characters) are retained. */
void nd_music_clean_text(char *out, unsigned int capacity, const char *input);
/* Generate one viewport of a padded, looping title; bounded, NUL terminated. */
void nd_music_marquee(char *out, unsigned int capacity, const char *title,
                      unsigned int position, unsigned int width, int enabled);
/* Convert a player's raw volume and build its configured absolute command. */
int nd_music_parse_volume(const struct NDMusicProfile *profile,
                          const char *text, unsigned int *percent);
int nd_music_volume_command(const struct NDMusicProfile *profile,
                            unsigned int percent, char *out,
                            unsigned int capacity);
int nd_music_parse_playing(const char *text, int *playing);
#endif
