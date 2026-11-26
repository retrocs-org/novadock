#!/usr/bin/env python3
"""Keep the 2026-11-23 review fixes wired into the dock.

The policies themselves are exercised by tests/test_input_policy.c; these
checks make sure the dock, plugins, installer and tables keep using them.
"""
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def text(rel):
    return (ROOT / rel).read_text(encoding='latin-1')


def function(source, signature):
    """Body of the definition (not a prototype) starting with SIGNATURE."""
    start = source.index(signature)
    while True:
        depth, at = 0, start
        while True:  # find the parameter list's closing parenthesis
            depth += (source[at] == '(') - (source[at] == ')')
            if source[at] == ')' and depth == 0:
                break
            at += 1
        tail = source[at + 1:].lstrip()
        if tail.startswith('{'):
            break
        start = source.index(signature, start + 1)
    brace = source.index('{', start)
    depth, at = 1, brace + 1
    while depth:
        depth += (source[at] == '{') - (source[at] == '}')
        at += 1
    return source[start:at]


# Media constants used by the host policy test match the protocol header.
protocol = text('include/nd_protocol.h')
policy_test = text('tests/test_input_policy.c')
for name, local in (('PREVIOUS', 'PREVIOUS'), ('PLAY_PAUSE', 'PLAY_PAUSE'),
                    ('NEXT', 'NEXT'), ('VOLUME', 'VOLUME')):
    header = re.search(r'#define ND_EVENT_MEDIA_%s (\d+)U' % name, protocol).group(1)
    copy = re.search(r'#define %s (\d+)U' % local, policy_test).group(1)
    assert header == copy, name

# 1. The desktop snapshot always matches the dock size.
draw, window, reload = text('src/dock_draw.c'), text('src/dock_window.c'), text('src/dock_reload.c')
assert 'nd_snapshot_needs_alloc' in function(draw, 'int ensure_desktop_snapshot(')
assert 'dock->hover_desktop=AllocBitMap' not in window
assert draw.count('dock->hover_desktop=AllocBitMap') == 1  # only inside ensure_desktop_snapshot
assert 'ensure_desktop_snapshot(dock,locked->RastPort.BitMap)' in function(window, 'static int recapture_desktop(')
assert 'release_desktop_snapshot(dock);' in function(window, 'static int calculate_size(')
assert 'nd_live_reload_allowed(1,dock->hover_headroom,dock->hover_desktop!=NULL' in reload

# 2. Button events during a pending box change are deferred, never dropped.
events = text('src/dock_events.c')
loop = function(events, 'void window_events(')
assert 'nd_deferred_add(&dock->deferred_buttons' in loop
assert 'complete_window_box(dock)' in loop  # replays deferred buttons
assert 'sx=rx+dock->box_origin_x;sy=ry+dock->box_origin_y;' in loop  # pre-change origin
for rel in ('src/dock_timers.c', 'src/dock_window.c'):
    src = text(rel)
    assert src.count('ChangeWindowBox(dock->window') == src.count('dock->box_origin_x=dock->window->LeftEdge'), rel
assert 'if (dock->hidden || dock->box_pending) continue;\n            /* A queued button' not in loop
assert 'note_pointer_epoch(dock,secs,micros);' in loop
assert loop.count('note_pointer_epoch(dock,secs,micros);') == 2  # CHANGEWINDOW + ACTIVEWINDOW
sync = function(window, 'int sync_dock_viewport(')
assert 'dock->press_item>=0' in sync and 'IEQUALIFIER_LEFTBUTTON' in sync
assert 'note_pointer_epoch_now(dock)' in function(window, 'void apply_dock_viewport(')
assert 'nd_deferred_clear(&dock->deferred_buttons)' in function(window, 'static struct Window *create_dock_window(')
timers = text('src/dock_timers.c')
assert 'nd_press_orphaned(' in function(timers, 'static void orphan_press_tick(')
assert 'orphan_press_tick(dock);' in timers

# 3. Queued pointer events keep their own coordinates unless stale.
assert 'nd_pointer_event_stale(secs,micros,dock->pointer_epoch_secs' in loop

# 4. Music actions accumulate.
assert 'nd_media_fold(item->media_event,action' in function(draw, 'static int set_music_control(')
assert 'item->media_event=(unsigned short)action;' not in draw
assert 'nd_media_merge(' in text('src/dock_plugins.c')

