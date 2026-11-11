/* NovaDock 0.1 MusicPlayer: EaglePlayer, AmigaAMP and HippoPlayer controller.
 * Separate Exec process; no player binaries, engines or libraries included.
 * Port presence is NOT proof of playback. Metadata queries are configurable.
 */
#include <exec/types.h>
#include <exec/libraries.h>
#include <rexx/storage.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/rexxsyslib.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <stdio.h>
#include <string.h>
#include "nd_version.h"
#include "nd_client.h"
#include "nd_option_runtime.h"
#include "nd_musicplayer.h"

/* Version string for the AmigaDOS Version command (kept global so the
 * linker never discards it). */
const char nd_version[]=ND_VER_STRING("MusicPlayer");
/* Honoured by the vbcc and libnix startup code; harmless elsewhere. */
long __stack=16384;

#define MP_W 474
#define MP_H 167
#define MP_G_PREV 100
#define MP_G_PLAY 101
#define MP_G_PAUSE 102
#define MP_G_STOP 103
#define MP_G_NEXT 104
#define MP_G_MODE 105
#define MP_G_CLOSE 106
#define MP_G_INFO 107
#define MP_G_VOL_DOWN 108
#define MP_G_VOL_UP 109

struct IntuitionBase *IntuitionBase = NULL;
struct Library *GadToolsBase = NULL;
struct RxsLib *RexxSysBase = NULL;

struct MPPanel {
    struct Screen *screen;
    struct Window *window;
    APTR visual;
    struct Gadget *gadgets;
    struct Gadget *last;
    char status[155];
};
struct MPMetadata {
    int player;
    char port[ND_MUSIC_PORT_LEN + 1];
    char title[ND_MUSIC_TITLE_LEN + 1];
    char time[ND_MUSIC_TIME_LEN + 1];
    unsigned int offset;
};
static void clear_metadata(struct MPMetadata *meta)
{
    memset(meta, 0, sizeof(*meta));
}
static void metadata_player(struct MPMetadata *meta, int player, const char *port)
{
    if (meta->player != player ||
        strcmp(meta->port, port ? port : "")) {
        clear_metadata(meta);
        meta->player = player;
        if (port) strcpy(meta->port, port);
    }
}


static int file_exists(const char *path)
{
    BPTR lock = Lock((STRPTR)path, ACCESS_READ);
    if (!lock) return 0;
    UnLock(lock);
    return 1;
}
static int load_effective(struct NDMusicConfig *cfg)
{
    nd_music_defaults(cfg);
    return nd_plugin_options_effective("MUSIC",nd_music_apply_option,cfg)==1;
}
static int has_port(const char *name)
{
    int found;
    if (!nd_music_valid_port(name)) return 0;
    Forbid();
    found = FindPort((STRPTR)name) != NULL;
    Permit();
    return found;
}
static const char *detected_port(const struct NDMusicProfile *profile)
{
    if (has_port(profile->port)) return profile->port;
    if (has_port(profile->alt_port)) return profile->alt_port;
    return NULL;
}
/* ARexx synchronous request. RESULT is an optional ArgString returned by
 * the player on success (not a numeric error). We copy it before freeing.
 * These calls are only automatic when AUTO_QUERY=YES; a non-responding host
 * can block this separate plugin process, never NovaDock itself.
 * NEVER free a RexxMsg or reply port while its request is outstanding.
 */
