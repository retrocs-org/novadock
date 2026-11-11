# NovaDock Archive Extractor

Archive Extractor is a bundled drop target for common Amiga archive formats.
It uses unpacker commands already installed in `C:`; NovaDock does not bundle
or replace those tools.

## Use

1. Add or enable **Extract** in Preferences > Organize. It is enabled in a
   fresh default configuration and offered in **Add item > Plugins** after upgrading an
   older configuration.
2. Drag an archive's Workbench icon onto the Extract tile. This works while
   the dock is locked.
3. NovaDock creates a drawer beside the archive, using the archive filename
   without its final extension, and extracts into that drawer.

Archive Extractor is deliberately drop-only. An ordinary left click does
nothing; it cannot start an empty extraction or change the tile's display.

For example, dropping `Work:Downloads/Demo.lha` creates
`Work:Downloads/Demo/`. The tile reports `EXTRACTING`, then `DONE` or a short
error status. A separate **NovaDock Archive Extractor** Shell window displays
the unpacker's live, file-by-file output and any prompts. None of the supported
extractors are started in quiet mode, so the archive contents remain visible as
they are unpacked. When it finishes, the window shows the result and remains
open until RETURN is pressed or its close gadget is clicked. Extraction runs in
a separate helper process, so the dock and its other plugins keep responding
while the window is open. After a successful extraction, the helper notifies
Workbench that the destination drawer was added; an open window for the
archive's parent directory refreshes automatically to show it.

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

## Safety and retry behaviour

- The destination drawer must not already exist. This avoids silently mixing
  an archive with old contents or prompting from a background command.
- Paths containing shell control characters, wildcards or quotes are rejected.
- Drawers and unknown extensions cannot be dropped on this tile.
- A failed extraction removes the new drawer when it is still empty. If the
  unpacker had already written files, the extraction window asks whether to
  delete the partial drawer (`C:Delete ... ALL`) or keep it for inspection.
- Every extraction writes its own `T:NovaDockArchive-<id>.status` file, so
  several archives can be dropped at once; the tile shows `EXTRACTING 2` while
  two are running and then the latest result.
- Extractor shells receive a 16 KB stack.
- Extractors receive an interactive `CON:` window for both input and output.
  This exposes their normal progress and error text and permits input when an
  installed tool requests it. NovaDock waits only inside the short-lived
  extraction helper, never inside the dock process.

NovaDock constructs these command forms:

```text
UnZip -o archive -d destination
LhA x archive destination/
UnRAR x -o+ archive destination/
ARJ x -y archive destination/
CD destination + UnARJ x archive
LZX x archive destination/
CD destination + UnLZX -x archive
CD destination + tar -xvf archive        (or -xzvf for .tar.gz/.tgz)
gzip >destination/name -dc archive.gz
```

For `.tar.gz` the destination drawer drops both extensions
(`src-1.0.tar.gz` becomes `src-1.0/`). A single `.gz` file is decompressed
into a drawer of its own name without the `.gz`.

Command paths and file paths are quoted by the implementation. The portable
host tests cover case-insensitive recognition, destination naming, unsafe-path
rejection and command construction.