# 6. A changed desktop is never recaptured under the pointer.
startup = function(window, 'void desktop_startup_tick(')
assert '(!runtime && (dock->tooltip || IntuitionBase->ActiveWindow==dock->window)) ||' in startup
assert re.search(r'\n\s+\(dock->screen->MouseX>=dock->window->LeftEdge', startup)

# 7. Always-on-top raises only for a real overlap.
top = function(timers, 'void always_on_top_tick(')
assert 'foreign_layer_overlaps(dock)' in top and 'WLayer->front)' not in top
assert 'nd_rect_overlap(' in function(timers, 'static int foreign_layer_overlaps(')

# 8. MusicPlayer never waits forever for a player.
music = text('plugin_src/musicplayer/musicplayer.c')
send = function(music, 'static LONG send_command(')
assert 'WaitPort(' not in send and 'nd_reply_abandon(' not in send
assert 'nd_wait_port(&music_timer, reply, ND_MUSIC_REXX_TIMEOUT_SECONDS * 50UL)' in send
assert 'stuck[slot].request = request' in send and 'stuck_slot(port)' in send
assert '!strcmp(stuck[i].port, port)' in function(music, 'static int stuck_slot(')
close = function(music, 'static void music_rexx_close(')
assert 'nd_reply_abandon(stuck[i].reply)' in close
for rel in ('plugin_src/common/nd_task.c', 'plugin_src/monitor/monitor.c'):
    worker = text(rel)
    assert 'nd_wait_port(NULL,reply,150UL)' in worker and 'Delay(2);}' not in worker, rel
assert 'nd_wait_port(&timer,job->port,100UL)' in function(text('src/nd_update_native.c'), 'static int wait_job(')
assert 'music_rexx_close();' in function(music, 'int main(')

# 9. ARexx launcher actions are bounded.
menu = text('src/dock_menu.c')
rexx_send = function(menu, 'static int send_rexx_action(')
assert 'nd_rexx_admit(dock->rexx_track,port' in rexx_send and 'nd_rexx_track(' in rexx_send
assert 'rexx_pending>=' not in rexx_send  # no global cap across ports
assert 'nd_rexx_release(dock->rexx_track,reply)' in function(menu, 'void process_rexx_replies(')

# 10. A second upgrade can replace NovaDock.previous.
install = text('Install')
before = install.index('(rename #app (cat #app ".previous"))')
assert 'Delete \\"" #app ".previous\\" ALL QUIET FORCE' in install[before - 600:before]

# 13. Third review round: box-change timeout, deferred menu, layer lock,
# epoch clock resets, per-tile shelf refresh and cached label widths.
assert 'complete_window_box(dock)' in loop
assert 'dock->deferred_menu=1;' in loop
assert 'complete_window_box(dock)' in function(timers, 'static void box_pending_tick(')
assert 'box_pending_tick(dock);' in timers
complete = function(events, 'int complete_window_box(')
assert 'replay_deferred_buttons(dock)' in complete and 'dock->deferred_menu' in complete
layers = function(timers, 'static int foreign_layer_overlaps(')
assert 'LockLayerInfo(&dock->screen->LayerInfo)' in layers and 'UnlockLayerInfo(' in layers
assert 'nd_pointer_epoch_update(' in function(events, 'void note_pointer_epoch(')
assert 'refresh_shelf_tiles(dock)' in function(draw, 'void flush_dirty_tiles(')
assert 'refresh_shelf_tiles(dock)' in function(draw, 'void draw_tile(')
assert 'TextLength(' not in function(draw, 'static void prepare_hover_layout(')
assert 'offsetof(struct NDTaskPacket,data)+bytes' in text('plugin_src/common/nd_task.c')

# 14. Fourth round: partial hover frames, idle auto-hide timer, ARexx
# settings/INFO/SAVE/UNDO/STATUS DETAIL, Edit-mode undo and catalogues.
hover = function(draw, 'void draw_hover(')
assert 'nd_hover_dirty_span(' in hover and 'diagnostic_hover_skipped' in hover
assert 'dock->hover_prev_valid=0;' in function(draw, 'void draw_all(')
assert 'dock->hover_prev_valid=0;' in loop  # REFRESHWINDOW damage
assert 'dock->hover_tile_changed[i]=1' in function(draw, 'static int refresh_shelf_tiles(')
assert 'nd_autohide_needs_fast_timer(' in timers and 'dock->conf.auto_hide ||' not in timers
arexx = text('src/dock_arexx.c')
for verb in ('ND_RX_GET', 'ND_RX_SET', 'ND_RX_THEME', 'ND_RX_INFO', 'ND_RX_SAVE', 'ND_RX_UNDO'):
    assert 'case %s:' % verb in arexx, verb