static LONG send_command(const char *port, const char *command,
                         char *output, unsigned int output_len)
{
    struct MsgPort *reply, *target;
    struct RexxMsg *request, *response;
    LONG result = 20;
    int delivered = 0;
    if (output && output_len) output[0] = 0;
    if (!RexxSysBase || !nd_music_valid_port(port) ||
        !nd_music_valid_command(command)) return 20;
    reply = CreateMsgPort();
    if (!reply) return 20;
    request = CreateRexxMsg(reply, (STRPTR)"rexx", (STRPTR)port);
    if (!request) { DeleteMsgPort(reply); return 20; }
    request->rm_Args[0] = CreateArgstring((STRPTR)command, (LONG)strlen(command));
    if (!request->rm_Args[0]) {
        DeleteRexxMsg(request); DeleteMsgPort(reply); return 20;
    }
    request->rm_Action = RXCOMM | (output && output_len ? RXFF_RESULT : 0);
    Forbid();
    target = FindPort((STRPTR)port);
    if (target) {
        PutMsg(target, (struct Message *)request);
        delivered = 1;
    }
    Permit();
    if (delivered) {
        WaitPort(reply);
        response = (struct RexxMsg *)GetMsg(reply);
        if (response == request) {
            result = request->rm_Result1;
            if (result == 0 && output && output_len && request->rm_Result2) {
                STRPTR text = (STRPTR)request->rm_Result2;
                nd_music_clean_text(output, output_len, text);
                DeleteArgstring(text);
                request->rm_Result2 = 0;
            }
        }
    }
    DeleteArgstring(request->rm_Args[0]);
    DeleteRexxMsg(request);
    DeleteMsgPort(reply);
    return result;
}
/* Only send configured queries. A command returning no RESULT must not
 * produce a fabricated title or elapsed time. */
