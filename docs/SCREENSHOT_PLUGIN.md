# Screenshot plugin

The Screenshot tile (`Plugins/Screenshot`, ID `SCREENSHOT`) saves a screen as
an IFF ILBM picture that any Amiga paint program or viewer can load.

## Using it

1. Click the camera icon. Its countdown-clock symbol gives you time to bring another
   screen to the front first. A click during the countdown cancels it.
2. The front screen (or Workbench, see Settings) is copied and saved as
   `Screenshot-YYYYMMDD-HHMMSS.iff` in the chosen drawer. A second shot within
   the same second gets `-1`, `-2` and so on.
3. A check mark confirms the picture was saved. The badge counts shots taken
   since the plugin started. A Workbench `.info` icon is created beside the
   image so it appears in Icons Only views; double-click opens MultiView.

The camera changes to a flash while capturing or writing the picture, then
returns to its idle appearance after the saved indication. A cross marks a
cancelled or failed operation. Hover over the tile for countdown, save
progress or error details; status words are kept out of the tile itself.

## What is saved

- Indexed screens retain their palette (`CMAP`) and display mode (`CAMG`).
  15-, 16-, 24- and 32-bit RTG screens are read through the CyberGraphX-compatible
  interface (including compatible Picasso96 setups) and saved as 24-bit RGB
  ILBM images. Both paths use lossless ByteRun1 compression.
- The screen is copied with one blit while Intuition is locked, then written
  without the lock, so other programs are held up only for that copy. The copy
  needs as much memory as the screen itself; `NO MEMORY` means it was not
  available.
- True-colour capture needs `cybergraphics.library` version 41 or newer and
  a compatible RTG bitmap. Missing support reports `NO RTG READER`.
- Screen copies use memory according to their size and depth. True-colour
  encoding needs only one additional RGB row, rather than another full image.
- The mouse pointer is a sprite and is not part of the picture.

## Settings

| Setting | Meaning | Default |
|---|---|---|
| Save in drawer | Where pictures are written; `NO DRAWER` if it does not exist | `RAM:` |
| Countdown seconds | 0-30; 0 saves immediately | 2 |
| Screen | `FRONT` saves the frontmost screen, `WORKBENCH` the Workbench screen | FRONT |
