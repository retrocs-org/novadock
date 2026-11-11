# NovaDock removable media tiles

When a floppy, PCMCIA/CF card, USB stick or mounted disk image is present,
NovaDock shows it as a temporary drawer tile at the end of the dock. The tile
disappears when the medium is removed. Nothing is written to the
configuration file.

## Using the tiles

- **Click** opens the volume, like any drawer tile.
- **Drop** files or drawers on the centre of the tile to copy them onto the
  disk (works while the dock is locked).
- **Right-click** offers:
  - **Open** and **Popup contents** - as for drawers.
  - **Flush (safe to remove)** - inhibits and immediately re-enables the
    drive, which makes the filesystem write all pending data. The disk is
    then safe to remove until something writes to it again. Amiga drives
    have no motorised eject, so this replaces an "Eject" command.
  - **Pin to dock** - adds a permanent drawer tile for the volume. The
    temporary tile then disappears, because a pinned volume is never shown
    twice.
- The used-space meter is shown as for pinned volumes.
- Temporary tiles cannot be dragged, reordered or removed; they always stay
  after the configured items. Pin a disk first to arrange it. New launchers
  dropped on the dock are inserted before the temporary tiles.

A status message reports **Disk inserted** and **Disk removed**.

## What counts as removable

A volume qualifies when its DOS device is handled by one of these exec
devices:

```text
trackdisk.device   internal and external floppy drives (DF0:-DF3:)
mfm.device         CrossDOS PC floppies (PC0:, PC1:)
carddisk.device    PCMCIA memory cards on A600/A1200
compactflash.device  CF cards in the PCMCIA slot
usbscsi.device     Poseidon USB mass storage
massstorage.device other USB mass-storage stacks
diskimage.device   mounted ADF/HDF images
```

Override the list in `novadock.cfg` with a comma-separated replacement, for
example to include a CD-ROM on a separate controller:

```text
OPTION.MEDIA.DEVICES=trackdisk.device,usbscsi.device,cd.device
```

The volume holding `SYS:` is never shown, so a system that boots from a CF
card or USB stick does not get a tile for its own boot volume. At most four
temporary tiles are shown, and only while the 24-tile limit leaves room. If
the extra tiles would stop the dock fitting on screen, they are left out.

Switch the feature off with **Media: Off** on the General page of
Preferences, or `REMOVABLE_MEDIA=NO`.

## How it works

- Detection reads only the DOS device and volume lists. NovaDock never
  calls `Lock()` on a removable volume to find out whether it is present, so
  an empty drive cannot raise "Please insert volume" or start a floppy motor.
- A Commodities filter for disk-inserted and disk-removed input events starts
  a rescan within about half a second. A low-cost rescan every two seconds
  also covers drivers that mount volumes without those events (USB stacks,
  disk images).
- Background work that touches volumes (icon loading and used-space meters,
  also for pinned volumes) runs with system requesters suppressed. A pinned
  floppy that is not in a drive therefore no longer brings up an insert-volume
  requester every few seconds; clicking it still asks for the disk as usual.
- Changing the set of tiles reuses the normal configuration reload, so plugin
  state and graph history are kept.

## Native testing checklist

1. Insert and remove a floppy in DF0: and an external drive; check the tile
   appears within about two seconds and disappears on removal.
2. Insert a disk whose volume is already pinned; no second tile appears.
3. Use **Flush**, then **Pin to dock**, and check the result after a restart.
4. Mount an ADF with diskimage.device and plug in a USB stick under Poseidon.
5. Remove a pinned floppy and leave the dock running: no requester appears.
