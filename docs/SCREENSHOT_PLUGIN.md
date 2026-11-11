# Screenshot plugin

The Screenshot tile (`Plugins/Screenshot`, ID `SCREENSHOT`) saves a screen as
an IFF ILBM picture that any Amiga paint program or viewer can load.

## Using it

1. Click the tile. It counts down (`IN 2`, `IN 1`), so you can bring another
   screen to the front first. A click during the countdown cancels it.
2. The front screen (or Workbench, see Settings) is copied and saved as
   `Screenshot-YYYYMMDD-HHMMSS.iff` in the chosen drawer. A second shot within
   the same second gets `-1`, `-2` and so on.
3. The tile shows `SAVED`, and its badge counts the shots taken since the
   plugin started.

While a large screen is being written the tile shows `SAVING n%`.

## What is saved

- The whole screen at its own resolution, with its palette (`CMAP`) and
  display mode (`CAMG`), ByteRun1 compressed, up to 256 colours.
- The screen is copied with one blit while Intuition is locked, then written
  without the lock, so other programs are held up only for that copy. The copy
  needs as much memory as the screen itself; `NO MEMORY` means it was not
  available.
- 15-, 16- and 24-bit RTG screens are not supported yet and report
  `DEEP SCREEN`. 8-bit RTG screens work.
- The mouse pointer is a sprite and is not part of the picture.

## Settings

| Setting | Meaning | Default |
|---|---|---|
| Save in drawer | Where pictures are written; `NO DRAWER` if it does not exist | `RAM:` |
| Countdown seconds | 0-30; 0 saves immediately | 2 |
| Screen | `FRONT` saves the frontmost screen, `WORKBENCH` the Workbench screen | FRONT |
