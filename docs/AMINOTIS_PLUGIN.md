# AMINOTIS: AmiNotis inbox

AMINOTIS shows the inbox of the separately installed AmiNotis notification
service in the dock. It replaces the retired session-only ALERTS tile. The
dock's own notices still reach AmiNotis through General Preferences >
**Send alerts to AmiNotis**. AMINOTIS reads the shared inbox that AmiNotis
keeps for every application.

## Set up

1. Install [AmiNotis](https://aminotis.retrocs.org) and start its service, or
   let it start with Workbench.
2. Add **AMINOTIS** through NovaDock Preferences -> Organize -> Add -> Plugin.

The plugin needs no other setup. When the service is not running, the tile
shows a sad bell with **AmiNotis not running**. It checks for the service
every five seconds and continues by itself once AmiNotis starts.

## Tile

| Artwork | Meaning |
| --- | --- |
| Bell | Connected, nothing unread |
| Bell with red dot | Unread notifications; the badge shows the count |
| Bell with blue Z | Quiet mode (Do Not Disturb) is on, with its end time when one is set |
| Bell with sad face | AmiNotis is not running, or the installed AmiNotis is too old |

The tile follows the inbox live. A parked AmiNotis watch wakes the plugin
only when the history, unread state or quiet mode changes. It does not poll
the service. All requests are asynchronous and bounded, so a busy or stopped
service never holds up the dock.

## Panel

Click the tile to open a non-modal panel. It shows the unread and total
counts and four notifications per page, newest first: an asterisk for
unread, time, application, title and the start of the body. The buttons are:

- **Newer** and **Older** page through the inbox.
- **Read all** marks the notifications you have seen as read. Anything that
  arrives after the panel refreshed stays unread.
- **Quiet 1h** turns on quiet mode for an hour. While quiet mode is on, the
  button reads **Quiet off**.
- **Inbox** opens the full AmiNotis dashboard.

Read state, quiet mode and dismissals belong to AmiNotis, so every AmiNotis
frontend shows the same result.

## Settings

| Setting | Values | Default |
| --- | --- | --- |
| Click opens | `PANEL` (the dock panel) or `INBOX` (the AmiNotis dashboard) | `PANEL` |
| Badge counts | `BADGE` (sources that take part in badges) or `UNREAD` (all unread) | `BADGE` |
| Flash on new notifications | Yes or No | No |

## Build

The plugin is `plugin_src/aminotis/aminotis.c`. `plugin_src/aminotis/sdk/`
holds an unchanged copy of the AmiNotis v2 SDK. That copy is newer than the
snapshot in `src/aminotis_sdk/`, which the dock uses for forwarding.
`nd_aminotis_sdk.c` builds the copy with plugin resource accounting. Refresh
the three SDK files together and update the digests in
`tests/test_aminotis_plugin.py`.

`tools/generate_aminotis_art.py` writes `Plugins/AmiNotis.assets/Artwork.ndart`
and `Plugins/AmiNotis.info`. Run it with `--check` to verify both.
`plugin_src/aminotis/build.sh` builds only this plugin through
`Makefile.cross` and checks that the patched vbcc startup is linked.
