# MusicPlayer plugin — EaglePlayer, AmigaAMP and HippoPlayer

MusicPlayer is one logical dock item for three separately installed AmigaOS
music players. On a top or bottom dock it spans two ordinary tile widths so
the module title and controls are not cramped. It remains one item for
selection, dragging, paging and configuration. Left and right docks retain a
normal single-column tile. MusicPlayer discovers known configurable ARexx
ports, allows manual choice, and can issue playback controls through explicitly
configured ARexx messages. Detected port presence **does not** guarantee known
playback status.

While a supported player is detected, the dock tile itself shows compact
Previous, Play/Pause and Next buttons. A volume track is also shown when the
selected profile has both a volume query and an absolute volume command.
Click or drag along that track to set the level. Clicking elsewhere on the
tile is intentionally inert while a player is online, keeping the fixed
two-slot layout stable and avoiding a redundant second control window. When
no supported player is available, NovaDock shows the bundled musical-note
icon instead. Set `DEFAULT_APP` in the plugin settings and clicking that idle
tile starts the selected player.

Right-click the MusicPlayer tile → **Plugin settings...**. The unified
`NovaDockPrefs` provides paginated settings for `MUSIC`, including:

```ini
OPTION.MUSIC.MODE=AUTO
OPTION.MUSIC.PREFER=EAGLEPLAYER
OPTION.MUSIC.DEFAULT_APP=SYS:Utilities/HippoPlayer/HippoPlayer
OPTION.MUSIC.POLL_TICKS=100
OPTION.MUSIC.AUTO_QUERY=YES
OPTION.MUSIC.QUERY_TICKS=100
OPTION.MUSIC.SCROLL_TITLE=YES
OPTION.MUSIC.SCROLL_TICKS=25
OPTION.MUSIC.TITLE_WIDTH=24
OPTION.MUSIC.EAGLE_PORT=rexx_EP
OPTION.MUSIC.EAGLE_QUERY_TITLE=STATUS M FIL
OPTION.MUSIC.EAGLE_QUERY_VOLUME=STATUS G VOL
OPTION.MUSIC.EAGLE_SET_VOLUME=VOLUME
OPTION.MUSIC.EAGLE_VOLUME_MAX=255
OPTION.MUSIC.AMIGAAMP_PORT=AMIGAAMP
OPTION.MUSIC.HIPPO_PORT=HIPPOPLAYER
OPTION.MUSIC.HIPPO_PLAY=CONT
OPTION.MUSIC.HIPPO_PAUSE=STOP
OPTION.MUSIC.HIPPO_PREVIOUS=CHOOSEPREV
OPTION.MUSIC.HIPPO_NEXT=CHOOSENEXT
OPTION.MUSIC.HIPPO_QUERY_TITLE=GET NAME
OPTION.MUSIC.HIPPO_QUERY_PLAYING=GET PLAY
OPTION.MUSIC.HIPPO_QUERY_VOLUME=GET VOLU
OPTION.MUSIC.HIPPO_SET_VOLUME=VOLUME
OPTION.MUSIC.HIPPO_VOLUME_MAX=64
```

All settings are in the **same** `novadock.cfg` as the dock and the other
plugins. EaglePlayer 2.x is preconfigured with its documented `rexx_EP`
ARexx port and `STATUS M FIL` module-name query. Commands include optional `EAGLE_QUERY_TITLE`,
`AMIGAAMP_QUERY_TITLE`, volume up/down and other player-specific fields.
HippoPlayer is detected through its `HIPPOPLAYER` ARexx port. The bundled
profile follows the current HippoPlayer source interface: Previous/Next first
select the adjacent playlist entry and then issue `PLAY`; Play/Pause uses
`CONT`/`STOP`; `GET NAME`, `GET PLAY` and `GET VOLU` keep the tile title,
button state and 0..64 volume slider synchronized. These newer commands are
intended for HippoPlayer 2.47 or later. Select `HIPPOPLAYER` for MODE to force
it, or leave MODE at AUTO and set PREFER if several supported players run.

AmigaAMP commands remain version-dependent and should be checked against the
installed player. Automatic metadata queries are enabled for EaglePlayer.
EaglePlayer's bundled 2.06 guide documents `STATUS G VOL` and `VOLUME 0..255`,
which NovaDock converts to and from the tile's 0..100 percent slider. HippoPlayer's
documented 0..64 range is converted in the same way. AmigaAMP
volume fields remain empty by default because command syntax varies by version;
configure its query, set command and maximum before the slider is enabled.

Inline actions travel back in the plugin's existing request/reply message.
They create no additional task, timer, public port or idle polling loop.

Old `MusicPlayer.cfg` files are not read in this release. Transfer customized
ports or command mappings into `OPTION.MUSIC.*` before upgrading.
No player binaries or copyrighted music are included.
