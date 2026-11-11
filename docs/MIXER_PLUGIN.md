# Mixer plugin

The Mixer tile (`Plugins/Mixer`, ID `MIXER`) shows the AHI output volume and
changes it. It needs AHI 4 or later.

## Using it

- The tile shows the volume stored in AHI's preferences, `75%` for example,
  with a matching meter, or `MUTED`. `NO AHI` means `ENV:Sys/ahi.prefs` was not
  found: open **Prefs/AHI** and choose **Save** once.
- Click the tile to open a small window with a volume slider and a **Mute**
  box. Release the slider to apply the new volume; **Close**, Esc or Return
  closes the window. Click the tile again to close it as well.

## How the volume is applied

AHI has no system-wide "master volume" that every program shares. What it
does have is an output volume for the sound card, stored per unit in
`ENV:Sys/ahi.prefs` and sent to the card's driver whenever a program starts
using the default audio mode. The Mixer:

1. writes the new volume to every unit in `ENV:Sys/ahi.prefs`;
2. reopens `ahi.device`, which re-reads the file, and briefly allocates the
   default audio mode so the driver receives the new value at once;
3. copies the file to `ENVARC:Sys/ahi.prefs` when the window closes (setting
   **Remember volume after restart**), so a floppy or slow card is not written
   at every step.

The status line in the window says what happened:

| Status | Meaning |
|---|---|
| Applied | The driver has an output volume control and took the new value. |
| Card in use: applies on next playback | Another program has the sound card (most drivers allow only one at a time). The value is saved and is used the next time a program starts playing. |
| Driver has no volume control | This audio mode's driver reports no adjustable output volume (Paula, for example). The value is still stored for drivers and units that use it. |
| ahi.device not available | AHI is not installed. |

## Settings

| Setting | Meaning | Default |
|---|---|---|
| Remember volume after restart | Copy the changed preferences to ENVARC: when the window closes | YES |
| Refresh ticks | How often the tile re-reads the preferences (50 per second) | 100 |

Preference saves first write and close `ahi.prefs.new`, then replace the target
while preserving the previous complete file as `ahi.prefs.bak`. Failed writes
or closes leave the original intact. Failed replacement rolls back; if that
also fails, the complete original remains in `.bak`. Permanent-save failures
show SAVE FAILED and stay pending: reopen and close Mixer to retry. If Mixer
exits before saving succeeds, it displays an explanation after releasing dock
IPC. Check free space and write access to ENVARC:Sys.