assert 'known_theme(value)' in function(arexx, 'static int rexx_set(')
# SET/THEME queue the edit; one reload applies a run of them (timer or the
# next command that reads the dock), and any reload clears the queue.
assert 'queue_settings_save(dock,&full)' in function(arexx, 'static int rexx_set(')
assert 'dock->rexx_apply_pending=1;' in function(arexx, 'static int rexx_set(')
assert 'flush_rexx_settings(dock);' in function(arexx, 'void process_rexx(')
assert 'flush_rexx_settings(dock)' in timers and 'dock->rexx_apply_pending' in function(timers, 'static void arm_timer(')
assert 'dock->rexx_apply_pending=0;' in function(text('src/dock_reload.c'), 'int save_and_reload(')
assert 'apply_configuration(dock,dock->edit_config)' in function(text('src/dock_reload.c'), 'void flush_rexx_settings(')
reload_src = text('src/dock_reload.c')
assert reload_src.count('remember_undo(dock,full);') == 2
assert 'release_undo(dock);' in function(menu, 'void set_edit_mode(')
assert 'undo_last_edit(dock)' in function(menu, 'static int dock_popup_action(')
assert 'nd_config_get(cfg, nd_config_keys[i]' in function(text('src/config.c'), 'int nd_write_config(')
for lang in ('deutsch', 'svenska', 'italiano', 'dansk'):
    assert (ROOT / 'catalogs' / (lang + '.ct')).is_file(), lang

# 15. Fifth round: decoration-aware spans with overlap closure, undo ends on
# other changes, abandoned MusicPlayer slots, late box acknowledgements,
# fallback recovery ticks and clock-reset-only epoch rewinds.
assert 'hover_tile_extent(dock,i,&dock->hover_ext[i])' in hover
assert 'nd_hover_draw_set(' in hover and 'dock->wave_draw[i]' in function(draw, 'static void paint_hover_wave(')
assert 'refresh_shelf_tiles(dock)' in function(draw, 'void redraw_changed_tiles(')
assert 'if (!dock->undo_keep) dock->undo_kind=ND_UNDO_NONE;' in reload_src
assert 'if(!dock->undo_keep)dock->undo_kind=ND_UNDO_NONE;' in function(text('src/dock_save.c'), 'void accept_settings_save(')
assert reload_src.count('dock->undo_keep=1;') == 3
reclaim = function(music, 'static void reclaim_stuck_requests(')
assert 'ND_MUSIC_STUCK_EXPIRE' in reclaim and 'nd_reply_abandon(stuck[i].reply)' in reclaim and 'FindPort(' in reclaim
assert 'dock->box_late_ack' in loop and 'fallback_recovery_tick(dock);' in loop
assert 'CurrentTime(&now_secs,&now_micros)' in function(events, 'void note_pointer_epoch(')
assert 'nd_config_canonical_key(' in function(arexx, 'static int rexx_set(')
assert 'nd_config_canonical_key(' in function(arexx, 'static int rexx_get(')

# 16. Sixth round: drawing-order overlap set on exact bounds, hidden tiles
# ignored, event-driven plugin panels, bounded ARexx shutdown wait and
# sleeping helper/quit waits.
assert 'dock->hover_visible' in hover and 'v,lo,hi,dock->hover,dock->wave_draw' in hover
for rel in ('plugin_src/mixer/mixer.c', 'plugin_src/runningapps/runningapps.c', 'plugin_src/games/games.c'):
    src = text(rel)
    assert 'Delay(5);elapsed=5;' not in src and 'nd_client_wait_signals(&client' in src, rel
stop = function(menu, 'void stop_rexx_replies(')
assert 'nd_rexx_live(' in stop and 'nd_wait_port(&timer,dock->rexx_reply' in stop and 'Delay(5)' not in stop
for rel in ('plugin_src/screenmode/screenmode.c', 'plugin_src/shelf/shelf.c', 'plugin_src/tooltypes/tooltypes.c'):
    assert 'nd_wait_port(NULL,reply,' in text(rel), rel
