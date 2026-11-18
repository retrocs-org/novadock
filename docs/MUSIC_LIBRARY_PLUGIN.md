# MUSIC plugin

Add **MUSIC** from **Add item > Plugins**. Add it more than once to keep
separate collections, such as Modules, MP3s and SID tunes. This is a collection
browser; the MusicPlayer plugin remains available for playback controls.

## Settings

Right-click the tile and choose **Edit settings**:

- **Tile label**: your collection name, up to 31 characters.
- **Scan folder (recursive)**: one folder per tile. Every real subdirectory is
  searched. New tiles have no folder selected.
- **Tile icon**: choose from the illustrated library. Music is the default.
- **Custom icon or picture**: an existing file overrides the library choice.
  Clear this field to use the library icon. Missing files fall back to it.
- **Player**: choose a Workbench-compatible music player. Leave blank to use
  the MusicPlayer plugin's Default app, or the file icon's default tool.
- **Format checkboxes**: MP3, MOD, XM, S3M, IT, MED/OctaMED, SID, WAV, AIFF,
  Ogg and FLAC. Only checked formats enter this collection. MP3 and tracker
  modules are enabled initially; enable SID or other formats as needed.

Choose **Use** for the current session or **Save** to retain settings after
restart. Your chosen player must support the formats you enable. Indexing SID
files does not itself add SID playback to a player.

## Browsing and playback

A sad music icon means the folder is unset, missing or unreadable. Its tooltip
explains the reason. With a valid folder, the tile shows your chosen icon and a
corner count, including **0**. **...** indicates an index in progress.

Click a nonempty collection to open its browser. Type in **Filter** to narrow
titles immediately; matching ignores case and accepts multiple words in any
order. **Clear** restores the full list. Select a track and press **Play**, Return,
or double-click it. The file is passed to the configured player as a Workbench
argument. Source files and their icons are not changed.

A zero-count collection stays closed. Click it to rescan; a browser opens only
if matching files are found. **Rescan** in an open browser refreshes the index
after adding or removing music. No selected formats means a zero-item index.

## Saved indexes

Every tile has its own index under
`ENVARC:NovaDock/Cache/MusicLibrary/<instance-id>.idx`. Restarting reuses it;
changing the folder or format checkboxes rebuilds it. Changing just the label,
player or icon reuses the same index. An incomplete scan does not replace the
last complete saved index. Removing a tile removes its settings and index only.

Up to 10,000 tracks and paths up to 299 characters are supported. Directory and
soft links are skipped. Module names support both `mod.tune` and `tune.mod`;
SID tunes accept `.sid`, `.psid`, `.rsid` and the `sid.` prefix. Format detection
uses filenames. Files do not need a Workbench icon to be indexed.

New instances select all eleven supported formats by default. Existing saved
format choices are retained when updating.

## Plugin-owned status artwork

In NovaDock 0.4, the sad status icon ships in
`Plugins/MusicLibrary.assets/Artwork.ndart`, beside the plugin executable.
Update the artwork and executable together, then reload or restart NovaDock.
Missing or damaged artwork falls back to status text.
A valid scan folder continues to use your selected collection or custom icon.
