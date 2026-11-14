# NovaDock Archiver

Archiver unpacks supported archives and creates ZIP or LHA archives using
the commands installed in `C:`. Existing saved Extract tiles keep working;
the internal `ARCHIVE` ID and `Plugins/ArchiveExtractor` executable are retained.

## Drop an item

Use the tile while the dock is **locked**. When unlocked, drops add dock items.

- **An archive:** open **Extract archive**. The destination defaults to the
  archive's parent drawer. **Create new drawer** is selected and its name
  defaults to the archive filename without the archive suffix. Change the
  destination or drawer name, then press **Extract**.
- **Several files or drawers:** open one **Create archive** dialog and put all
  selected items in the same ZIP/LHA archive. The filename starts empty and must
  be entered. Multiple selected archives are packed together rather than unpacked.
- **A normal file or drawer:** open **Create archive**. Choose ZIP or LHA,
  the destination drawer and archive filename, then press **Compress**.
- **Create Workbench icon (.info):** selected initially in both dialogs; the last choice is remembered.
  Creates an icon for the saved archive or the new extraction drawer. Existing
  icons are preserved. Turn it off to save without adding an icon.
- **Extract directly:** clear **Create new drawer** to use the chosen drawer
  itself. A confirmation explains that the unpacker may replace matching files.
  The new-drawer icon option is disabled in this mode.
- **Cancel:** closes the dialog without creating an archive.

**Open destination when finished** opens the saved archive's parent drawer or
shows the extracted files after a successful operation (including unpacker
warnings). It starts off and remembers your choice. In a finished Shell window,
press **O** to open the destination at any time, then click or press Return to
close the window. Opening a drawer requires Workbench V44 or later.

Compression remembers the last format, save drawer and icon choice across
restarts. A missing save drawer falls back to the source's parent; unavailable
formats fall back to an installed command. Multi-item filenames still start
empty. Extraction keeps its same-parent destination and new-drawer defaults,
and remembers its own icon choice. Last-used choices live in ENV:/ENVARC:
under NovaDock as ArchiveFormat, ArchiveSaveDrawer, ArchiveCreateIcon,
ArchiveExtractIcon and ArchiveOpenDestination.

Browse opens the standard save requester. The filename extension follows the
selected format. ZIP creation requires `C:Zip`; LHA creation requires `C:LhA`.
Unpackers alone cannot create an archive. Missing tools are explained without
changing source files. Single archive drops still open extraction. Combined
archives put each selected basename at their top level, including sources from
different parent drawers. Redundant selections already inside a selected drawer
are included once. Conflicting top-level names are refused; rename or narrow
the selection before retrying. Up to 128 dropped objects are accepted.

Supported suffixes and common file signatures identify archives. A drawer
with a name such as `Photos.zip` is still a drawer and opens compression.
Recognised archives without a suffix suggest `<name>.unpacked` as the new drawer name.
Ordinary left clicks do nothing; right-click **Edit settings...** changes the
status refresh interval.

## Icons and progress

The idle tile shows a zipped archive box. Its arrow points **up while
compressing** and **down while extracting**. Simultaneous compression and
extraction show both directions. A badge counts active jobs. Selecting options
keeps the idle icon. The tooltip is always headed **ARCHIVE** and shows the
operation and current source basename (up to 31 characters). Concurrent jobs
show their count and **Multiple items**. The latest result remains briefly
visible after completion. Completed jobs stop showing the activity arrow and
badge even while their output window remains open. If a helper exits without
a final result, its abandoned status is cleared and briefly shows **INTERRUPTED**.
Only live helper processes count as active jobs.

An independent helper owns each dialog and live Shell progress window, so the
dock stays responsive. The output window reports success or failure and stays
open when the operation finishes. Click inside the Shell window, press Return,
or use its close gadget to dismiss it. Mouse-to-close is enabled only after the
archive command has finished; clicks during an operation do not dismiss it.

## Formats and commands

| Archive | Commands searched in `C:` |
|---|---|
| ZIP | `UnZip`, `unzip` |
| LHA, LZH | `LhA`, `lha` |
| RAR | `UnRAR`, `unrar`, `RAR`, `rar` |
| ARJ | `UnARJ`, `unarj`, `ARJ`, `arj` |
| LZX | `LZX`, `lzx`, `UnLZX`, `unlzx` |
| TAR | `tar` |
| TAR.GZ, TGZ | `tar` (GNU tar with `-z`; needs `gzip` in the path) |
| GZ (single file) | `gzip` |

The first matching command for that format is used. A format remains
unavailable until one of its commands is installed. Right-click the tile and
choose **Edit settings...** to change only its lightweight status refresh interval.

## Destinations and source files

- Existing output archives are refused; choose a different name.
- An archive cannot be saved inside the drawer being compressed, or replace
  the source file. Whole volume roots are not accepted as compression sources.
- Compression runs from the source parent, so the archive contains the source
  basename and its descendants rather than an absolute drive path.
- A source file or drawer's existing `.info` file is included beside it in ZIP
  and LHA archives. Icons inside a drawer are included through recursion.
  Sources without an icon still archive normally. The **Create Workbench icon**
  checkbox controls the new archive's icon; it does not exclude source icons.
  The progress window explicitly names each included source icon.
- Output is built in a reserved temporary drawer beside the chosen destination.
  Only a successful result is renamed to the chosen filename. If another file
  takes that name, the completed result is retained separately and reported.
- Failed compression cleans up only its own temporary output. Source files
  and existing archives are preserved.
- With **Create new drawer** selected, an existing drawer is refused. After
  failure, an empty newly created drawer is removed; a non-empty partial drawer
  can be kept or removed through the existing confirmation prompt.
- Direct extraction never removes the chosen existing drawer during failure
  cleanup. Unpacker output should be reviewed if an operation fails.
- Info-ZIP/UnZip result 1 is shown as **WARNINGS**, with the output retained.
  Review its messages: files may have been skipped. Actual errors still use
  failure cleanup, and other unpackers keep their normal success rules.
- Paths containing shell control characters, quotes or wildcards are refused.
  Source basenames starting with `-` are refused to avoid command options.
- Per-helper status files keep concurrent jobs separate. Workbench windows
  refresh after successful operations where workbench.library supports it.

Creation commands are `Zip -r output source` and `LhA -r a output source`,
run from the source parent with quoted arguments, plus `source.info` when present. The installed commands
provide recursion, compression and extraction; NovaDock does not bundle them.