static int read_metadata(const struct NDMusicProfile *profile,
                         const char *port, struct MPMetadata *meta,
                         unsigned int *volume, int *playing)
{
    LONG rc;
    char value[ND_MUSIC_TITLE_LEN + 1];
    int succeeded = 0;
    if (!profile || !port || !meta) return 0;
    if (profile->extras[ND_MUSIC_QUERY_TITLE][0]) {
        rc = send_command(port, profile->extras[ND_MUSIC_QUERY_TITLE],
                          value, sizeof(value));
        if (rc == 0) {
            strcpy(meta->title, value);
            meta->offset = 0;
            ++succeeded;
        } else meta->title[0] = 0;
    }
    if (profile->extras[ND_MUSIC_QUERY_TIME][0]) {
        rc = send_command(port, profile->extras[ND_MUSIC_QUERY_TIME],
                          value, sizeof(value));
        if (rc == 0) {
            nd_music_clean_text(meta->time, sizeof(meta->time), value);
            ++succeeded;
        } else meta->time[0] = 0;
    }
    if (volume && profile->extras[ND_MUSIC_QUERY_VOLUME][0]) {
        rc = send_command(port, profile->extras[ND_MUSIC_QUERY_VOLUME],
                          value, sizeof(value));
        if (rc == 0 && nd_music_parse_volume(profile, value, volume))
            ++succeeded;
        else *volume = ND_MEDIA_VOLUME_UNKNOWN;
    }
    if (playing && profile->extras[ND_MUSIC_QUERY_PLAYING][0]) {
        rc = send_command(port,profile->extras[ND_MUSIC_QUERY_PLAYING],
                          value,sizeof(value));
        if (rc==0 && nd_music_parse_playing(value,playing)) ++succeeded;
    }
    return succeeded;
}
static unsigned int media_capabilities(const struct NDMusicProfile *profile)
{
    unsigned int result = 0;
    if (!profile) return 0;
    if (profile->commands[ND_MUSIC_PREVIOUS][0]) result |= ND_MEDIA_CAP_PREVIOUS;
    if (profile->commands[ND_MUSIC_PLAY][0] &&
        profile->commands[ND_MUSIC_PAUSE][0]) result |= ND_MEDIA_CAP_PLAY_PAUSE;
    if (profile->commands[ND_MUSIC_NEXT][0]) result |= ND_MEDIA_CAP_NEXT;
    if (profile->extras[ND_MUSIC_QUERY_VOLUME][0] &&
        profile->extras[ND_MUSIC_SET_VOLUME][0] && profile->volume_max)
        result |= ND_MEDIA_CAP_VOLUME;
    return result;
}
static int media_action(unsigned int event, unsigned int requested_volume,
                        const struct NDMusicProfile *profile,
                        const char *port, int player, int *playing)
{
    const char *command = NULL;
    const char *followup;
    char volume_command[ND_MUSIC_COMMAND_LEN + 1];
    LONG rc;
    int action=-1;
    if (!profile || !port || !playing) return 0;
    if (event & ND_EVENT_MEDIA_PREVIOUS) {
        action=ND_MUSIC_PREVIOUS;
        command = profile->commands[ND_MUSIC_PREVIOUS];
    } else if (event & ND_EVENT_MEDIA_PLAY_PAUSE) {
        action=*playing ? ND_MUSIC_PAUSE : ND_MUSIC_PLAY;
        command = profile->commands[action];
    } else if (event & ND_EVENT_MEDIA_NEXT) {
        action=ND_MUSIC_NEXT;
        command = profile->commands[ND_MUSIC_NEXT];
    } else if ((event & ND_EVENT_MEDIA_VOLUME) && requested_volume <= 100U) {
        if (!nd_music_volume_command(profile, requested_volume,
                                     volume_command, sizeof(volume_command)))
            return 0;
        command = volume_command;
    }
    if (!command || !command[0]) return 0;
    rc = send_command(port, command, NULL, 0);
    followup=nd_music_followup(player,action);
    if (rc==0 && followup) rc=send_command(port,followup,NULL,0);
    if (rc == 0 && (event & ND_EVENT_MEDIA_PLAY_PAUSE)) *playing = !*playing;
    return rc == 0;
}
static struct Gadget *button(struct MPPanel *p, WORD x, WORD y,
                              WORD width, UWORD id, STRPTR title)
{
    struct NewGadget ng;
    memset(&ng, 0, sizeof(ng));
    ng.ng_LeftEdge = x;
    ng.ng_TopEdge = y;
    ng.ng_Width = width;
    ng.ng_Height = 18;
    ng.ng_GadgetID = id;
    ng.ng_GadgetText = title;
    ng.ng_TextAttr = p->screen->Font;
    ng.ng_VisualInfo = p->visual;
    ng.ng_Flags = PLACETEXT_IN;
    p->last = CreateGadget(BUTTON_KIND, p->last, &ng, TAG_DONE);
    return p->last;
}
static void panel_close(struct MPPanel *p)
{
    if (p->window) { CloseWindow(p->window); p->window = NULL; }
    if (p->gadgets) { FreeGadgets(p->gadgets); p->gadgets = NULL; p->last = NULL; }
    if (p->visual) { FreeVisualInfo(p->visual); p->visual = NULL; }
    if (p->screen) { UnlockPubScreen(NULL, p->screen); p->screen = NULL; }
}
static void panel_status(struct MPPanel *p, const char *text)
{
    if (!p->window) return;
    strncpy(p->status, text, sizeof(p->status) - 1);
    p->status[sizeof(p->status) - 1] = 0;
    SetWindowTitles(p->window, (STRPTR)p->status, (STRPTR)-1);
}
static const char *mode_name(int mode)
{
    if (mode == ND_MUSIC_EAGLE) return "EaglePlayer only";
    if (mode == ND_MUSIC_AMP) return "AmigaAMP only";
    if (mode == ND_MUSIC_HIPPO) return "HippoPlayer only";
    return "Automatic";
}
static int panel_open(struct MPPanel *p)
{
    WORD left, top;
    if (p->window) {
        WindowToFront(p->window);
        ActivateWindow(p->window);
        return 1;
    }
    p->screen = LockPubScreen("Workbench");
    if (!p->screen) return 0;
    if (p->screen->Width < MP_W + 8 ||
        p->screen->Height < MP_H + p->screen->BarHeight) goto failed;
    p->visual = GetVisualInfo(p->screen, TAG_DONE);
    if (!p->visual || !CreateContext(&p->gadgets)) goto failed;
    p->last = p->gadgets;
    if (!button(p, 10, 42, 87, MP_G_PREV, "Previous") ||
        !button(p, 102, 42, 87, MP_G_PLAY, "Play") ||
        !button(p, 194, 42, 87, MP_G_PAUSE, "Pause") ||
        !button(p, 286, 42, 87, MP_G_STOP, "Stop") ||
        !button(p, 378, 42, 86, MP_G_NEXT, "Next") ||
        !button(p, 10, 74, 145, MP_G_MODE, "Switch player") ||
        !button(p, 168, 74, 138, MP_G_INFO, "Refresh info") ||
        !button(p, 313, 74, 151, MP_G_CLOSE, "Close") ||
        !button(p, 10, 107, 219, MP_G_VOL_DOWN, "Volume -") ||
        !button(p, 244, 107, 220, MP_G_VOL_UP, "Volume +")) goto failed;
    left = (WORD)((p->screen->Width - MP_W) / 2);
    top = (WORD)(p->screen->BarHeight + 1);
    p->window = OpenWindowTags(NULL,
                               WA_PubScreen, (ULONG)p->screen,
                               WA_Left, left, WA_Top, top,
                               WA_Width, MP_W, WA_Height, MP_H,
                               WA_Title, (ULONG)"NovaDock MusicPlayer",
                               WA_Gadgets, (ULONG)p->gadgets,
                               WA_DragBar, TRUE, WA_CloseGadget, TRUE,
                               WA_DepthGadget, TRUE, WA_Activate, TRUE,
                               WA_IDCMP, IDCMP_GADGETUP | IDCMP_CLOSEWINDOW |
                                         IDCMP_REFRESHWINDOW,
                               WA_Flags, WFLG_SMART_REFRESH | WFLG_ACTIVATE,
                               TAG_DONE);
    if (!p->window) goto failed;
    GT_RefreshWindow(p->window, NULL);
    return 1;
failed:
    panel_close(p);
    return 0;
}
/* Always check every profile on demand; never reuse stale port pointers or
 * silently switch players during an already-clicked playback command.
 */