quit_save = function(text('src/dock_save.c'), 'int finish_settings_saves(')
assert 'nd_wait_signals(&timer,mask,10UL)' in quit_save and 'Delay(1)' not in quit_save

# 12. Limits follow ND_MAX_ITEMS.
assert '#define ND_PLUGIN_CATALOG_MAX ND_MAX_ITEMS' in text('include/nd_plugin_discovery.h')
trace = text('src/nd_trace.c')
assert 'tiles[48]' not in trace and 'index>=48' not in trace and 'tiles[ND_MAX_ITEMS]' in trace

# 11. Every theme index round-trips through the configuration to its folder.
program = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_config.h"
#include "nd_theme.h"
int main(void){static struct NDConfig a,b;int i;char line[80];
 for(i=0;i<ND_THEME_COUNT;++i){const char *dir=nd_theme_directory(i);
  nd_default_config(&a);sprintf(line,"THEME=%s",dir);assert(nd_parse_line(&a,line));
  assert(a.theme==i);assert(!strcmp(nd_config_theme_id(&a),dir));
  assert(strcmp(nd_theme_name(i),"MOCHA")||i==ND_THEME_MOCHA);
  assert(nd_write_config(&a,"theme-roundtrip.cfg"));nd_default_config(&b);
  assert(nd_load_config(&b,"theme-roundtrip.cfg")==0);assert(b.theme==i);
  assert(!strcmp(nd_config_theme_id(&b),dir));}
 remove("theme-roundtrip.cfg");remove("theme-roundtrip.cfg.bak");remove("theme-roundtrip.cfg.old");
 return 0;}
'''
with tempfile.TemporaryDirectory(prefix='novadock-theme-ids-') as tmp:
    tmp = Path(tmp)
    (tmp / 'check.c').write_text(program)
    cmd = [os.environ.get('CC', 'cc'), '-std=c99', '-I' + str(ROOT / 'include'),
           str(tmp / 'check.c'), str(ROOT / 'src/config.c'), str(ROOT / 'src/nd_options.c'),
           str(ROOT / 'src/theme.c'), '-o', str(tmp / 'check')]
    subprocess.run(cmd, check=True)
    subprocess.run([str(tmp / 'check')], cwd=tmp, check=True)

print('Review fixes: snapshot sizing, deferred buttons, pointer epochs, music folding, '
      'recapture and always-on-top guards, MusicPlayer timeout, ARexx cap, installer, '
      'theme IDs, item limits, box-change timeout, deferred menus, layer locking, '
      'shelf tile refresh, partial hover frames, ARexx settings and Edit-mode '
      'undo (ending on other changes), decoration-aware hover spans and stuck '
      'player recovery, event-driven panels and sleeping waits are wired in.')

# Cached Eyes/Clock settings: paint/timer paths never search the option table,
# and every place that replaces the live configuration clears the cache.
import re as _re
_draw = text('src/dock_draw.c')
_loader = function(_draw, 'void load_settings_cache(')
for _src_name in ('src/dock_draw.c', 'src/dock_timers.c'):
    _src = text(_src_name).replace(_loader, '').replace(function(_draw, 'static int eyes_color_option('), '')
    assert not _re.search(r'nd_option_get\(&dock->conf,"(EYES|CLOCK)"', _src), _src_name
    assert 'eyes_option_num(' not in _src and 'eyes_option_yes(' not in _src, _src_name
assert 'swap_configs(&dock->conf,buffer);\n    invalidate_settings_cache(dock);' in text('src/dock_reload.c')
assert 'dock->conf = *old;\n        invalidate_settings_cache(dock);' in text('src/dock_reload.c')
assert 'dock->conf=*next;invalidate_settings_cache(dock);' in text('src/dock_plugins.c')
assert 'nd_load_effective(&dock->conf);\n    invalidate_settings_cache(dock);' in text('src/novadock.c')
for _name in ('src/dock_events.c', 'src/dock_menu.c', 'src/dock_arexx.c', 'src/dock_timers.c', 'src/dock_save.c'):
    for _m in _re.finditer(r'nd_option_set\(&dock->conf,[^\n]*\n', text(_name)):
        assert 'invalidate_settings_cache(dock);' in text(_name)[_m.end():_m.end()+80], _name
print('Settings cache: Eyes/Clock paths use the cache; all live-config replacements invalidate it.')