static int current_player(const struct NDMusicConfig *cfg, int mode,
                          const char **port_out)
{
    const char *ep = detected_port(&cfg->eagle);
    const char *amp = detected_port(&cfg->amp);
    const char *hippo = detected_port(&cfg->hippo);
    int selected;
    /* Upgrade old 0.1 configs transparently.  Early builds used the main
     * application-port label instead of EaglePlayer 2.x's ARexx host name. */
    if (!ep && has_port("rexx_EP")) ep = "rexx_EP";
    selected = nd_music_select(mode,cfg->prefer,ep != NULL,amp != NULL,
                               hippo != NULL);
    *port_out = selected == ND_MUSIC_EAGLE ? ep :
                selected == ND_MUSIC_AMP ? amp :
                selected == ND_MUSIC_HIPPO ? hippo : NULL;
    return selected;
}
/* String is shown as a window title to avoid font or refresh assumptions.
 * The dock tile independently shows the scrolling title, if known. */
static void show_metadata(struct MPPanel *p, const struct MPMetadata *meta,
                          int player)
{
    char label[155];
    size_t used;
    strcpy(label, nd_music_name(player));
    if (meta->title[0]) {
        strcat(label, " - ");
        used = strlen(label);
        strncat(label, meta->title, sizeof(label) - used - 1);
    } else strcat(label, " - title unavailable");
    if (meta->time[0]) {
        used = strlen(label);
        strncat(label, " [", sizeof(label) - used - 1);
        used = strlen(label);
        strncat(label, meta->time, sizeof(label) - used - 1);
        used = strlen(label);
        strncat(label, "]", sizeof(label) - used - 1);
    }
    panel_status(p, label);
}
static void panel_events(struct MPPanel *p, const struct NDMusicConfig *cfg,
                         int config_ok, int *override_mode,
                         struct MPMetadata *meta)
{
    struct IntuiMessage *msg;
    ULONG kind;
    UWORD id;
    int action, player, mode;
    LONG rc;
    const char *port, *command;
    const struct NDMusicProfile *profile;
    char title[155];
    if (!p->window) return;
    while (p->window && (msg = GT_GetIMsg(p->window->UserPort)) != NULL) {
        kind = msg->Class;
        id = kind == IDCMP_GADGETUP ?
            ((struct Gadget *)msg->IAddress)->GadgetID : 0;
        GT_ReplyIMsg(msg);
        if (kind == IDCMP_CLOSEWINDOW || id == MP_G_CLOSE) {
            panel_close(p);
            break;
        }
        if (kind == IDCMP_REFRESHWINDOW) {
            GT_RefreshWindow(p->window, NULL);
            continue;
        }
        mode = *override_mode < 0 ? cfg->mode : *override_mode;
        if (id == MP_G_MODE) {
            mode = mode == ND_MUSIC_AUTO ? ND_MUSIC_EAGLE :
                   mode == ND_MUSIC_EAGLE ? ND_MUSIC_AMP :
                   mode == ND_MUSIC_AMP ? ND_MUSIC_HIPPO : ND_MUSIC_AUTO;
            *override_mode = mode;
            clear_metadata(meta);
            sprintf(title, "Selection: %s (temporary)", mode_name(mode));
            panel_status(p, title);
            continue;
        }
        if (id != MP_G_INFO && id != MP_G_VOL_UP && id != MP_G_VOL_DOWN &&
            id != MP_G_PLAY && id != MP_G_PAUSE && id != MP_G_STOP &&
            id != MP_G_PREV && id != MP_G_NEXT) continue;
        if (!config_ok) { panel_status(p, "Invalid unified MusicPlayer settings"); continue; }
        if (!RexxSysBase) { panel_status(p, "ARexx library unavailable"); continue; }
        player = current_player(cfg, mode, &port);
        profile = nd_music_profile(cfg, player);
        if (!profile || !port) { panel_status(p, "Selected player is offline"); continue; }
        metadata_player(meta, player, port);
        if (id == MP_G_INFO) {
            if (!profile->extras[ND_MUSIC_QUERY_TITLE][0] &&
                !profile->extras[ND_MUSIC_QUERY_TIME][0]) {
                panel_status(p, "Configure QUERY_TITLE or QUERY_TIME first");
            } else {
                (void)read_metadata(profile, port, meta, NULL, NULL);
                show_metadata(p, meta, player);
            }
            continue;
        }
        if (id == MP_G_VOL_UP || id == MP_G_VOL_DOWN) {
            command = profile->extras[id == MP_G_VOL_UP ? ND_MUSIC_VOL_UP : ND_MUSIC_VOL_DOWN];
            if (!command[0]) {
                panel_status(p, "Volume command is not configured");
                continue;
            }
        } else {
            action = id == MP_G_PLAY ? ND_MUSIC_PLAY :
                     id == MP_G_PAUSE ? ND_MUSIC_PAUSE :
                     id == MP_G_STOP ? ND_MUSIC_STOP :
                     id == MP_G_PREV ? ND_MUSIC_PREVIOUS : ND_MUSIC_NEXT;
            command = profile->commands[action];
        }
        rc = send_command(port, command, NULL, 0);
        if (rc==0 && (id==MP_G_PREV || id==MP_G_NEXT)) {
            const char *followup=nd_music_followup(player,
                id==MP_G_PREV ? ND_MUSIC_PREVIOUS : ND_MUSIC_NEXT);
            if (followup) rc=send_command(port,followup,NULL,0);
        }
        sprintf(title, "%s: %s (ARexx RC %ld)", nd_music_name(player),
                rc == 0 ? "command accepted" : "command failed", rc);
        panel_status(p, title);
    }
}
int main(int argc, char **argv)
{
    struct NDClient client;
    struct NDMusicConfig cfg;
    struct MPPanel panel;
    struct MPMetadata meta;
    char value[ND_TEXT_LEN];
    const char *port;
    const struct NDMusicProfile *profile;
    unsigned int event;
    unsigned int requested_volume = ND_MEDIA_VOLUME_UNKNOWN;
    unsigned int volume = ND_MEDIA_VOLUME_UNKNOWN;
    unsigned int capabilities = 0;
    unsigned int ticks = 0, scroll_ticks = 0, query_ticks = 0;
    unsigned int wait_ticks, effective_wait, elapsed=0, remaining;
    int config_ok, player, quit = 0, override_mode = -1, refresh = 1;
    int mode, active = ND_MUSIC_NONE, previous_active = ND_MUSIC_NONE;
    int playing = 0;
    if (argc < 2 || !nd_client_init(&client, argv[1])) return 20;
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39);
    GadToolsBase = OpenLibrary("gadtools.library", 39);
    RexxSysBase = (struct RxsLib *)OpenLibrary("rexxsyslib.library", 0);
    if (!IntuitionBase || !GadToolsBase) {
        if (RexxSysBase) CloseLibrary((struct Library *)RexxSysBase);
        if (GadToolsBase) CloseLibrary(GadToolsBase);
        if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
        nd_client_close(&client);
        return 20;
    }
    memset(&panel, 0, sizeof(panel));
    clear_metadata(&meta);
    nd_music_defaults(&cfg);
    config_ok = load_effective(&cfg);
    while (!quit) {
        if (refresh || ticks >= cfg.poll_ticks) {
            config_ok = load_effective(&cfg);
            if (!config_ok) nd_music_defaults(&cfg);
            mode = override_mode < 0 ? cfg.mode : override_mode;
            player = current_player(&cfg, mode, &port);
            if (!config_ok || !RexxSysBase) player = ND_MUSIC_NONE;
            metadata_player(&meta, player, port);
            active = player;
            profile = nd_music_profile(&cfg, player);
            if (active != previous_active) {
                volume = ND_MEDIA_VOLUME_UNKNOWN;
                playing = active != ND_MUSIC_NONE;
                query_ticks = cfg.query_ticks;
                previous_active = active;
            }
            capabilities = media_capabilities(profile);
            if (cfg.auto_query && profile && port &&
                query_ticks >= cfg.query_ticks) {
                (void)read_metadata(profile,port,&meta,&volume,&playing);
                if (panel.window) show_metadata(&panel, &meta, player);
                query_ticks = 0;
            }
            ticks = 0;
            refresh = 1;
        }
        if (refresh || (cfg.scroll_title && meta.title[0] &&
                        scroll_ticks >= cfg.scroll_ticks)) {
            if (!config_ok) strcpy(value, "CONFIG ERROR");
            else if (!RexxSysBase) strcpy(value, "NO AREXX");
            else if (active == ND_MUSIC_NONE) strcpy(value, "OFFLINE");
            else if (meta.title[0]) {
                nd_music_marquee(value, sizeof(value), meta.title,
                                  meta.offset, cfg.title_width, cfg.scroll_title);
                if (cfg.scroll_title) ++meta.offset;
            } else strcpy(value, nd_music_name(active));
            event = nd_client_update_media_quiet(&client, "MUSIC", value,
                capabilities, playing ? ND_MEDIA_STATE_PLAYING : 0U,
                volume, &requested_volume, elapsed, 500U);
            if (event & ND_EVENT_QUIT) quit = 1;
            if (!quit && profile && port &&
                (event & (ND_EVENT_MEDIA_PREVIOUS |
                          ND_EVENT_MEDIA_PLAY_PAUSE |
                          ND_EVENT_MEDIA_NEXT |
                          ND_EVENT_MEDIA_VOLUME))) {
                if (media_action(event,requested_volume,profile,port,player,
                                 &playing)) {
                    if ((event & ND_EVENT_MEDIA_VOLUME) &&
                        requested_volume <= 100U)
                        volume = requested_volume;
                    query_ticks = cfg.query_ticks;
                }
            }
            /* The dock's wide music tile is the stable control surface.
             * Ordinary tile clicks are deliberately ignored; only explicit
             * previous/play/next/volume events act on playback. */
            scroll_ticks = 0;
            refresh = 0;
        }
        if (!quit) panel_events(&panel, &cfg, config_ok, &override_mode, &meta);
        if (!quit) {
            /* With no control panel open there is no reason to wake ten
             * times per second. Sleep until the next poll/marquee deadline;
             * NovaDock interrupts the wait for controls or changed settings. */
            wait_ticks=ticks<cfg.poll_ticks ? cfg.poll_ticks-ticks : 1U;
            if (cfg.scroll_title && meta.title[0]) {
                remaining=scroll_ticks<cfg.scroll_ticks ?
                          cfg.scroll_ticks-scroll_ticks : 1U;
                if (remaining<wait_ticks) wait_ticks=remaining;
            }
            if (panel.window) {
                Delay(5);
                elapsed=5;
            } else {
                effective_wait=nd_client_delay_ticks(&client,wait_ticks);
                elapsed=nd_client_wait(&client,effective_wait);
                if (elapsed<effective_wait) refresh=1;
            }
            ticks += elapsed;
            scroll_ticks += elapsed;
            if (query_ticks < 60000U) {
                if (elapsed>60000U-query_ticks) query_ticks=60000U;
                else query_ticks += elapsed;
            }
        }
    }
    panel_close(&panel);
    nd_client_close(&client);
    if (RexxSysBase) CloseLibrary((struct Library *)RexxSysBase);
    CloseLibrary(GadToolsBase);
    CloseLibrary((struct Library *)IntuitionBase);
    return 0;
}
